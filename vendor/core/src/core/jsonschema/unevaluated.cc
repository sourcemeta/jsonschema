#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <cassert> // assert
#include <utility> // std::move

namespace {
using namespace sourcemeta::core;

// TODO: Extract all of this into a public utility to traverse
// adjacent subschemas
auto find_adjacent_dependencies(const JSON::String &current, const JSON &schema,
                                const Frame &frame, const SchemaWalker &walker,
                                const SchemaResolver &resolver,
                                const std::set<JSON::String> &keywords,
                                const Frame::LocationsEntry &root,
                                const Frame::LocationsEntry &entry,
                                const bool is_static, UnevaluatedEntry &result)
    -> void {
  const auto &subschema{get(schema, entry.pointer)};
  if (!subschema.is_object()) {
    return;
  }

  const auto subschema_vocabularies{
      vocabularies(subschema, resolver, entry.dialect)};

  for (const auto &property : subschema.as_object()) {
    if (property.first == current && entry.pointer == root.pointer) {
      continue;
    } else if (keywords.contains(property.first)) {
      // In 2019-09, `additionalItems` takes no effect without `items`
      if (subschema_vocabularies.contains(
              "https://json-schema.org/draft/2019-09/vocab/applicator") &&
          property.first == "additionalItems" && !subschema.defines("items")) {
        continue;
      }

      auto pointer{entry.pointer.concat({property.first})};
      if (is_static) {
        result.static_dependencies.emplace(std::move(pointer));
      } else {
        result.dynamic_dependencies.emplace(std::move(pointer));
      }

      continue;
    }

    switch (walker(property.first, subschema_vocabularies).type) {
      // References
      case KeywordType::Reference: {
        const auto reference{frame.dereference(entry, {property.first})};
        if (reference.first == ReferenceType::Static &&
            reference.second.has_value()) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              reference.second.value().get(), is_static, result);
        } else if (reference.first == ReferenceType::Dynamic) {
          result.unresolved = true;
        }

        break;
      }

      // Static
      case KeywordType::ApplicatorElementsInline:
        for (std::size_t index = 0; index < property.second.size(); index++) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, {property.first, index}), is_static,
              result);
        }

        break;

      // Dynamic
      case KeywordType::ApplicatorElementsInPlace:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, {property.first, index}), false, result);
          }
        }

        break;
      case KeywordType::ApplicatorValueInPlace:
        if (is_schema(property.second)) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, {property.first}), false, result);
        }

        break;
      case KeywordType::ApplicatorValueOrElementsInPlace:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, {property.first, index}), false, result);
          }
        } else if (is_schema(property.second)) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, {property.first}), false, result);
        }

        break;
      case KeywordType::ApplicatorMembersInPlace:
        if (property.second.is_object()) {
          for (const auto &pair : property.second.as_object()) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, {property.first, pair.first}), false,
                result);
          }
        }

        break;

      // Anything else does not contribute to the dependency list
      default:
        break;
    }
  }
}

} // namespace

namespace sourcemeta::core {

auto unevaluated(const JSON &schema, const Frame &frame,
                 const SchemaWalker &walker, const SchemaResolver &resolver)
    -> UnevaluatedEntries {
  UnevaluatedEntries result;

  for (const auto &entry : frame.locations()) {
    if (entry.second.type != Frame::LocationType::Subschema &&
        entry.second.type != Frame::LocationType::Resource) {
      continue;
    }

    const auto &subschema{get(schema, entry.second.pointer)};
    assert(is_schema(subschema));
    if (!subschema.is_object()) {
      continue;
    }

    const auto subschema_vocabularies{
        frame.vocabularies(entry.second, resolver)};
    for (const auto &pair : subschema.as_object()) {
      const auto keyword_uri{frame.uri(entry.second, {pair.first})};
      UnevaluatedEntry unevaluated;
      if ((subschema_vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/unevaluated") &&
           subschema_vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/applicator")) &&
          pair.first == "unevaluatedProperties") {
        find_adjacent_dependencies(
            pair.first, schema, frame, walker, resolver,
            {"properties", "patternProperties", "additionalProperties",
             "unevaluatedProperties"},
            entry.second, entry.second, true, unevaluated);
        result.emplace(keyword_uri, std::move(unevaluated));
      } else if (
          (subschema_vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/unevaluated") &&
           subschema_vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/applicator")) &&
          pair.first == "unevaluatedItems") {
        find_adjacent_dependencies(
            pair.first, schema, frame, walker, resolver,
            {"prefixItems", "items", "contains", "unevaluatedItems"},
            entry.second, entry.second, true, unevaluated);
        result.emplace(keyword_uri, std::move(unevaluated));
      } else if (subschema_vocabularies.contains(
                     "https://json-schema.org/draft/2019-09/vocab/"
                     "applicator") &&
                 pair.first == "unevaluatedProperties") {
        find_adjacent_dependencies(
            pair.first, schema, frame, walker, resolver,
            {"properties", "patternProperties", "additionalProperties",
             "unevaluatedProperties"},
            entry.second, entry.second, true, unevaluated);
        result.emplace(keyword_uri, std::move(unevaluated));
      } else if (subschema_vocabularies.contains(
                     "https://json-schema.org/draft/2019-09/vocab/"
                     "applicator") &&
                 pair.first == "unevaluatedItems") {
        find_adjacent_dependencies(
            pair.first, schema, frame, walker, resolver,
            {"items", "additionalItems", "unevaluatedItems"}, entry.second,
            entry.second, true, unevaluated);
        result.emplace(keyword_uri, std::move(unevaluated));
      }
    }
  }

  return result;
}

} // namespace sourcemeta::core
