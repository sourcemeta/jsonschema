#include <sourcemeta/blaze/output_jsonld.h>

#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/langtag.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/uritemplate.h>

#include <algorithm>     // std::ranges::sort, std::ranges::any_of
#include <deque>         // std::deque
#include <functional>    // std::ref
#include <optional>      // std::optional
#include <sstream>       // std::ostringstream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tie, std::make_tuple
#include <unordered_map> // std::unordered_map
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

auto fill_collection(sourcemeta::core::JSONLDWeakAnnotationMap &map,
                     const Accumulator &accumulator,
                     const sourcemeta::core::WeakPointer &pointer,
                     const sourcemeta::core::JSON &value) -> void;

// Give an undescribed collection member a default kind so it still
// materializes, a scalar as a literal and a nested array as an inner collection
auto fill_element(sourcemeta::core::JSONLDWeakAnnotationMap &map,
                  const Accumulator &accumulator,
                  sourcemeta::core::WeakPointer element_pointer,
                  const sourcemeta::core::JSON &element) -> void {
  if (accumulator.contains(element_pointer)) {
    return;
  }

  if (element.is_array()) {
    map.emplace(
        element_pointer,
        sourcemeta::core::JSONLDDescriptor{
            .edges = {}, .value = sourcemeta::core::JSONLDCollection{}});
    fill_collection(map, accumulator, element_pointer, element);
  } else if (!element.is_object()) {
    map.emplace(std::move(element_pointer),
                sourcemeta::core::JSONLDDescriptor{
                    .edges = {}, .value = sourcemeta::core::JSONLDLiteral{}});
  }
}

// Default the undescribed members of a collection, whether it ranges over an
// array or over an object
auto fill_collection(sourcemeta::core::JSONLDWeakAnnotationMap &map,
                     const Accumulator &accumulator,
                     const sourcemeta::core::WeakPointer &pointer,
                     const sourcemeta::core::JSON &value) -> void {
  if (value.is_array()) {
    for (std::size_t index = 0; index < value.size(); index += 1) {
      auto element_pointer{pointer};
      element_pointer.push_back(index);
      fill_element(map, accumulator, std::move(element_pointer),
                   value.at(index));
    }
  } else {
    for (const auto &entry : value.as_object()) {
      auto element_pointer{pointer};
      element_pointer.push_back(std::cref(entry.first));
      fill_element(map, accumulator, std::move(element_pointer), entry.second);
    }
  }
}

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
          !sourcemeta::core::is_langtag(entry.first)) {
        return facet_error(
            pointer, sourcemeta::blaze::JSONLDFacet::Container,
            "A JSON-LD language container requires BCP 47 language tag keys");
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
// reserved variable this to its own value. A string binds directly and any
// other scalar binds through its JSON stringification, whereas an object,
// array, absent, or null value cannot bind. A binding that is not usable, or a
// result that is not an absolute IRI, is a fail-loud resolution error
auto expand_self(const sourcemeta::core::WeakPointer &pointer,
                 const sourcemeta::core::JSON::String &pattern,
                 const sourcemeta::core::JSON &value)
    -> std::variant<sourcemeta::core::JSON::String,
                    sourcemeta::blaze::JSONLDResolutionError> {
  std::optional<sourcemeta::blaze::JSONLDResolutionError> failure;
  std::deque<std::string> stringified;
  const sourcemeta::core::URITemplate uri_template{pattern};
  auto expanded{uri_template.expand(
      [&value, &pointer, &failure, &stringified](
          const std::string_view name) -> sourcemeta::core::URITemplateValue {
        const sourcemeta::core::JSON *bound{nullptr};
        if (value.is_object()) {
          bound = value.try_at(name);
        } else if (name == "this") {
          bound = &value;
        }

        if (bound == nullptr || bound->is_object() || bound->is_array() ||
            bound->is_null()) {
          if (!failure.has_value()) {
            failure = facet_error(
                pointer, sourcemeta::blaze::JSONLDFacet::Self,
                "A JSON-LD self identity template variable must bind to a "
                "string, number, or boolean");
          }

          return std::nullopt;
        }

        if (bound->is_string()) {
          if (bound->to_string().empty()) {
            if (!failure.has_value()) {
              failure = facet_error(
                  pointer, sourcemeta::blaze::JSONLDFacet::Self,
                  "A JSON-LD self identity template variable cannot bind to an "
                  "empty string");
            }

            return std::nullopt;
          }

          return std::make_tuple(std::string_view{bound->to_string()},
                                 std::nullopt, false);
        }

        // A number or boolean has no direct string, so it binds through its
        // compact JSON form, kept alive for the duration of the expansion
        std::ostringstream stream;
        sourcemeta::core::stringify(*bound, stream);
        stringified.push_back(stream.str());
        return std::make_tuple(std::string_view{stringified.back()},
                               std::nullopt, false);
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

// Turn the JSON-LD annotations into a resolved map, or the first error found.
// The first pass groups the facts by location, the second derives each kind
// from the value shape and validates the facts against it
auto resolve(const sourcemeta::core::JSON &instance,
             const sourcemeta::blaze::SimpleOutput &output)
    -> std::variant<sourcemeta::core::JSONLDWeakAnnotationMap,
                    sourcemeta::blaze::JSONLDResolutionError> {
  Accumulator accumulator;

  for (const auto &[location, values] : output.annotations()) {
    if (location.evaluate_path.empty()) {
      continue;
    }

    const auto &keyword{location.evaluate_path.back().to_property()};
    const auto &instance_location{location.instance_location};

    if (keyword == "x-jsonld-id" || keyword == "x-jsonld-reverse") {
      const bool reverse{keyword == "x-jsonld-reverse"};
      for (const auto &value : values) {
        if (!is_iri_value(value)) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Predicate,
              reverse ? "The value of x-jsonld-reverse must be an absolute IRI"
                      : "The value of x-jsonld-id must be an absolute IRI");
        }

        add_edge(accumulator[instance_location].edges, value.to_string(),
                 reverse);
      }
    } else if (keyword == "x-jsonld-type") {
      auto &types{accumulator[instance_location].types};
      for (const auto &value : values) {
        if (value.is_array()) {
          for (const auto &element : value.as_array()) {
            if (!is_iri_value(element)) {
              return type_iri_error(instance_location);
            }

            add_type(types, element.to_string());
          }
        } else {
          if (!is_iri_value(value)) {
            return type_iri_error(instance_location);
          }

          add_type(types, value.to_string());
        }
      }
    } else if (keyword == "x-jsonld-datatype") {
      for (const auto &value : values) {
        if (!is_iri_value(value)) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Datatype,
              "The value of x-jsonld-datatype must be an absolute IRI");
        }

        auto &datatype{accumulator[instance_location].datatype};
        if (datatype.has_value() && datatype.value() != value.to_string()) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Datatype,
              "A JSON-LD datatype cannot be assigned more than one value");
        }

        datatype = value.to_string();
      }
    } else if (keyword == "x-jsonld-language") {
      for (const auto &value : values) {
        if (!value.is_string() ||
            !sourcemeta::core::is_langtag(value.to_string())) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Language,
              "The value of x-jsonld-language must be a BCP 47 language tag");
        }

        auto &language{accumulator[instance_location].language};
        if (language.has_value()) {
          // Language tags compare case-insensitively, so the first spelling is
          // kept and only a genuinely different tag is a conflict
          if (!sourcemeta::core::equals_ignore_case(language.value(),
                                                    value.to_string())) {
            return facet_error(
                instance_location, sourcemeta::blaze::JSONLDFacet::Language,
                "A JSON-LD language cannot be assigned more than one value");
          }
        } else {
          language = value.to_string();
        }
      }
    } else if (keyword == "x-jsonld-direction") {
      for (const auto &value : values) {
        const auto parsed{parse_direction(value)};
        if (!parsed.has_value()) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Direction,
              R"(The value of x-jsonld-direction must be "ltr" or "rtl")");
        }

        auto &direction{accumulator[instance_location].direction};
        if (direction.has_value() && direction.value() != parsed.value()) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Direction,
              "A JSON-LD direction cannot be assigned more than one value");
        }

        direction = parsed;
      }
    } else if (keyword == "x-jsonld-json") {
      for (const auto &value : values) {
        if (!value.is_boolean()) {
          return facet_error(instance_location,
                             sourcemeta::blaze::JSONLDFacet::JSON,
                             "The value of x-jsonld-json must be a boolean");
        }

        // A false value leaves the fact unset, so it must stay a no-op and not
        // create an entry
        if (value.to_boolean()) {
          accumulator[instance_location].json = true;
        }
      }
    } else if (keyword == "x-jsonld-graph") {
      for (const auto &value : values) {
        if (!value.is_boolean()) {
          return facet_error(instance_location,
                             sourcemeta::blaze::JSONLDFacet::Graph,
                             "The value of x-jsonld-graph must be a boolean");
        }

        if (value.to_boolean()) {
          accumulator[instance_location].graph = true;
        }
      }
    } else if (keyword == "x-jsonld-container") {
      for (const auto &value : values) {
        const auto parsed{parse_container(value)};
        if (!parsed.has_value()) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Container,
              R"(The value of x-jsonld-container must be "@list", "@set", "@language", or "@index")");
        }

        auto &container{accumulator[instance_location].container};
        if (container.has_value() && container.value() != parsed.value()) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Container,
              "A JSON-LD container cannot be assigned more than one value");
        }

        container = parsed;
      }
    } else if (keyword == "x-jsonld-self") {
      for (const auto &value : values) {
        if (!value.is_string() ||
            !sourcemeta::core::URITemplate::is_uritemplate(value.to_string())) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Self,
              "The value of x-jsonld-self must be a URI Template");
        }

        auto &self{accumulator[instance_location].self};
        if (self.has_value() && self.value() != value.to_string()) {
          return facet_error(
              instance_location, sourcemeta::blaze::JSONLDFacet::Self,
              "A JSON-LD self identity cannot be assigned more than one value");
        }

        self = value.to_string();
      }
    }
  }

  sourcemeta::core::JSONLDWeakAnnotationMap map;
  map.reserve(accumulator.size());
  for (auto &[pointer, facts] : accumulator) {
    std::ranges::sort(facts.edges,
                      [](const sourcemeta::core::JSONLDEdge &left,
                         const sourcemeta::core::JSONLDEdge &right) -> bool {
                        return std::tie(left.predicate, left.reverse) <
                               std::tie(right.predicate, right.reverse);
                      });
    std::ranges::sort(facts.types);

    const auto &value{sourcemeta::core::get(instance, pointer)};

    // A language container materializes its members directly as language-tagged
    // strings, never consulting their descriptors, so a member cannot carry a
    // JSON-LD annotation of its own
    if (!pointer.empty()) {
      auto parent{pointer};
      parent.pop_back();
      const auto parent_facts{accumulator.find(parent)};
      if (parent_facts != accumulator.cend() &&
          parent_facts->second.container ==
              sourcemeta::core::JSONLDContainer::Language) {
        return facet_error(pointer, sourcemeta::blaze::JSONLDFacet::Container,
                           "A JSON-LD language container member cannot carry a "
                           "JSON-LD annotation");
      }
    }

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

    map.emplace(pointer, std::move(descriptor));

    // A language container reads its object members directly, but every other
    // collection needs its undescribed members defaulted so they materialize
    if (facts.container.has_value()) {
      if (facts.container.value() !=
          sourcemeta::core::JSONLDContainer::Language) {
        fill_collection(map, accumulator, pointer, value);
      }
    } else if (value.is_array() && !facts.json) {
      fill_collection(map, accumulator, pointer, value);
    }
  }

  return map;
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

  const auto &map{
      std::get<sourcemeta::core::JSONLDWeakAnnotationMap>(resolved)};
  return sourcemeta::core::jsonld_materialize(instance, map);
}

} // namespace sourcemeta::blaze
