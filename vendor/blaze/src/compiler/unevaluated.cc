#include <sourcemeta/blaze/compiler.h>

#include "compile_helpers.h"

namespace {
using namespace sourcemeta::core;
using namespace sourcemeta::blaze;
using Known = Vocabularies::Known;

static const std::string UNEVALUATED_PROPERTIES{"unevaluatedProperties"};
static const std::string UNEVALUATED_ITEMS{"unevaluatedItems"};

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
              Known::JSON_Schema_2019_09_Applicator) &&
          property.first == "additionalItems" && !subschema.defines("items")) {
        continue;
      }

      auto pointer{entry.pointer.concat(make_weak_pointer(property.first))};
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
        const auto reference{
            frame.dereference(entry, make_weak_pointer(property.first))};
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
              frame.traverse(entry, make_weak_pointer(property.first, index)),
              is_static, result);
        }

        break;

      // Dynamic
      case SchemaKeywordType::ApplicatorElementsInPlaceSome:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, make_weak_pointer(property.first, index)),
                false, result);
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
              frame.traverse(entry, make_weak_pointer(property.first)), false,
              result);
        }

        break;
      case SchemaKeywordType::ApplicatorValueOrElementsInPlace:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, make_weak_pointer(property.first, index)),
                false, result);
          }
        } else if (is_schema(property.second)) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, make_weak_pointer(property.first)), false,
              result);
        }

        break;
      case SchemaKeywordType::ApplicatorMembersInPlaceSome:
        if (property.second.is_object()) {
          for (const auto &pair : property.second.as_object()) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry,
                               make_weak_pointer(property.first, pair.first)),
                false, result);
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

    const bool has_unevaluated_properties{
        subschema.defines("unevaluatedProperties")};
    const bool has_unevaluated_items{subschema.defines("unevaluatedItems")};
    if (!has_unevaluated_properties && !has_unevaluated_items) {
      continue;
    }

    const auto subschema_vocabularies{
        frame.vocabularies(entry.second, resolver)};

    if (has_unevaluated_properties) {
      if ((subschema_vocabularies.contains(
               Known::JSON_Schema_2020_12_Unevaluated) &&
           subschema_vocabularies.contains(
               Known::JSON_Schema_2020_12_Applicator)) ||
          subschema_vocabularies.contains(
              Known::JSON_Schema_2019_09_Applicator)) {
        SchemaUnevaluatedEntry unevaluated;
        find_adjacent_dependencies(
            "unevaluatedProperties", schema, frame, walker, resolver,
            {"properties", "patternProperties", "additionalProperties",
             "unevaluatedProperties"},
            entry.second, entry.second, true, unevaluated);
        result.emplace(
            frame.uri(entry.second, make_weak_pointer(UNEVALUATED_PROPERTIES)),
            std::move(unevaluated));
      }
    }

    if (has_unevaluated_items) {
      SchemaUnevaluatedEntry unevaluated;
      if (subschema_vocabularies.contains(
              Known::JSON_Schema_2020_12_Unevaluated) &&
          subschema_vocabularies.contains(
              Known::JSON_Schema_2020_12_Applicator)) {
        find_adjacent_dependencies(
            "unevaluatedItems", schema, frame, walker, resolver,
            {"prefixItems", "items", "contains", "unevaluatedItems"},
            entry.second, entry.second, true, unevaluated);
        result.emplace(
            frame.uri(entry.second, make_weak_pointer(UNEVALUATED_ITEMS)),
            std::move(unevaluated));
      } else if (subschema_vocabularies.contains(
                     Known::JSON_Schema_2019_09_Applicator)) {
        find_adjacent_dependencies(
            "unevaluatedItems", schema, frame, walker, resolver,
            {"items", "additionalItems", "unevaluatedItems"}, entry.second,
            entry.second, true, unevaluated);
        result.emplace(
            frame.uri(entry.second, make_weak_pointer(UNEVALUATED_ITEMS)),
            std::move(unevaluated));
      }
    }
  }

  return result;
}

} // namespace sourcemeta::blaze
