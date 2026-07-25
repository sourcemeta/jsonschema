#include <sourcemeta/blaze/output_jsonld.h>

#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/langtag.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/uritemplate.h>

#include <algorithm>     // std::ranges::sort, std::ranges::any_of
#include <cassert>       // assert
#include <functional>    // std::ref, std::reference_wrapper
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tie, std::make_tuple
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <variant>       // std::variant, std::get, std::holds_alternative
#include <vector>        // std::vector

namespace {

// The facts gathered at one instance location before its kind is known
struct Facts {
  std::vector<sourcemeta::core::JSONLDEdge> edges;
  std::vector<sourcemeta::core::JSON::String> types;
  std::optional<sourcemeta::core::JSON::String> datatype;
  std::optional<sourcemeta::core::JSON::String> language;
  std::optional<sourcemeta::core::JSONLDDirection> direction;
  std::optional<sourcemeta::core::JSONLDContainer> container;
  std::optional<sourcemeta::core::JSON::String> self;
  bool json{false};
  bool graph{false};
};

using Accumulator = std::unordered_map<sourcemeta::core::WeakPointer, Facts,
                                       sourcemeta::core::WeakPointer::Hasher>;

auto add_edge(std::vector<sourcemeta::core::JSONLDEdge> &edges,
              const sourcemeta::core::JSON::String &predicate,
              const bool reverse) -> void {
  const auto exists{std::ranges::any_of(
      edges,
      [&predicate, reverse](const sourcemeta::core::JSONLDEdge &edge) -> bool {
        return edge.predicate == predicate && edge.reverse == reverse;
      })};
  if (!exists) {
    edges.push_back({.predicate = predicate, .reverse = reverse});
  }
}

auto add_type(std::vector<sourcemeta::core::JSON::String> &types,
              const sourcemeta::core::JSON::String &type) -> void {
  const auto exists{std::ranges::any_of(
      types, [&type](const sourcemeta::core::JSON::String &existing) -> bool {
        return existing == type;
      })};
  if (!exists) {
    types.push_back(type);
  }
}

// Whether an annotation value is usable as an absolute IRI
auto is_iri_value(const sourcemeta::core::JSON &value) -> bool {
  return value.is_string() && sourcemeta::core::URI::is_iri(value.to_string());
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

auto type_iri_error(const sourcemeta::core::WeakPointer &instance_location)
    -> sourcemeta::blaze::JSONLDResolutionError {
  return {.instance_location = sourcemeta::core::to_pointer(instance_location),
          .facet = sourcemeta::blaze::JSONLDFacet::Type,
          .message = "The value of x-jsonld-type must be an absolute IRI"};
}

auto facet_error(const sourcemeta::core::WeakPointer &instance_location,
                 const sourcemeta::blaze::JSONLDFacet facet,
                 std::string message)
    -> sourcemeta::blaze::JSONLDResolutionError {
  return {.instance_location = sourcemeta::core::to_pointer(instance_location),
          .facet = facet,
          .message = std::move(message)};
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
    const sourcemeta::core::JSON &value)
    -> std::optional<sourcemeta::blaze::JSONLDResolutionError> {
  if (container == sourcemeta::core::JSONLDContainer::List ||
      container == sourcemeta::core::JSONLDContainer::Set) {
    if (!value.is_array()) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Container,
          "A JSON-LD list or set container can only be assigned to an array "
          "value");
    }

    return std::nullopt;
  }

  if (!value.is_object()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Container,
        "A JSON-LD language or index container can only be assigned to an "
        "object value");
  }

  if (container == sourcemeta::core::JSONLDContainer::Language) {
    for (const auto &entry : value.as_object()) {
      if (entry.first != "@none" &&
          !sourcemeta::core::is_canonical_langtag(entry.first)) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Container,
                           "A JSON-LD language container requires canonical "
                           "BCP 47 language tag keys");
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
            "A JSON-LD language container requires string or null members");
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
    if (!facts.types.empty() && !facts.self.has_value()) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Type,
          "A JSON-LD type can only be assigned to an object value");
    }

    if (facts.graph) {
      return facet_error(
          pointer, sourcemeta::blaze::JSONLDFacet::Graph,
          "A JSON-LD graph flag can only be assigned to an object value");
    }
  }

  if (!value.is_object() && !value.is_array()) {
    return std::nullopt;
  }

  if (facts.datatype.has_value()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Datatype,
        "A JSON-LD datatype can only be assigned to a scalar value");
  }

  if (facts.language.has_value()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Language,
        "A JSON-LD language can only be assigned to a scalar value");
  }

  if (facts.direction.has_value()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Direction,
        "A JSON-LD direction can only be assigned to a scalar value");
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
        "A JSON-LD datatype cannot carry a language or direction");
  }

  if (facts.language.has_value() && !value.is_string()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Language,
        "A JSON-LD language can only be assigned to a string value");
  }

  if (facts.direction.has_value() && !value.is_string()) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Direction,
        "A JSON-LD direction can only be assigned to a string value");
  }

  return std::nullopt;
}

// Expand an x-jsonld-self URI Template into a concrete identifier. An object
// binds each variable to the member of that name, and a scalar binds the
// reserved variable this to its own value. Only a non-empty string can bind,
// and a binding that is not usable, or a result that is not an absolute IRI,
// is a fail-loud resolution error
auto expand_self(const sourcemeta::core::WeakPointer &pointer,
                 const sourcemeta::core::JSON::String &pattern,
                 const sourcemeta::core::JSON &value)
    -> std::variant<sourcemeta::core::JSON::String,
                    sourcemeta::blaze::JSONLDResolutionError> {
  std::optional<sourcemeta::blaze::JSONLDResolutionError> failure;
  const sourcemeta::core::URITemplate uri_template{pattern};
  auto expanded{uri_template.expand(
      [&value, &pointer, &failure](
          const std::string_view name) -> sourcemeta::core::URITemplateValue {
        const sourcemeta::core::JSON *bound{nullptr};
        if (value.is_object()) {
          bound = value.try_at(name);
        } else if (name == "this") {
          bound = &value;
        }

        if (bound == nullptr || !bound->is_string()) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable must bind to a "
                "string");
          }

          return std::nullopt;
        }

        const auto &text{bound->to_string()};
        if (text.empty()) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable cannot bind to an "
                "empty string");
          }

          return std::nullopt;
        }

        return std::make_tuple(std::string_view{text}, std::nullopt, false);
      })};

  if (failure.has_value()) {
    return failure.value();
  }

  if (!sourcemeta::core::URI::is_iri(expanded)) {
    return facet_error(
        pointer, sourcemeta::blaze::JSONLDFacet::Self,
        "A JSON-LD self identity must expand to an absolute IRI");
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
        !element_facts->second.self.has_value()) {
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

// A collected keyword value pending resolution, paired with the evaluate
// path of the annotation that carried it
struct Candidate {
  const sourcemeta::core::JSON *value;
  const sourcemeta::core::WeakPointer *path;
  bool marked{false};
};

// The candidates gathered at one instance location before resolution, plus
// the evaluate paths of the x-jsonld-override annotations that evaluated to
// true at that location
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
  std::vector<const sourcemeta::core::WeakPointer *> marks;

  [[nodiscard]] auto annotated() const -> bool {
    return !this->ids.empty() || !this->reverses.empty() ||
           !this->types.empty() || !this->datatypes.empty() ||
           !this->languages.empty() || !this->directions.empty() ||
           !this->jsons.empty() || !this->graphs.empty() ||
           !this->containers.empty() || !this->selves.empty();
  }
};

using PendingMap = std::unordered_map<sourcemeta::core::WeakPointer, Pending,
                                      sourcemeta::core::WeakPointer::Hasher>;

// Whether the annotation at the given evaluate path shares its schema object
// with one of its location's x-jsonld-override marks
auto marked_by(const std::vector<const sourcemeta::core::WeakPointer *> &marks,
               const sourcemeta::core::WeakPointer &path) -> bool {
  for (const auto *mark : marks) {
    if (mark->size() == path.size() && path.starts_with_initial(*mark)) {
      return true;
    }
  }

  return false;
}

// A null that no override mark licenses declares nothing, as if the keyword
// were absent
auto unmarked_null(const Candidate &candidate) -> bool {
  return candidate.value->is_null() && !candidate.marked;
}

// Attach the location's override marks to a facet's candidates and drop the
// nulls that no mark licenses. This cannot happen at collection time, as the
// sibling mark may be collected in any order relative to the null
auto prepare(std::vector<Candidate> &candidates,
             const std::vector<const sourcemeta::core::WeakPointer *> &marks)
    -> void {
  for (auto &candidate : candidates) {
    candidate.marked = marked_by(marks, *candidate.path);
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
// state, either because no candidate exists or because a tombstone won
struct Election {
  const Candidate *winner{nullptr};
  bool conflict{false};
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
        if (candidates[other].marked && keys[other] != keys[index] &&
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
      return {.winner = nullptr, .conflict = true};
    }
  }

  if (!elected.has_value()) {
    return {};
  }

  // Every survivor carries the elected key, and equal keys mean equal
  // values, so any survivor serves as the winner
  return {.winner = &candidates[survivors.front()], .conflict = false};
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
                   std::vector<sourcemeta::core::JSONLDEdge> &edges) -> void {
  for (const auto &candidate : candidates) {
    if (candidate.value->is_null() ||
        removed_by_tombstone(candidates, candidate)) {
      continue;
    }

    add_edge(edges, candidate.value->to_string(), reverse);
  }
}

auto resolve_types(const std::vector<Candidate> &candidates,
                   std::vector<sourcemeta::core::JSON::String> &types) -> void {
  for (const auto &candidate : candidates) {
    if (candidate.value->is_null() ||
        removed_by_tombstone(candidates, candidate)) {
      continue;
    }

    if (candidate.value->is_array()) {
      for (const auto &element : candidate.value->as_array()) {
        add_type(types, element.to_string());
      }
    } else {
      add_type(types, candidate.value->to_string());
    }
  }
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

// Gather an already-validated annotation into its location's pending
// candidates for the override-aware slow path
auto collect(const sourcemeta::blaze::SimpleOutput::AnnotationEntry &entry,
             Pending &pending) -> void {
  const auto &keyword{entry.evaluate_path.back()};
  const Candidate candidate{.value = &entry.value,
                            .path = &entry.evaluate_path};

  if (keyword.property_equals("x-jsonld-override", HASH_OVERRIDE)) {
    if (entry.value.to_boolean()) {
      pending.marks.push_back(&entry.evaluate_path);
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

    if (keyword.property_equals("x-jsonld-override", HASH_OVERRIDE)) {
      if (!value.is_boolean()) {
        return facet_error(instance_location,
                           sourcemeta::blaze::JSONLDFacet::Override,
                           "The value of x-jsonld-override must be a boolean");
      }

      // A mark only matters when it shadows a diverging value or licenses a
      // null or a false, and each of those demotes the location on its own,
      // so the slow path recollects the mark whenever it can act. A future
      // keyword whose mark changes agreeing resolutions must demote here
    } else if (keyword.property_equals("x-jsonld-id", HASH_ID) ||
               keyword.property_equals("x-jsonld-reverse", HASH_REVERSE)) {
      const bool reverse{
          keyword.property_equals("x-jsonld-reverse", HASH_REVERSE)};
      if (!value.is_null() && !is_iri_value(value)) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Predicate,
            reverse ? "The value of x-jsonld-reverse must be an absolute IRI"
                    : "The value of x-jsonld-id must be an absolute IRI");
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        add_edge(accumulator[instance_location].edges, value.to_string(),
                 reverse);
      }
    } else if (keyword.property_equals("x-jsonld-type", HASH_TYPE)) {
      if (value.is_array()) {
        for (const auto &element : value.as_array()) {
          if (!is_iri_value(element)) {
            return type_iri_error(instance_location);
          }
        }
      } else if (!value.is_null() && !is_iri_value(value)) {
        return type_iri_error(instance_location);
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &types{accumulator[instance_location].types};
        if (value.is_array()) {
          for (const auto &element : value.as_array()) {
            add_type(types, element.to_string());
          }
        } else {
          add_type(types, value.to_string());
        }
      }
    } else if (keyword.property_equals("x-jsonld-datatype", HASH_DATATYPE)) {
      if (!value.is_null() && !is_iri_value(value)) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Datatype,
            "The value of x-jsonld-datatype must be an absolute IRI");
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        if (facts.datatype.has_value() && facts.datatype.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else {
          facts.datatype = text;
        }
      }
    } else if (keyword.property_equals("x-jsonld-language", HASH_LANGUAGE)) {
      if (!value.is_null() &&
          (!value.is_string() ||
           !sourcemeta::core::is_canonical_langtag(value.to_string()))) {
        return facet_error(instance_location,
                           sourcemeta::blaze::JSONLDFacet::Language,
                           "The value of x-jsonld-language must be a "
                           "canonical BCP 47 language tag");
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        if (facts.language.has_value() && facts.language.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else {
          facts.language = text;
        }
      }
    } else if (keyword.property_equals("x-jsonld-direction", HASH_DIRECTION)) {
      const auto direction{parse_direction(value)};
      if (!value.is_null() && !direction.has_value()) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Direction,
            R"(The value of x-jsonld-direction must be "ltr" or "rtl")");
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (facts.direction.has_value() && facts.direction != direction) {
          demote(accumulator, dirty, instance_location);
        } else {
          facts.direction = direction;
        }
      }
    } else if (keyword.property_equals("x-jsonld-json", HASH_JSON) ||
               keyword.property_equals("x-jsonld-graph", HASH_GRAPH)) {
      const bool graph{keyword.property_equals("x-jsonld-graph", HASH_GRAPH)};
      if (!value.is_null() && !value.is_boolean()) {
        return facet_error(
            instance_location,
            graph ? sourcemeta::blaze::JSONLDFacet::Graph
                  : sourcemeta::blaze::JSONLDFacet::JSON,
            graph ? "The value of x-jsonld-graph must be a boolean"
                  : "The value of x-jsonld-json must be a boolean");
      }

      // A false declares nothing anywhere, as the absent state it restates
      // already is false, so removal only ever spells null
      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (value.to_boolean() && !dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (graph) {
          facts.graph = true;
        } else {
          facts.json = true;
        }
      }
    } else if (keyword.property_equals("x-jsonld-container", HASH_CONTAINER)) {
      const auto container{parse_container(value)};
      if (!value.is_null() && !container.has_value()) {
        return facet_error(
            instance_location, sourcemeta::blaze::JSONLDFacet::Container,
            R"(The value of x-jsonld-container must be "@list", "@set", "@language", or "@index")");
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        if (facts.container.has_value() && facts.container != container) {
          demote(accumulator, dirty, instance_location);
        } else {
          facts.container = container;
        }
      }
    } else if (keyword.property_equals("x-jsonld-self", HASH_SELF)) {
      if (!value.is_null() &&
          (!value.is_string() ||
           !sourcemeta::core::URITemplate::is_uritemplate(value.to_string()))) {
        return facet_error(instance_location,
                           sourcemeta::blaze::JSONLDFacet::Self,
                           "The value of x-jsonld-self must be a URI Template");
      }

      if (value.is_null()) {
        demote(accumulator, dirty, instance_location);
      } else if (!dirty.contains(instance_location)) {
        auto &facts{accumulator[instance_location]};
        const auto &text{value.to_string()};
        if (facts.self.has_value() && facts.self.value() != text) {
          demote(accumulator, dirty, instance_location);
        } else {
          facts.self = text;
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

    std::erase_if(entry.jsons, false_boolean);
    std::erase_if(entry.graphs, false_boolean);

    if (!entry.annotated()) {
      continue;
    }

    Facts facts;
    resolve_edges(entry.ids, false, facts.edges);
    resolve_edges(entry.reverses, true, facts.edges);
    resolve_types(entry.types, facts.types);

    const auto datatype{elect(entry.datatypes, key_exact)};
    if (datatype.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::Datatype,
          "A JSON-LD datatype cannot be assigned more than one value");
    } else if (datatype.winner != nullptr) {
      facts.datatype = datatype.winner->value->to_string();
    }

    const auto language{elect(entry.languages, key_exact)};
    if (language.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::Language,
          "A JSON-LD language cannot be assigned more than one value");
    } else if (language.winner != nullptr) {
      facts.language = language.winner->value->to_string();
    }

    const auto direction{elect(entry.directions, key_exact)};
    if (direction.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::Direction,
          "A JSON-LD direction cannot be assigned more than one value");
    } else if (direction.winner != nullptr) {
      facts.direction = parse_direction(*direction.winner->value);
    }

    const auto json{elect(entry.jsons, key_boolean)};
    if (json.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::JSON,
          "A JSON-LD JSON literal flag cannot be assigned more than one "
          "value");
    } else if (json.winner != nullptr) {
      facts.json = true;
    }

    const auto graph{elect(entry.graphs, key_boolean)};
    if (graph.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::Graph,
          "A JSON-LD graph flag cannot be assigned more than one value");
    } else if (graph.winner != nullptr) {
      facts.graph = true;
    }

    const auto container{elect(entry.containers, key_exact)};
    if (container.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::Container,
          "A JSON-LD container cannot be assigned more than one value");
    } else if (container.winner != nullptr) {
      facts.container = parse_container(*container.winner->value);
    }

    const auto self{elect(entry.selves, key_exact)};
    if (self.conflict) {
      return facet_error(
          location, sourcemeta::blaze::JSONLDFacet::Self,
          "A JSON-LD self identity cannot be assigned more than one value");
    } else if (self.winner != nullptr) {
      facts.self = self.winner->value->to_string();
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
      if (accumulator.contains(member)) {
        return facet_error(member, sourcemeta::blaze::JSONLDFacet::Container,
                           "A JSON-LD language container member cannot carry a "
                           "JSON-LD annotation");
      }
    }
  }

  sourcemeta::core::JSONLDWeakAnnotationList annotations;
  annotations.reserve(accumulator.size());
  for (auto &[pointer, facts] : accumulator) {
    std::ranges::sort(facts.edges,
                      [](const sourcemeta::core::JSONLDEdge &left,
                         const sourcemeta::core::JSONLDEdge &right) -> bool {
                        return std::tie(left.predicate, left.reverse) <
                               std::tie(right.predicate, right.reverse);
                      });
    std::ranges::sort(facts.types);

    const auto &value{sourcemeta::core::get(instance, pointer)};

    // A container sets the collection shape and stands alone, excluding every
    // other fact and choosing the value shape it ranges over
    if (facts.container.has_value()) {
      if (!facts.types.empty() || facts.graph || facts.datatype.has_value() ||
          facts.language.has_value() || facts.direction.has_value() ||
          facts.json || facts.self.has_value()) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Container,
                           "A JSON-LD container cannot be combined with any "
                           "other JSON-LD annotation");
      }

      if (const auto error{container_placement_error(
              pointer, facts.container.value(), value)};
          error.has_value()) {
        return error.value();
      }
    }

    // A JSON literal preserves any value verbatim, so it stands alone and
    // excludes every other fact
    if (facts.json &&
        (!facts.types.empty() || facts.graph || facts.datatype.has_value() ||
         facts.language.has_value() || facts.direction.has_value() ||
         facts.self.has_value())) {
      return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::JSON,
                         "A JSON-LD JSON literal cannot be combined with any "
                         "other JSON-LD annotation");
    }

    // A self identity mints an @id, promoting a scalar to a reference and
    // giving an object its identifier. It describes a node, so it excludes the
    // literal facets and cannot apply to an array collection
    if (facts.self.has_value()) {
      if (facts.datatype.has_value() || facts.language.has_value() ||
          facts.direction.has_value()) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Self,
                           "A JSON-LD self identity cannot carry a datatype, "
                           "language, or direction");
      }

      if (value.is_array()) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Self,
                           "A JSON-LD self identity can only be assigned to an "
                           "object or scalar value");
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
              : "A JSON-LD predicate cannot be assigned to an array element");
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
            "A JSON-LD predicate cannot be assigned to a container member");
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
          (value.is_object() || facts.self.has_value() ||
           (value.is_array() && array_of_nodes(accumulator, pointer, value)))};
      if (!points_to_node) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Predicate,
                           "A JSON-LD reverse predicate can only point to a "
                           "node or an array of nodes");
      }
    }

    std::optional<sourcemeta::core::JSON::String> identifier;
    if (facts.self.has_value()) {
      auto expanded{expand_self(pointer, facts.self.value(), value)};
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
    } else if (value.is_object()) {
      descriptor.value =
          sourcemeta::core::JSONLDNode{.id = std::move(identifier),
                                       .types = std::move(facts.types),
                                       .graph = facts.graph};
    } else if (value.is_array()) {
      descriptor.value = sourcemeta::core::JSONLDCollection{};
    } else if (identifier.has_value()) {
      descriptor.value = sourcemeta::core::JSONLDReference{
          .id = std::move(identifier.value()), .types = std::move(facts.types)};
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
