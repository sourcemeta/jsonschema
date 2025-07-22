#include <sourcemeta/blaze/compiler.h>

namespace {
using namespace sourcemeta::core;

auto find_adjacent_dependencies(
    const JSON::String &current, const JSON &schema, const SchemaFrame &frame,
    const SchemaWalker &walker, const SchemaResolver &resolver,
    const std::set<JSON::String> &keywords, const SchemaFrame::Location &root,
    const SchemaFrame::Location &entry, const bool is_static,
    sourcemeta::blaze::SchemaUnevaluatedEntry &result) -> void {
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
      case SchemaKeywordType::Reference: {
        const auto reference{frame.dereference(entry, {property.first})};
        if (reference.first == SchemaReferenceType::Static &&
            reference.second.has_value()) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              reference.second.value().get(), is_static, result);
        } else if (reference.first == SchemaReferenceType::Dynamic) {
          result.unresolved = true;
        }

        break;
      }

      // Static
      case SchemaKeywordType::ApplicatorElementsInPlace:
        for (std::size_t index = 0; index < property.second.size(); index++) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, {property.first, index}), is_static,
              result);
        }

        break;

      // Dynamic
      case SchemaKeywordType::ApplicatorElementsInPlaceSome:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, {property.first, index}), false, result);
          }
        }

        break;
      case SchemaKeywordType::ApplicatorValueTraverseAnyItem:
        [[fallthrough]];
      case SchemaKeywordType::ApplicatorValueTraverseParent:
        [[fallthrough]];
      case SchemaKeywordType::ApplicatorValueInPlaceMaybe:
        if (is_schema(property.second)) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, {property.first}), false, result);
        }

        break;
      case SchemaKeywordType::ApplicatorValueOrElementsInPlace:
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
      case SchemaKeywordType::ApplicatorMembersInPlaceSome:
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

namespace sourcemeta::blaze {

// TODO: Refactor this entire function using `SchemaFrame`'s new `Instances`
// mode. We can loop over every subschema that defines `unevaluatedProperties`
// or `unevaluatedItems`, find all other subschemas with the same unresolved
// instance location (static dependency) or conditional equivalent unresolved
// instance location (dynamic dependency) and see if those ones define any of
// the dependent keywords.
auto unevaluated(const JSON &schema, const SchemaFrame &frame,
                 const SchemaWalker &walker, const SchemaResolver &resolver)
    -> SchemaUnevaluatedEntries {
  SchemaUnevaluatedEntries result;

  for (const auto &entry : frame.locations()) {
    if (entry.second.type != SchemaFrame::LocationType::Subschema &&
        entry.second.type != SchemaFrame::LocationType::Resource) {
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
      SchemaUnevaluatedEntry unevaluated;

      if ((subschema_vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/unevaluated") &&
           subschema_vocabularies.contains(
               "https://json-schema.org/draft/2020-12/vocab/applicator")) &&
          // NOLINTNEXTLINE(bugprone-branch-clone)
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

} // namespace sourcemeta::blaze
