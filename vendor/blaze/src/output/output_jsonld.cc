#include <sourcemeta/blaze/output_jsonld.h>

#include <sourcemeta/core/email.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/langtag.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/uritemplate.h>

#include <algorithm> // std::ranges::sort, std::ranges::any_of, std::ranges::find
#include <cassert>   // assert
#include <functional>       // std::ref, std::reference_wrapper
#include <initializer_list> // std::initializer_list
#include <memory>           // std::unique_ptr, std::make_unique
#include <optional>         // std::optional
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <tuple>            // std::tie, std::make_tuple
#include <unordered_map>    // std::unordered_map
#include <unordered_set>    // std::unordered_set
#include <utility>          // std::move
#include <variant>          // std::variant, std::get, std::holds_alternative
#include <vector>           // std::vector

namespace {

// The facts gathered at one instance location before its kind is known. Each
// facet remembers the schema location of the annotation that last changed it,
// never of a redundant duplicate, so a later error about the facet can cite
// its origin. The origins are plain pointers into the collected annotation
// entries, so carrying them costs no allocation. The value predicate and the
// literal facets additionally remember their evaluate paths, as retargeting
// consent compares schema object placement
// The promotion facts, boxed behind one pointer on Facts so that schemas
// without promotion keywords never pay for their size in the accumulator
struct PromotionFacts {
  std::optional<sourcemeta::core::JSON::String> value;
  std::optional<sourcemeta::core::JSON> constants;
  const std::string *value_origin{nullptr};
  const std::string *constants_origin{nullptr};
};

struct Facts {
  std::vector<sourcemeta::core::JSONLDEdge> edges;
  std::vector<sourcemeta::core::JSON::String> types;
  std::optional<sourcemeta::core::JSON::String> datatype;
  std::optional<sourcemeta::core::JSON::String> language;
  std::optional<sourcemeta::core::JSONLDDirection> direction;
  std::optional<sourcemeta::core::JSONLDContainer> container;
  std::optional<sourcemeta::core::JSON::String> self;
  std::unique_ptr<PromotionFacts> promotion;
  bool json{false};
  bool graph{false};
  const std::string *edges_origin{nullptr};
  const std::string *reverse_origin{nullptr};
  const std::string *types_origin{nullptr};
  const std::string *datatype_origin{nullptr};
  const std::string *language_origin{nullptr};
  const std::string *direction_origin{nullptr};
  const std::string *container_origin{nullptr};
  const std::string *self_origin{nullptr};
  const std::string *json_origin{nullptr};
  const std::string *graph_origin{nullptr};
};

// The promotion facts of a location, materialized on first use
auto promotion_facts(Facts &facts) -> PromotionFacts & {
  if (facts.promotion == nullptr) {
    facts.promotion = std::make_unique<PromotionFacts>();
  }

  return *facts.promotion;
}

// Whether the location is promoted by a value predicate
auto promoted(const Facts &facts) -> bool {
  return facts.promotion != nullptr && facts.promotion->value.has_value();
}

// Whether the location carries a constants fragment
auto with_constants(const Facts &facts) -> bool {
  return facts.promotion != nullptr && facts.promotion->constants.has_value();
}

auto promotion_value_origin(const Facts &facts) -> const std::string * {
  return facts.promotion == nullptr ? nullptr : facts.promotion->value_origin;
}

auto promotion_constants_origin(const Facts &facts) -> const std::string * {
  return facts.promotion == nullptr ? nullptr
                                    : facts.promotion->constants_origin;
}

using Accumulator = std::unordered_map<sourcemeta::core::WeakPointer, Facts,
                                       sourcemeta::core::WeakPointer::Hasher>;

auto add_edge(std::vector<sourcemeta::core::JSONLDEdge> &edges,
              const sourcemeta::core::JSON::String &predicate,
              const bool reverse) -> bool {
  const auto exists{std::ranges::any_of(
      edges,
      [&predicate, reverse](const sourcemeta::core::JSONLDEdge &edge) -> bool {
        return edge.predicate == predicate && edge.reverse == reverse;
      })};
  if (!exists) {
    edges.push_back({.predicate = predicate, .reverse = reverse});
  }

  return !exists;
}

auto add_type(std::vector<sourcemeta::core::JSON::String> &types,
              const sourcemeta::core::JSON::String &type) -> bool {
  const auto exists{std::ranges::any_of(
      types, [&type](const sourcemeta::core::JSON::String &existing) -> bool {
        return existing == type;
      })};
  if (!exists) {
    types.push_back(type);
  }

  return !exists;
}

// Whether an annotation value is usable as an absolute IRI
auto is_iri_value(const sourcemeta::core::JSON &value) -> bool {
  return value.is_string() && sourcemeta::core::URI::is_iri(value.to_string());
}

// Whether a canonical constants fragment carries a null entry, whose
// legality depends on override marks that only the slow path can consult
auto fragment_has_null_entry(const sourcemeta::core::JSON &fragment) -> bool {
  for (const auto &entry : fragment.as_object()) {
    if (entry.second.is_null()) {
      return true;
    }
  }

  return false;
}

// Union one canonical constants entry into the accumulated constants map,
// unioning and deduplicating the terms under its key. Returns whether
// anything new was contributed
auto merge_constants_entry(std::optional<sourcemeta::core::JSON> &accumulated,
                           const sourcemeta::core::JSON::String &key,
                           const sourcemeta::core::JSON &terms) -> bool {
  if (!accumulated.has_value()) {
    accumulated = sourcemeta::core::JSON::make_object();
  }

  if (!accumulated->defines(key)) {
    accumulated->assign(sourcemeta::core::JSON::String{key},
                        sourcemeta::core::JSON{terms});
    return true;
  }

  bool contributed{false};
  auto &existing{accumulated->at(key)};
  for (const auto &term : terms.as_array()) {
    if (std::ranges::find(existing.as_array(), term) ==
        existing.as_array().cend()) {
      existing.push_back(sourcemeta::core::JSON{term});
      contributed = true;
    }
  }

  return contributed;
}

// Union a canonical constants fragment into the accumulated constants map by
// key. Returns whether anything new was contributed. The caller guarantees
// the fragment carries no null entries
auto merge_constants_fragment(
    std::optional<sourcemeta::core::JSON> &accumulated,
    const sourcemeta::core::JSON &fragment) -> bool {
  bool contributed{false};
  for (const auto &entry : fragment.as_object()) {
    contributed =
        merge_constants_entry(accumulated, entry.first, entry.second) ||
        contributed;
  }

  return contributed;
}

// The constants map a node-producing descriptor takes: the accumulated
// canonical fragment, or the empty map the descriptor kinds default to
auto take_constants(Facts &facts) -> sourcemeta::core::JSON {
  if (with_constants(facts)) {
    return std::move(facts.promotion->constants).value();
  }

  return sourcemeta::core::JSON::make_object();
}

using DirtyLocations =
    std::unordered_set<sourcemeta::core::WeakPointer,
                       sourcemeta::core::WeakPointer::Hasher>;

// Forget the streamed facts of a location whose annotations cannot be
// resolved by simple agreement, handing it over to the override-aware slow
// path
auto demote(Accumulator &accumulator, DirtyLocations &dirty,
            const sourcemeta::core::WeakPointer &location) -> void {
  accumulator.erase(location);
  dirty.insert(location);
}

// The first present schema origin among an error's counterpart annotations.
// Only runs on the error path
auto first_origin(std::initializer_list<const std::string *> pointers)
    -> std::optional<std::string> {
  for (const auto *origin : pointers) {
    if (origin != nullptr) {
      return *origin;
    }
  }

  return std::nullopt;
}

auto type_iri_error(const sourcemeta::core::WeakPointer &instance_location,
                    const std::string &origin)
    -> sourcemeta::blaze::JSONLDResolutionError {
  return {.instance_location = sourcemeta::core::to_pointer(instance_location),
          .facet = sourcemeta::blaze::JSONLDFacet::Type,
          .message = "The value of x-jsonld-type must be an absolute IRI",
          .schema_location = origin,
          .conflicting_schema_location = std::nullopt,
          .inert_override_location = std::nullopt};
}

auto facet_error(
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::blaze::JSONLDFacet facet, std::string message,
    std::string schema_location,
    std::optional<std::string> conflicting_schema_location = std::nullopt,
    std::optional<std::string> inert_override_location = std::nullopt)
    -> sourcemeta::blaze::JSONLDResolutionError {
  return {.instance_location = sourcemeta::core::to_pointer(instance_location),
          .facet = facet,
          .message = std::move(message),
          .schema_location = std::move(schema_location),
          .conflicting_schema_location = std::move(conflicting_schema_location),
          .inert_override_location = std::move(inert_override_location)};
}

// Validate and normalize an x-jsonld-constants fragment into canonical
// expanded form, wrapping the fragment grammar violation, if any, as a
// resolution error citing the annotation
auto canonicalize_constants(
    const sourcemeta::core::JSON &fragment,
    const sourcemeta::core::WeakPointer &instance_location,
    const std::string &origin)
    -> std::variant<sourcemeta::core::JSON,
                    sourcemeta::blaze::JSONLDResolutionError> {
  try {
    return sourcemeta::core::jsonld_canonicalize_fragment(fragment);
  } catch (const sourcemeta::core::JSONLDFragmentError &error) {
    return facet_error(instance_location,
                       sourcemeta::blaze::JSONLDFacet::Constants, error.what(),
                       origin);
  }
}

auto parse_direction(const sourcemeta::core::JSON &value)
    -> std::optional<sourcemeta::core::JSONLDDirection> {
  if (!value.is_string()) {
    return std::nullopt;
  }

  const auto &text{value.to_string()};
  if (text == "ltr") {
    return sourcemeta::core::JSONLDDirection::LTR;
  }
  if (text == "rtl") {
    return sourcemeta::core::JSONLDDirection::RTL;
  }

  return std::nullopt;
}

auto direction_text(const sourcemeta::core::JSONLDDirection direction)
    -> sourcemeta::core::JSON::String {
  return direction == sourcemeta::core::JSONLDDirection::LTR ? "ltr" : "rtl";
}

auto parse_container(const sourcemeta::core::JSON &value)
    -> std::optional<sourcemeta::core::JSONLDContainer> {
  if (!value.is_string()) {
    return std::nullopt;
  }

  const auto &text{value.to_string()};
  if (text == "@list") {
    return sourcemeta::core::JSONLDContainer::List;
  }
  if (text == "@set") {
    return sourcemeta::core::JSONLDContainer::Set;
  }
  if (text == "@language") {
    return sourcemeta::core::JSONLDContainer::Language;
  }
  if (text == "@index") {
    return sourcemeta::core::JSONLDContainer::Index;
  }

  return std::nullopt;
}

// A container overrides the value shape, as a list or set ranges over an array
// and a language or index map ranges over an object. A language map only holds
// string members, which is what the materializer requires
auto container_placement_error(
    const sourcemeta::core::WeakPointer &pointer,
    const sourcemeta::core::JSONLDContainer container,
    const sourcemeta::core::JSON &value, const std::string &origin)
    -> std::optional<sourcemeta::blaze::JSONLDResolutionError> {
  if (container == sourcemeta::core::JSONLDContainer::List ||
      container == sourcemeta::core::JSONLDContainer::Set) {
    if (!value.is_array()) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Container,
          "A JSON-LD list or set container can only be assigned to an array "
          "value",
          origin);
    }

    return std::nullopt;
  }

  if (!value.is_object()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Container,
        "A JSON-LD language or index container can only be assigned to an "
        "object value",
        origin);
  }

  if (container == sourcemeta::core::JSONLDContainer::Language) {
    for (const auto &entry : value.as_object()) {
      if (entry.first != "@none" &&
          !sourcemeta::core::is_canonical_langtag(entry.first)) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Container,
                           "A JSON-LD language container requires canonical "
                           "BCP 47 language tag keys",
                           origin);
      }

      // A null member, or a null item in an array member, is treated as absent
      const auto &member{entry.second};
      const bool usable{member.is_null() || member.is_string() ||
                        (member.is_array() &&
                         std::ranges::all_of(
                             member.as_array(),
                             [](const sourcemeta::core::JSON &element) -> bool {
                               return element.is_string() || element.is_null();
                             }))};
      if (!usable) {
        return facet_error(
            pointer, sourcemeta::blaze::JSONLDFacet::Container,
            "A JSON-LD language container requires string or null members",
            origin);
      }
    }
  }

  return std::nullopt;
}

// Whether the facts suit the value shape, as a node fact needs an object and a
// literal fact needs a scalar. Returns the first mismatch, or nothing
auto placement_error(const sourcemeta::core::WeakPointer &pointer,
                     const Facts &facts, const sourcemeta::core::JSON &value)
    -> std::optional<sourcemeta::blaze::JSONLDResolutionError> {
  if (!value.is_object()) {
    // A self identity promotes a scalar to a reference, which carries its own
    // types, so a type is only misplaced on a scalar that has no self identity
    if (!facts.types.empty() && !facts.self.has_value() && !promoted(facts)) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Type,
          "A JSON-LD type can only be assigned to an object value",
          *facts.types_origin);
    }

    if (facts.graph) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Graph,
          "A JSON-LD graph flag can only be assigned to an object value",
          *facts.graph_origin);
    }
  }

  if (!value.is_object() && !value.is_array()) {
    return std::nullopt;
  }

  if (facts.datatype.has_value()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Datatype,
        "A JSON-LD datatype can only be assigned to a scalar value",
        *facts.datatype_origin);
  }

  if (facts.language.has_value()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Language,
        "A JSON-LD language can only be assigned to a scalar value",
        *facts.language_origin);
  }

  if (facts.direction.has_value()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Direction,
        "A JSON-LD direction can only be assigned to a scalar value",
        *facts.direction_origin);
  }

  return std::nullopt;
}

// The constraints among literal facts, as a datatype excludes a language or
// direction, and a language or direction needs a string. Returns the first
// violation, or nothing
auto literal_error(const sourcemeta::core::WeakPointer &pointer,
                   const Facts &facts, const sourcemeta::core::JSON &value)
    -> std::optional<sourcemeta::blaze::JSONLDResolutionError> {
  if (facts.datatype.has_value() &&
      (facts.language.has_value() || facts.direction.has_value())) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Datatype,
        "A JSON-LD datatype cannot carry a language or direction",
        *facts.datatype_origin,
        first_origin({facts.language_origin, facts.direction_origin}));
  }

  if (facts.language.has_value() && !value.is_string()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Language,
        "A JSON-LD language can only be assigned to a string value",
        *facts.language_origin);
  }

  if (facts.direction.has_value() && !value.is_string()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Direction,
        "A JSON-LD direction can only be assigned to a string value",
        *facts.direction_origin);
  }

  return std::nullopt;
}

// Expand an x-jsonld-self URI Template into a concrete identifier. An object
// binds each variable to the member of that name, and a scalar binds the
// reserved variable this to its own value. Only a non-empty string can bind,
// and a binding that is not usable, or a result that is not an absolute IRI,
// is a fail-loud resolution error. Expansion runs in IRI mode so that
// internationalized characters flowing through a template mint the same raw
// term that constant identities emit, as RDF compares IRIs by simple string
// comparison (RDF 1.1 Concepts Section 3.2). A scheme identity name bypasses
// expansion entirely, minting the canonical IRI of the string value in the
// named scheme, where an input outside the scheme's source grammar is a
// fail-loud resolution error
auto expand_self(const sourcemeta::core::WeakPointer &pointer,
                 const sourcemeta::core::JSON::String &pattern,
                 const sourcemeta::core::JSON &value, const std::string &origin)
    -> std::variant<sourcemeta::core::JSON::String,
                    sourcemeta::blaze::JSONLDResolutionError> {
  if (pattern == "mailto" || pattern == "acct") {
    if (!value.is_string()) {
      return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Self,
                         "A JSON-LD self identity scheme can only be assigned "
                         "to a string value",
                         origin);
    }

    auto identity{pattern == "mailto"
                      ? sourcemeta::core::mailto_iri(value.to_string())
                      : sourcemeta::core::acct_iri(value.to_string())};
    if (!identity.has_value()) {
      return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Self,
                         "A JSON-LD self identity value is outside the domain "
                         "of its scheme",
                         origin);
    }

    return sourcemeta::core::JSON::String{std::move(identity.value())};
  }

  std::optional<sourcemeta::blaze::JSONLDResolutionError> failure;
  const sourcemeta::core::URITemplate uri_template{pattern};
  auto expanded{uri_template.expand(
      [&value, &pointer, &failure, &origin](
          const std::string_view name) -> sourcemeta::core::URITemplateValue {
        const sourcemeta::core::JSON *bound{nullptr};
        if (value.is_object()) {
          bound = value.try_at(name);
        } else if (name == "this") {
          bound = &value;
        }

        if (bound == nullptr) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable must bind to an "
                "instance value",
                origin);
          }

          return std::nullopt;
        }

        if (bound->is_null()) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable cannot bind to a "
                "null value",
                origin);
          }

          return std::nullopt;
        }

        if (!bound->is_string()) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable can only bind to a "
                "string value",
                origin);
          }

          return std::nullopt;
        }

        const auto &text{bound->to_string()};
        if (text.empty()) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable cannot bind to an "
                "empty string",
                origin);
          }

          return std::nullopt;
        }

        return std::make_tuple(std::string_view{text}, std::nullopt, false);
      },
      sourcemeta::core::URITemplateExpansionMode::IRI)};

  if (failure.has_value()) {
    return failure.value();
  }

  if (!sourcemeta::core::URI::is_iri(expanded)) {
    return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Self,
                       "A JSON-LD self identity must expand to an absolute IRI",
                       origin);
  }

  return sourcemeta::core::JSON::String{std::move(expanded)};
}

// Whether every element of an array materializes as a node, so the array can be
// the subject of a reverse predicate. An element is a node when it is a raw
// object or a scalar promoted to a reference by its own self identity
auto array_of_nodes(const Accumulator &accumulator,
                    const sourcemeta::core::WeakPointer &pointer,
                    const sourcemeta::core::JSON &value) -> bool {
  for (std::size_t index = 0; index < value.size(); index += 1) {
    if (value.at(index).is_object()) {
      continue;
    }

    auto element_pointer{pointer};
    element_pointer.push_back(index);
    const auto element_facts{accumulator.find(element_pointer)};
    if (element_facts == accumulator.cend() ||
        (!element_facts->second.self.has_value() &&
         !promoted(element_facts->second))) {
      return false;
    }
  }

  return true;
}

// Whether the first annotation's schema object encloses the second's, which
// is the outer-to-inner relation that x-jsonld-override shadows along. Both
// arguments are full evaluate paths whose trailing token is the keyword, so
// the relation holds when the first path's parent strictly prefixes the
// second's
auto encloses(const sourcemeta::core::WeakPointer &outer,
              const sourcemeta::core::WeakPointer &inner) -> bool {
  return outer.size() < inner.size() && inner.starts_with_initial(outer);
}

// Whether two annotations share their schema object, that is, their evaluate
// paths differ at most in the trailing keyword token
auto same_schema_object(const sourcemeta::core::WeakPointer &left,
                        const sourcemeta::core::WeakPointer &right) -> bool {
  return left.size() == right.size() && right.starts_with_initial(left);
}

// The declaration paths that can consent to retargeting at one location:
// the paths declaring the resolved value predicate and the paths of the
// override marks. Only promotion locations with a cross-facet fusion ever
// collect them, so the scans never cost the common paths anything
struct ConsentPaths {
  std::vector<const sourcemeta::core::WeakPointer *> values;
  std::vector<const sourcemeta::core::WeakPointer *> marks;
};

// Whether a facet declaration is consented to follow the value predicate
// into the promoted node: either it shares a schema object with a value
// predicate declaration, one author's intentional spelling, or a value
// predicate declaration is override-marked and its schema object encloses
// the facet's, a composer restructuring what it composes
auto declaration_consented(const ConsentPaths &consent,
                           const sourcemeta::core::WeakPointer &facet_path)
    -> bool {
  for (const auto *value_path : consent.values) {
    if (same_schema_object(*value_path, facet_path)) {
      return true;
    }

    for (const auto *mark : consent.marks) {
      if (same_schema_object(*mark, *value_path) &&
          encloses(*value_path, facet_path)) {
        return true;
      }
    }
  }

  return false;
}

// An x-jsonld-override annotation that evaluated to true at a location,
// paired with the schema location of the mark itself for error citations
struct Mark {
  const sourcemeta::core::WeakPointer *path;
  const std::string *origin;
};

// A collected keyword value pending resolution, paired with the evaluate
// path and schema origin of the annotation that carried it, plus the
// override mark sharing its schema object, if any
struct Candidate {
  const sourcemeta::core::JSON *value;
  const sourcemeta::core::WeakPointer *path;
  const std::string *origin;
  const Mark *mark{nullptr};
};

// The candidates gathered at one instance location before resolution, plus
// the x-jsonld-override annotations that evaluated to true at that location
struct Pending {
  std::vector<Candidate> ids;
  std::vector<Candidate> reverses;
  std::vector<Candidate> types;
  std::vector<Candidate> datatypes;
  std::vector<Candidate> languages;
  std::vector<Candidate> directions;
  std::vector<Candidate> jsons;
  std::vector<Candidate> graphs;
  std::vector<Candidate> containers;
  std::vector<Candidate> selves;
  std::vector<Candidate> values;
  std::vector<Candidate> constants;
  std::vector<Mark> marks;

  [[nodiscard]] auto annotated() const -> bool {
    return !this->ids.empty() || !this->reverses.empty() ||
           !this->types.empty() || !this->datatypes.empty() ||
           !this->languages.empty() || !this->directions.empty() ||
           !this->jsons.empty() || !this->graphs.empty() ||
           !this->containers.empty() || !this->selves.empty() ||
           !this->values.empty() || !this->constants.empty();
  }
};

using PendingMap = std::unordered_map<sourcemeta::core::WeakPointer, Pending,
                                      sourcemeta::core::WeakPointer::Hasher>;

// The x-jsonld-override mark sharing its schema object with the annotation
// at the given evaluate path, if any
auto find_mark(const std::vector<Mark> &marks,
               const sourcemeta::core::WeakPointer &path) -> const Mark * {
  for (const auto &mark : marks) {
    if (mark.path->size() == path.size() &&
        path.starts_with_initial(*mark.path)) {
      return &mark;
    }
  }

  return nullptr;
}

// A null that no override mark licenses declares nothing, as if the keyword
// were absent
auto unmarked_null(const Candidate &candidate) -> bool {
  return candidate.value->is_null() && candidate.mark == nullptr;
}

// Attach the location's override marks to a facet's candidates and drop the
// nulls that no mark licenses. This cannot happen at collection time, as the
// sibling mark may be collected in any order relative to the null
auto prepare(std::vector<Candidate> &candidates, const std::vector<Mark> &marks)
    -> void {
  for (auto &candidate : candidates) {
    candidate.mark = find_mark(marks, *candidate.path);
  }

  std::erase_if(candidates, unmarked_null);
}

// The comparable form of a single-valued candidate, where an absent result
// stands for a tombstone
using NormalizedKey = std::optional<sourcemeta::core::JSON::String>;

auto key_exact(const sourcemeta::core::JSON &value) -> NormalizedKey {
  if (value.is_null()) {
    return std::nullopt;
  }

  return value.to_string();
}

// Only true booleans survive collection, so a tombstone normalizes to the
// absent state exactly like every other facet
auto key_boolean(const sourcemeta::core::JSON &value) -> NormalizedKey {
  if (value.is_null()) {
    return std::nullopt;
  }

  return sourcemeta::core::JSON::String{"true"};
}

// The resolution of a single-valued facet at one instance location. An
// absent winner without a conflict means the facet resolves to its absent
// state, either because no candidate exists or because a tombstone won. A
// conflict is witnessed by its first diverging pair of surviving candidates,
// mirroring the fail-first semantics of resolution errors
struct Election {
  const Candidate *winner{nullptr};
  bool conflict{false};
  const Candidate *first{nullptr};
  const Candidate *second{nullptr};
};

// Resolve a single-valued facet: shadowed values are discarded, identical
// survivors deduplicate, and more than one distinct surviving value is a
// conflict. The evaluate path comparisons only happen when the candidates
// genuinely diverge
auto elect(const std::vector<Candidate> &candidates,
           NormalizedKey (*const key)(const sourcemeta::core::JSON &))
    -> Election {
  if (candidates.empty()) {
    return {};
  }

  std::vector<NormalizedKey> keys;
  keys.reserve(candidates.size());
  bool divergent{false};
  for (const auto &candidate : candidates) {
    keys.push_back(key(*candidate.value));
    divergent = divergent || keys.back() != keys.front();
  }

  std::vector<std::size_t> survivors;
  survivors.reserve(candidates.size());
  if (divergent) {
    for (std::size_t index = 0; index < candidates.size(); index += 1) {
      bool shadowed{false};
      for (std::size_t other = 0; other < candidates.size(); other += 1) {
        if (candidates[other].mark != nullptr && keys[other] != keys[index] &&
            encloses(*candidates[other].path, *candidates[index].path)) {
          shadowed = true;
          break;
        }
      }

      if (!shadowed) {
        survivors.push_back(index);
      }
    }
  } else {
    for (std::size_t index = 0; index < candidates.size(); index += 1) {
      survivors.push_back(index);
    }
  }

  // Enclosure is a strict relation, so the candidate with the shortest
  // evaluate path can never be shadowed
  assert(!survivors.empty());
  const auto &elected{keys[survivors.front()]};
  for (const auto index : survivors) {
    if (keys[index] != elected) {
      return {.winner = nullptr,
              .conflict = true,
              .first = &candidates[survivors.front()],
              .second = &candidates[index]};
    }
  }

  if (!elected.has_value()) {
    return {};
  }

  // Every survivor carries the elected key, and equal keys mean equal
  // values, so any survivor serves as the winner
  return {.winner = &candidates[survivors.front()],
          .conflict = false,
          .first = nullptr,
          .second = nullptr};
}

// The error for a facet whose surviving candidates diverge, citing the
// witnessing pair. A surviving marked candidate failed to shadow the
// diverging competitor it does not enclose, so its mark is cited as inert
auto conflict_error(const sourcemeta::core::WeakPointer &instance_location,
                    const sourcemeta::blaze::JSONLDFacet facet,
                    std::string message, const Election &election)
    -> sourcemeta::blaze::JSONLDResolutionError {
  const auto *marked{election.first->mark != nullptr    ? election.first
                     : election.second->mark != nullptr ? election.second
                                                        : nullptr};
  return facet_error(instance_location, facet, std::move(message),
                     *election.first->origin, *election.second->origin,
                     marked == nullptr
                         ? std::nullopt
                         : std::optional<std::string>{*marked->mark->origin});
}

// A false boolean never enters resolution, as the absent state it restates
// already is false, so removal only ever spells null
auto false_boolean(const Candidate &candidate) -> bool {
  return candidate.value->is_boolean() && !candidate.value->to_boolean();
}

// Whether an additive candidate is removed by a sibling tombstone, which
// happens when the tombstone's schema object encloses the candidate's
auto removed_by_tombstone(const std::vector<Candidate> &candidates,
                          const Candidate &candidate) -> bool {
  for (const auto &tombstone : candidates) {
    if (tombstone.value->is_null() &&
        encloses(*tombstone.path, *candidate.path)) {
      return true;
    }
  }

  return false;
}

auto resolve_edges(const std::vector<Candidate> &candidates, const bool reverse,
                   Facts &facts) -> void {
  for (const auto &candidate : candidates) {
    if (candidate.value->is_null() ||
        removed_by_tombstone(candidates, candidate)) {
      continue;
    }

    if (add_edge(facts.edges, candidate.value->to_string(), reverse)) {
      facts.edges_origin = candidate.origin;
      if (reverse) {
        facts.reverse_origin = candidate.origin;
      }
    }
  }
}

auto resolve_types(const std::vector<Candidate> &candidates, Facts &facts)
    -> void {
  for (const auto &candidate : candidates) {
    if (candidate.value->is_null() ||
        removed_by_tombstone(candidates, candidate)) {
      continue;
    }

    if (candidate.value->is_array()) {
      bool contributed{false};
      for (const auto &element : candidate.value->as_array()) {
        contributed = add_type(facts.types, element.to_string()) || contributed;
      }

      if (contributed) {
        facts.types_origin = candidate.origin;
      }
    } else if (add_type(facts.types, candidate.value->to_string())) {
      facts.types_origin = candidate.origin;
    }
  }
}

// Whether a constants contribution under the given key is removed by a
// marked tombstone whose schema object encloses the contributor's: either a
// null for the whole keyword or a null entry for that key
auto constants_key_removed(const std::vector<Candidate> &candidates,
                           const std::vector<sourcemeta::core::JSON> &fragments,
                           const Candidate &candidate,
                           const sourcemeta::core::JSON::String &key) -> bool {
  for (std::size_t index = 0; index < candidates.size(); index += 1) {
    const auto &tombstone{candidates[index]};
    if (tombstone.mark == nullptr ||
        !encloses(*tombstone.path, *candidate.path)) {
      continue;
    }

    const auto &fragment{fragments[index]};
    if (fragment.is_null()) {
      return true;
    }

    const auto *entry{fragment.try_at(key)};
    if (entry != nullptr && entry->is_null()) {
      return true;
    }
  }

  return false;
}

// Resolve the additive constants facet: canonical fragments union by key,
// with per-key term union and dedupe. A marked null for the whole keyword
// tombstones every entry collected beneath its schema object, and a marked
// null entry tombstones its own key beneath, while a null entry outside an
// overriding schema object is an error
auto resolve_constants(const sourcemeta::core::WeakPointer &location,
                       const std::vector<Candidate> &candidates, Facts &facts)
    -> std::optional<sourcemeta::blaze::JSONLDResolutionError> {
  if (candidates.empty()) {
    return std::nullopt;
  }

  // The first pass validated every fragment, so canonicalization cannot
  // fail here
  std::vector<sourcemeta::core::JSON> fragments;
  fragments.reserve(candidates.size());
  for (const auto &candidate : candidates) {
    if (candidate.value->is_null()) {
      fragments.emplace_back(nullptr);
      continue;
    }

    fragments.push_back(
        sourcemeta::core::jsonld_canonicalize_fragment(*candidate.value));
    if (candidate.mark == nullptr &&
        fragment_has_null_entry(fragments.back())) {
      return facet_error(location, sourcemeta::blaze::JSONLDFacet::Constants,
                         "A JSON-LD constants entry can only be null inside "
                         "an overriding schema object",
                         *candidate.origin);
    }
  }

  for (std::size_t index = 0; index < candidates.size(); index += 1) {
    const auto &candidate{candidates[index]};
    const auto &fragment{fragments[index]};
    if (fragment.is_null()) {
      continue;
    }

    bool contributed{false};
    for (const auto &entry : fragment.as_object()) {
      if (entry.second.is_null() ||
          constants_key_removed(candidates, fragments, candidate,
                                entry.first)) {
        continue;
      }

      contributed = merge_constants_entry(promotion_facts(facts).constants,
                                          entry.first, entry.second) ||
                    contributed;
    }

    if (contributed) {
      promotion_facts(facts).constants_origin = candidate.origin;
    }
  }

  return std::nullopt;
}

// The x-jsonld-* keyword names, pre-hashed for the first-pass dispatch
using namespace std::string_view_literals;
const auto HASH_ID{sourcemeta::core::JSON::Object::hash("x-jsonld-id"sv)};
const auto HASH_REVERSE{
    sourcemeta::core::JSON::Object::hash("x-jsonld-reverse"sv)};
const auto HASH_TYPE{sourcemeta::core::JSON::Object::hash("x-jsonld-type"sv)};
const auto HASH_DATATYPE{
    sourcemeta::core::JSON::Object::hash("x-jsonld-datatype"sv)};
const auto HASH_LANGUAGE{
    sourcemeta::core::JSON::Object::hash("x-jsonld-language"sv)};
const auto HASH_DIRECTION{
    sourcemeta::core::JSON::Object::hash("x-jsonld-direction"sv)};
const auto HASH_JSON{sourcemeta::core::JSON::Object::hash("x-jsonld-json"sv)};
const auto HASH_GRAPH{sourcemeta::core::JSON::Object::hash("x-jsonld-graph"sv)};
const auto HASH_CONTAINER{
    sourcemeta::core::JSON::Object::hash("x-jsonld-container"sv)};
const auto HASH_SELF{sourcemeta::core::JSON::Object::hash("x-jsonld-self"sv)};
const auto HASH_OVERRIDE{
    sourcemeta::core::JSON::Object::hash("x-jsonld-override"sv)};
const auto HASH_VALUE{sourcemeta::core::JSON::Object::hash("x-jsonld-value"sv)};
const auto HASH_CONSTANTS{
    sourcemeta::core::JSON::Object::hash("x-jsonld-constants"sv)};

// The retargeting-relevant annotation kinds of the consent index
enum class ConsentKind : std::uint8_t {
  ValuePredicate,
  OverrideMark,
  Datatype,
  Language,
  Direction,
  Self
};

// One retargeting-relevant annotation at a location
struct ConsentAnnotation {
  ConsentKind kind;
  const sourcemeta::core::WeakPointer *path;
  const sourcemeta::core::JSON *value;
};

// The retargeting-relevant annotations of every location, built in one pass
// the first time any promoted location needs consent, so that consent stays
// linear in the collected annotations no matter how many locations promote
using ConsentIndex = std::unordered_map<sourcemeta::core::WeakPointer,
                                        std::vector<ConsentAnnotation>,
                                        sourcemeta::core::WeakPointer::Hasher>;

auto build_consent_index(const sourcemeta::blaze::SimpleOutput &output)
    -> ConsentIndex {
  ConsentIndex index;
  for (const auto &entry : output.annotations()) {
    if (entry.evaluate_path.empty()) {
      continue;
    }

    const auto &keyword{entry.evaluate_path.back()};
    std::optional<ConsentKind> kind;
    if (keyword.property_equals("x-jsonld-value", HASH_VALUE)) {
      if (entry.value.is_string()) {
        kind = ConsentKind::ValuePredicate;
      }
    } else if (keyword.property_equals("x-jsonld-override", HASH_OVERRIDE)) {
      if (entry.value.is_boolean() && entry.value.to_boolean()) {
        kind = ConsentKind::OverrideMark;
      }
    } else if (keyword.property_equals("x-jsonld-datatype", HASH_DATATYPE)) {
      kind = ConsentKind::Datatype;
    } else if (keyword.property_equals("x-jsonld-language", HASH_LANGUAGE)) {
      kind = ConsentKind::Language;
    } else if (keyword.property_equals("x-jsonld-direction", HASH_DIRECTION)) {
      kind = ConsentKind::Direction;
    } else if (keyword.property_equals("x-jsonld-self", HASH_SELF)) {
      kind = ConsentKind::Self;
    }

    if (kind.has_value()) {
      index[entry.instance_location].push_back({.kind = kind.value(),
                                                .path = &entry.evaluate_path,
                                                .value = &entry.value});
    }
  }

  return index;
}

// The consent paths of a promoted location: the declarations of its
// resolved value predicate and the override marks beside them
auto consent_paths_at(const std::vector<ConsentAnnotation> &annotations,
                      const sourcemeta::core::JSON::String &value_predicate)
    -> ConsentPaths {
  ConsentPaths result;
  for (const auto &annotation : annotations) {
    if (annotation.kind == ConsentKind::ValuePredicate &&
        annotation.value->to_string() == value_predicate) {
      result.values.push_back(annotation.path);
    } else if (annotation.kind == ConsentKind::OverrideMark) {
      result.marks.push_back(annotation.path);
    }
  }

  return result;
}

// Whether any declaration of the given facet value at the location is
// consented to follow the value predicate into the promoted node
auto facet_retargeting_consented(
    const std::vector<ConsentAnnotation> &annotations,
    const ConsentPaths &consent, const ConsentKind kind,
    const sourcemeta::core::JSON &expected) -> bool {
  for (const auto &annotation : annotations) {
    if (annotation.kind == kind && *annotation.value == expected &&
        declaration_consented(consent, *annotation.path)) {
      return true;
    }
  }

  return false;
}

// Gather an already-validated annotation into its location's pending
// candidates for the override-aware slow path
auto collect(const sourcemeta::blaze::SimpleOutput::AnnotationEntry &entry,
             Pending &pending) -> void {
  const auto &keyword{entry.evaluate_path.back()};
  const Candidate candidate{.value = &entry.value,
                            .path = &entry.evaluate_path,
                            .origin = &entry.schema_location.get()};

  if (keyword.property_equals("x-jsonld-override", HASH_OVERRIDE)) {
    if (entry.value.to_boolean()) {
      pending.marks.push_back({.path = &entry.evaluate_path,
                               .origin = &entry.schema_location.get()});
    }
  } else if (keyword.property_equals("x-jsonld-id", HASH_ID)) {
    pending.ids.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-reverse", HASH_REVERSE)) {
    pending.reverses.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-type", HASH_TYPE)) {
    pending.types.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-datatype", HASH_DATATYPE)) {
    pending.datatypes.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-language", HASH_LANGUAGE)) {
    pending.languages.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-direction", HASH_DIRECTION)) {
    pending.directions.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-json", HASH_JSON)) {
    pending.jsons.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-graph", HASH_GRAPH)) {
    pending.graphs.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-container", HASH_CONTAINER)) {
    pending.containers.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-self", HASH_SELF)) {
    pending.selves.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-value", HASH_VALUE)) {
    pending.values.push_back(candidate);
  } else if (keyword.property_equals("x-jsonld-constants", HASH_CONSTANTS)) {
    pending.constants.push_back(candidate);
  }
}

// Turn the JSON-LD annotations into a resolved list, or the first error found.
// The first pass validates every annotation and streams agreeing values
// straight into the accumulator, demoting a location to the slow path the
// moment a null or a genuinely diverging value arrives. The slow path
// collects the demoted locations' candidates and resolves each facet,
// shadowing through x-jsonld-override when the candidates genuinely conflict.
// The last pass derives each kind from the value shape and validates the
// facts against it
auto resolve(const sourcemeta::core::JSON &instance,
             const sourcemeta::blaze::SimpleOutput &output)
    -> std::variant<sourcemeta::core::JSONLDWeakAnnotationList,
                    sourcemeta::blaze::JSONLDResolutionError> {
  Accumulator accumulator;
  DirtyLocations dirty;

  for (const auto &entry : output.annotations()) {
    if (entry.evaluate_path.empty()) {
      continue;
    }

    const auto &keyword{entry.evaluate_path.back()};
    const auto &instance_location{entry.instance_location};
    const auto &value{entry.value};
    const auto *const origin{&entry.schema_location.get()};

    if (keyword.property_equals("x-jsonld-override", HASH_OVERRIDE)) {
      if (!value.is_boolean()) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Override,
            "The value of x-jsonld-override must be a boolean", *origin);
      }

      // A mark only matters when it shadows a diverging value or licenses a
      // null or a false, and each of those demotes the location on its own,
      // so the slow path recollects the mark whenever it can act. The one
      // exception is retargeting consent, where a mark reshapes an agreeing
      // resolution, which the final pass resolves by scanning the collected
      // annotations only when a value predicate actually needs consent
    } else if (keyword.property_equals("x-jsonld-id", HASH_ID) ||
               keyword.property_equals("x-jsonld-reverse", HASH_REVERSE)) {
      const bool reverse{
          keyword.property_equals("x-jsonld-reverse", HASH_REVERSE)};
      if (!value.is_null() && !is_iri_value(value)) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Predicate,
            reverse ? "The value of x-jsonld-reverse must be an absolute IRI"
                    : "The value of x-jsonld-id must be an absolute IRI",
            *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (add_edge(facts.edges, value.to_string(), reverse)) {
          facts.edges_origin = origin;
          if (reverse) {
            facts.reverse_origin = origin;
          }
        }
      }
    } else if (keyword.property_equals("x-jsonld-type", HASH_TYPE)) {
      if (value.is_array()) {
        for (const auto &element : value.as_array()) {
          if (!is_iri_value(element)) {
            return type_iri_error(instance_location, *origin);
          }
        }
      } else if (!value.is_null() && !is_iri_value(value)) {
        return type_iri_error(instance_location, *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (value.is_array()) {
          bool contributed{false};
          for (const auto &element : value.as_array()) {
            contributed =
                add_type(facts.types, element.to_string()) || contributed;
          }

          if (contributed) {
            facts.types_origin = origin;
          }
        } else if (add_type(facts.types, value.to_string())) {
          facts.types_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-datatype", HASH_DATATYPE)) {
      if (!value.is_null() && !is_iri_value(value)) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Datatype,
            "The value of x-jsonld-datatype must be an absolute IRI", *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        if (facts.datatype.has_value() && facts.datatype.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else if (!facts.datatype.has_value()) {
          facts.datatype = text;
          facts.datatype_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-language", HASH_LANGUAGE)) {
      if (!value.is_null() &&
          (!value.is_string() ||
           !sourcemeta::core::is_canonical_langtag(value.to_string()))) {
        return facet_error(instance_location,
                           sourcemeta::blaze::JSONLDFacet::Language,
                           "The value of x-jsonld-language must be a "
                           "canonical BCP 47 language tag",
                           *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        if (facts.language.has_value() && facts.language.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else if (!facts.language.has_value()) {
          facts.language = text;
          facts.language_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-direction", HASH_DIRECTION)) {
      const auto direction{parse_direction(value)};
      if (!value.is_null() && !direction.has_value()) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Direction,
            R"(The value of x-jsonld-direction must be "ltr" or "rtl")",
            *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (facts.direction.has_value() && facts.direction != direction) {
          demote(accumulator, dirty, instance_location);
        } else if (!facts.direction.has_value()) {
          facts.direction = direction;
          facts.direction_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-json", HASH_JSON) ||
               keyword.property_equals("x-jsonld-graph", HASH_GRAPH)) {
      const bool graph{keyword.property_equals("x-jsonld-graph", HASH_GRAPH)};
      if (!value.is_null() && !value.is_boolean()) {
        return facet_error(instance_location,
                           graph ? sourcemeta::blaze::JSONLDFacet::Graph
                                 : sourcemeta::blaze::JSONLDFacet::JSON,
                           graph
                               ? "The value of x-jsonld-graph must be a boolean"
                               : "The value of x-jsonld-json must be a boolean",
                           *origin);
      }

      // A false declares nothing anywhere, as the absent state it restates
      // already is false, so removal only ever spells null
      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (value.to_boolean() && !dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (graph) {
          if (!facts.graph) {
            facts.graph = true;
            facts.graph_origin = origin;
          }
        } else if (!facts.json) {
          facts.json = true;
          facts.json_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-container", HASH_CONTAINER)) {
      const auto container{parse_container(value)};
      if (!value.is_null() && !container.has_value()) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Container,
            R"(The value of x-jsonld-container must be "@list", "@set", "@language", or "@index")",
            *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (facts.container.has_value() && facts.container != container) {
          demote(accumulator, dirty, instance_location);
        } else if (!facts.container.has_value()) {
          facts.container = container;
          facts.container_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-self", HASH_SELF)) {
      if (!value.is_null() &&
          (!value.is_string() ||
           !sourcemeta::core::URITemplate::is_uritemplate(value.to_string()))) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Self,
            "The value of x-jsonld-self must be a URI Template", *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        if (facts.self.has_value() && facts.self.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else if (!facts.self.has_value()) {
          facts.self = text;
          facts.self_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-value", HASH_VALUE)) {
      if (!value.is_null() && !is_iri_value(value)) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::ValuePredicate,
            "The value of x-jsonld-value must be an absolute IRI", *origin);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        auto &promotion{promotion_facts(facts)};
        if (promotion.value.has_value() && promotion.value.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else if (!promotion.value.has_value()) {
          promotion.value = text;
          promotion.value_origin = origin;
        }
      }
    } else if (keyword.property_equals("x-jsonld-constants", HASH_CONSTANTS)) {
      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else {
        auto canonical{
            canonicalize_constants(value, instance_location, *origin)};
        if (std::holds_alternative<sourcemeta::blaze::JSONLDResolutionError>(
                canonical)) {
          return std::get<sourcemeta::blaze::JSONLDResolutionError>(
              std::move(canonical));
        }

        const auto &fragment{std::get<sourcemeta::core::JSON>(canonical)};

        // Whether a null entry is a licensed tombstone depends on override
        // marks that only the slow path can consult, and an empty fragment
        // asserts nothing, so it must not materialize the promotion facts
        if (fragment_has_null_entry(fragment)) {
          demote(accumulator, dirty, instance_location);
        } else if (!fragment.empty() && !dirty.contains(instance_location)) {
          auto &promotion{promotion_facts(accumulator[instance_location])};
          if (merge_constants_fragment(promotion.constants, fragment)) {
            promotion.constants_origin = origin;
          }
        }
      }
    }
  }

  PendingMap pending;
  if (!dirty.empty()) {
    pending.reserve(dirty.size());
    for (const auto &entry : output.annotations()) {
      if (entry.evaluate_path.empty() ||
          !dirty.contains(entry.instance_location)) {
        continue;
      }

      collect(entry, pending[entry.instance_location]);
    }
  }

  for (auto &[location, entry] : pending) {
    prepare(entry.ids, entry.marks);
    prepare(entry.reverses, entry.marks);
    prepare(entry.types, entry.marks);
    prepare(entry.datatypes, entry.marks);
    prepare(entry.languages, entry.marks);
    prepare(entry.directions, entry.marks);
    prepare(entry.jsons, entry.marks);
    prepare(entry.graphs, entry.marks);
    prepare(entry.containers, entry.marks);
    prepare(entry.selves, entry.marks);
    prepare(entry.values, entry.marks);
    prepare(entry.constants, entry.marks);

    std::erase_if(entry.jsons, false_boolean);
    std::erase_if(entry.graphs, false_boolean);

    if (!entry.annotated()) {
      continue;
    }

    Facts facts;
    resolve_edges(entry.ids, false, facts);
    resolve_edges(entry.reverses, true, facts);
    resolve_types(entry.types, facts);

    const auto datatype{elect(entry.datatypes, key_exact)};
    if (datatype.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::Datatype,
          "A JSON-LD datatype cannot be assigned more than one value",
          datatype);
    } else if (datatype.winner != nullptr) {
      facts.datatype = datatype.winner->value->to_string();
      facts.datatype_origin = datatype.winner->origin;
    }

    const auto language{elect(entry.languages, key_exact)};
    if (language.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::Language,
          "A JSON-LD language cannot be assigned more than one value",
          language);
    } else if (language.winner != nullptr) {
      facts.language = language.winner->value->to_string();
      facts.language_origin = language.winner->origin;
    }

    const auto direction{elect(entry.directions, key_exact)};
    if (direction.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::Direction,
          "A JSON-LD direction cannot be assigned more than one value",
          direction);
    } else if (direction.winner != nullptr) {
      facts.direction = parse_direction(*direction.winner->value);
      facts.direction_origin = direction.winner->origin;
    }

    const auto json{elect(entry.jsons, key_boolean)};
    if (json.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::JSON,
          "A JSON-LD JSON literal flag cannot be assigned more than one "
          "value",
          json);
    } else if (json.winner != nullptr) {
      facts.json = true;
      facts.json_origin = json.winner->origin;
    }

    const auto graph{elect(entry.graphs, key_boolean)};
    if (graph.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::Graph,
          "A JSON-LD graph flag cannot be assigned more than one value", graph);
    } else if (graph.winner != nullptr) {
      facts.graph = true;
      facts.graph_origin = graph.winner->origin;
    }

    const auto container{elect(entry.containers, key_exact)};
    if (container.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::Container,
          "A JSON-LD container cannot be assigned more than one value",
          container);
    } else if (container.winner != nullptr) {
      facts.container = parse_container(*container.winner->value);
      facts.container_origin = container.winner->origin;
    }

    const auto self{elect(entry.selves, key_exact)};
    if (self.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::Self,
          "A JSON-LD self identity cannot be assigned more than one value",
          self);
    } else if (self.winner != nullptr) {
      facts.self = self.winner->value->to_string();
      facts.self_origin = self.winner->origin;
    }

    const auto value_predicate{elect(entry.values, key_exact)};
    if (value_predicate.conflict) {
      return conflict_error(
          location, sourcemeta::blaze::JSONLDFacet::ValuePredicate,
          "A JSON-LD value predicate cannot be assigned more than one value",
          value_predicate);
    } else if (value_predicate.winner != nullptr) {
      auto &promotion{promotion_facts(facts)};
      promotion.value = value_predicate.winner->value->to_string();
      promotion.value_origin = value_predicate.winner->origin;
    }

    if (auto error{resolve_constants(location, entry.constants, facts)};
        error.has_value()) {
      return std::move(error).value();
    }

    accumulator.emplace(location, std::move(facts));
  }

  // A language container materializes its members directly as language-tagged
  // strings, never consulting their descriptors, so a member cannot carry a
  // JSON-LD annotation of its own
  for (const auto &[container_location, container_facts] : accumulator) {
    if (container_facts.container !=
        sourcemeta::core::JSONLDContainer::Language) {
      continue;
    }

    const auto &members{sourcemeta::core::get(instance, container_location)};
    if (!members.is_object()) {
      continue;
    }

    for (const auto &entry : members.as_object()) {
      auto member{container_location};
      member.push_back(std::cref(entry.first));
      const auto member_facts{accumulator.find(member)};
      if (member_facts != accumulator.cend()) {
        const auto &offender{member_facts->second};
        // The null gate applies to members too: a null member is treated as
        // absent, so its annotations declare nothing, except a JSON literal,
        // where the null is the data itself
        if (entry.second.is_null() && !offender.json) {
          continue;
        }

        return facet_error(
            member, sourcemeta::blaze::JSONLDFacet::Container,
            "A JSON-LD language container member cannot carry a "
            "JSON-LD annotation",
            *container_facts.container_origin,
            first_origin({offender.edges_origin, offender.types_origin,
                          offender.datatype_origin, offender.language_origin,
                          offender.direction_origin, offender.container_origin,
                          offender.self_origin, offender.json_origin,
                          offender.graph_origin,
                          promotion_value_origin(offender),
                          promotion_constants_origin(offender)}));
      }
    }
  }

  std::optional<ConsentIndex> consent_index;
  sourcemeta::core::JSONLDWeakAnnotationList annotations;
  annotations.reserve(accumulator.size());
  for (auto &[pointer, facts] : accumulator) {
    const auto &value{sourcemeta::core::get(instance, pointer)};

    // A null value is treated as if its entry were absent, and an absent
    // entry has no node and no annotations, so every fact at the location
    // drops before any placement check, except a JSON literal, where the
    // null is the data itself
    if (value.is_null() && !facts.json) {
      continue;
    }

    std::ranges::sort(facts.edges,
                      [](const sourcemeta::core::JSONLDEdge &left,
                         const sourcemeta::core::JSONLDEdge &right) -> bool {
                        return std::tie(left.predicate, left.reverse) <
                               std::tie(right.predicate, right.reverse);
                      });
    std::ranges::sort(facts.types);

    // A container sets the collection shape and stands alone, excluding every
    // other fact and choosing the value shape it ranges over
    if (facts.container.has_value()) {
      if (!facts.types.empty() || facts.graph || facts.datatype.has_value() ||
          facts.language.has_value() || facts.direction.has_value() ||
          facts.json || facts.self.has_value() || promoted(facts) ||
          with_constants(facts)) {
        return facet_error(
            pointer, sourcemeta::blaze::JSONLDFacet::Container,
            "A JSON-LD container can only be combined with "
            "predicate annotations",
            *facts.container_origin,
            first_origin({facts.types_origin, facts.graph_origin,
                          facts.datatype_origin, facts.language_origin,
                          facts.direction_origin, facts.json_origin,
                          facts.self_origin, promotion_value_origin(facts),
                          promotion_constants_origin(facts)}));
      }

      if (const auto error{
              container_placement_error(pointer, facts.container.value(), value,
                                        *facts.container_origin)};
          error.has_value()) {
        return error.value();
      }
    }

    // A JSON literal preserves any value verbatim, so it stands alone and
    // excludes every other fact
    if (facts.json &&
        (!facts.types.empty() || facts.graph || facts.datatype.has_value() ||
         facts.language.has_value() || facts.direction.has_value() ||
         facts.self.has_value() || promoted(facts) || with_constants(facts))) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::JSON,
          "A JSON-LD JSON literal can only be combined with "
          "predicate annotations",
          *facts.json_origin,
          first_origin({facts.types_origin, facts.graph_origin,
                        facts.datatype_origin, facts.language_origin,
                        facts.direction_origin, facts.self_origin,
                        promotion_value_origin(facts),
                        promotion_constants_origin(facts)}));
    }

    // A value predicate promotes its scalar into a node that carries the
    // scalar under that predicate, so it needs a scalar to carry, its graph
    // pairing is reserved, and reshaping another facet of the location
    // requires the consent of shared authorship or an overriding enclosure
    if (promoted(facts)) {
      const auto &promotion{*facts.promotion};
      if (facts.graph) {
        return facet_error(pointer,
                           sourcemeta::blaze::JSONLDFacet::ValuePredicate,
                           "A JSON-LD value predicate cannot be combined with "
                           "a graph flag",
                           *promotion.value_origin, *facts.graph_origin);
      }

      if (value.is_object() || value.is_array()) {
        return facet_error(pointer,
                           sourcemeta::blaze::JSONLDFacet::ValuePredicate,
                           "A JSON-LD value predicate can only be assigned to "
                           "a scalar value",
                           *promotion.value_origin);
      }

      if (facts.self.has_value() || facts.datatype.has_value() ||
          facts.language.has_value() || facts.direction.has_value()) {
        if (!consent_index.has_value()) {
          consent_index = build_consent_index(output);
        }

        const auto location_annotations{consent_index->find(pointer)};
        assert(location_annotations != consent_index->cend());
        const auto &nearby{location_annotations->second};
        const auto consent{consent_paths_at(nearby, promotion.value.value())};

        if (facts.self.has_value() &&
            !facet_retargeting_consented(
                nearby, consent, ConsentKind::Self,
                sourcemeta::core::JSON{facts.self.value()})) {
          return facet_error(pointer,
                             sourcemeta::blaze::JSONLDFacet::ValuePredicate,
                             "A JSON-LD value predicate cannot fuse with a "
                             "self identity from an unrelated schema object",
                             *promotion.value_origin, *facts.self_origin);
        }

        if (facts.datatype.has_value() &&
            !facet_retargeting_consented(
                nearby, consent, ConsentKind::Datatype,
                sourcemeta::core::JSON{facts.datatype.value()})) {
          return facet_error(pointer,
                             sourcemeta::blaze::JSONLDFacet::ValuePredicate,
                             "A JSON-LD value predicate cannot adopt a "
                             "datatype from an unrelated schema object",
                             *promotion.value_origin, *facts.datatype_origin);
        }

        if (facts.language.has_value() &&
            !facet_retargeting_consented(
                nearby, consent, ConsentKind::Language,
                sourcemeta::core::JSON{facts.language.value()})) {
          return facet_error(pointer,
                             sourcemeta::blaze::JSONLDFacet::ValuePredicate,
                             "A JSON-LD value predicate cannot adopt a "
                             "language from an unrelated schema object",
                             *promotion.value_origin, *facts.language_origin);
        }

        if (facts.direction.has_value() &&
            !facet_retargeting_consented(nearby, consent,
                                         ConsentKind::Direction,
                                         sourcemeta::core::JSON{direction_text(
                                             facts.direction.value())})) {
          return facet_error(pointer,
                             sourcemeta::blaze::JSONLDFacet::ValuePredicate,
                             "A JSON-LD value predicate cannot adopt a "
                             "direction from an unrelated schema object",
                             *promotion.value_origin, *facts.direction_origin);
        }
      }
    }

    // A constants fragment merges into a node, so its location must
    // materialize as one
    if (with_constants(facts) && !value.is_object() && !promoted(facts) &&
        !facts.self.has_value()) {
      return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Constants,
                         "A JSON-LD constants fragment requires an object "
                         "value, a value predicate, or a self identity",
                         *facts.promotion->constants_origin);
    }

    // A self identity mints an @id, promoting a scalar to a reference and
    // giving an object its identifier. It describes a node, so it excludes the
    // literal facets and cannot apply to an array collection
    if (facts.self.has_value()) {
      if (!promoted(facts) &&
          (facts.datatype.has_value() || facts.language.has_value() ||
           facts.direction.has_value())) {
        return facet_error(
            pointer, sourcemeta::blaze::JSONLDFacet::Self,
            "A JSON-LD self identity cannot carry a datatype, "
            "language, or direction",
            *facts.self_origin,
            first_origin({facts.datatype_origin, facts.language_origin,
                          facts.direction_origin}));
      }

      if (value.is_array()) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Self,
                           "A JSON-LD self identity can only be assigned to an "
                           "object or scalar value",
                           *facts.self_origin);
      }
    }

    if (const auto error{placement_error(pointer, facts, value)};
        error.has_value()) {
      return error.value();
    }

    if (const auto error{literal_error(pointer, facts, value)};
        error.has_value()) {
      return error.value();
    }

    // A predicate attaches to the parent node of an object property. The
    // document root has no parent and an array element inherits the enclosing
    // edge, so neither can carry one
    if (!facts.edges.empty() &&
        (pointer.empty() || !pointer.back().is_property())) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Predicate,
          pointer.empty()
              ? "A JSON-LD predicate cannot be assigned to the document root"
              : "A JSON-LD predicate cannot be assigned to an array element",
          *facts.edges_origin);
    }

    // A container member belongs to a collection, not a node, so it has no
    // parent to attach a predicate to
    if (!facts.edges.empty() && !pointer.empty()) {
      auto parent{pointer};
      parent.pop_back();
      const auto parent_facts{accumulator.find(parent)};
      if (parent_facts != accumulator.cend() &&
          parent_facts->second.container.has_value()) {
        return facet_error(
            pointer, sourcemeta::blaze::JSONLDFacet::Predicate,
            "A JSON-LD predicate cannot be assigned to a container member",
            *facts.edges_origin, *parent_facts->second.container_origin);
      }
    }

    // A reverse predicate makes its value the subject, so the value must be a
    // node or an array of nodes. A literal cannot be a subject
    if (std::ranges::any_of(
            facts.edges, [](const sourcemeta::core::JSONLDEdge &edge) -> bool {
              return edge.reverse;
            })) {
      const bool points_to_node{
          !facts.json && !facts.container.has_value() &&
          (value.is_object() || facts.self.has_value() || promoted(facts) ||
           (value.is_array() && array_of_nodes(accumulator, pointer, value)))};
      if (!points_to_node) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Predicate,
                           "A JSON-LD reverse predicate can only point to a "
                           "node or an array of nodes",
                           *facts.reverse_origin);
      }
    }

    std::optional<sourcemeta::core::JSON::String> identifier;
    if (facts.self.has_value()) {
      auto expanded{
          expand_self(pointer, facts.self.value(), value, *facts.self_origin)};
      if (std::holds_alternative<sourcemeta::blaze::JSONLDResolutionError>(
              expanded)) {
        return std::get<sourcemeta::blaze::JSONLDResolutionError>(
            std::move(expanded));
      }

      identifier =
          std::get<sourcemeta::core::JSON::String>(std::move(expanded));
    }

    sourcemeta::core::JSONLDDescriptor descriptor;
    descriptor.edges = std::move(facts.edges);
    if (facts.json) {
      descriptor.value = sourcemeta::core::JSONLDLiteral{.json = true};
    } else if (facts.container.has_value()) {
      descriptor.value = sourcemeta::core::JSONLDCollection{
          .container = facts.container.value()};
    } else if (promoted(facts)) {
      descriptor.value = sourcemeta::core::JSONLDPromotion{
          .id = std::move(identifier),
          .types = std::move(facts.types),
          .value = std::move(facts.promotion->value).value(),
          .literal =
              sourcemeta::core::JSONLDLiteral{
                  .datatype = std::move(facts.datatype),
                  .language = std::move(facts.language),
                  .direction = facts.direction,
                  .json = false},
          .constants = take_constants(facts)};
    } else if (value.is_object()) {
      descriptor.value =
          sourcemeta::core::JSONLDNode{.id = std::move(identifier),
                                       .types = std::move(facts.types),
                                       .graph = facts.graph,
                                       .constants = take_constants(facts)};
    } else if (value.is_array()) {
      descriptor.value = sourcemeta::core::JSONLDCollection{};
    } else if (identifier.has_value()) {
      descriptor.value =
          sourcemeta::core::JSONLDReference{.id = std::move(identifier.value()),
                                            .types = std::move(facts.types),
                                            .constants = take_constants(facts)};
    } else {
      descriptor.value =
          sourcemeta::core::JSONLDLiteral{.datatype = std::move(facts.datatype),
                                          .language = std::move(facts.language),
                                          .direction = facts.direction};
    }

    annotations.push_back(
        {.pointer = pointer, .descriptor = std::move(descriptor)});
  }

  return annotations;
}

} // namespace

namespace sourcemeta::blaze {

auto jsonld(Evaluator &evaluator, const Template &schema,
            const sourcemeta::core::JSON &instance) -> JSONLDOutcome {
  SimpleOutput output{instance};
  const auto valid{evaluator.validate(schema, instance, std::ref(output))};
  if (!valid) {
    return std::move(output).release();
  }

  auto resolved{resolve(instance, output)};
  if (std::holds_alternative<JSONLDResolutionError>(resolved)) {
    return std::get<JSONLDResolutionError>(std::move(resolved));
  }

  const auto &annotations{
      std::get<sourcemeta::core::JSONLDWeakAnnotationList>(resolved)};
  return sourcemeta::core::jsonld_materialize(instance, annotations);
}

} // namespace sourcemeta::blaze
