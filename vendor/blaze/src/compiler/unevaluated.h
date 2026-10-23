#ifndef SOURCEMETA_BLAZE_COMPILER_UNEVALUATED_H_
#define SOURCEMETA_BLAZE_COMPILER_UNEVALUATED_H_

#include <sourcemeta/blaze/compiler.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <set>         // std::set
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair

#include "compile_helpers.h"

namespace sourcemeta::blaze {

// NOLINTBEGIN(cert-err58-cpp,bugprone-throwing-static-initialization)
static const sourcemeta::core::JSON::String UNEVALUATED_PROPERTIES{
    "unevaluatedProperties"};
static const sourcemeta::core::JSON::String UNEVALUATED_ITEMS{
    "unevaluatedItems"};
// NOLINTEND(cert-err58-cpp,bugprone-throwing-static-initialization)

// Whether a location is already being visited has to survive the nested calls
// that walk through it, and stop applying the moment they unwind
class PathGuard {
public:
  PathGuard(std::set<std::pair<sourcemeta::core::WeakPointer, bool>> &visited,
            const sourcemeta::core::WeakPointer &pointer, const bool is_static)
      : visited_{visited}, entry_{pointer, is_static},
        first_visit_{visited.insert(this->entry_).second} {}

  ~PathGuard() {
    if (this->first_visit_) {
      this->visited_.erase(this->entry_);
    }
  }

  [[nodiscard]] auto first_visit() const -> bool { return this->first_visit_; }

  PathGuard(const PathGuard &) = delete;
  auto operator=(const PathGuard &) -> PathGuard & = delete;
  PathGuard(PathGuard &&) = delete;
  auto operator=(PathGuard &&) -> PathGuard & = delete;

private:
  std::set<std::pair<sourcemeta::core::WeakPointer, bool>> &visited_;
  const std::pair<sourcemeta::core::WeakPointer, bool> entry_;
  const bool first_visit_;
};

inline auto find_adjacent_dependencies(
    const sourcemeta::core::JSON::String &current,
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::set<sourcemeta::core::JSON::String> &keywords,
    const sourcemeta::core::SchemaFrame::Location &root,
    const sourcemeta::core::SchemaFrame::Location &entry, const bool is_static,
    std::set<std::pair<sourcemeta::core::WeakPointer, bool>> &visited,
    SchemaUnevaluatedEntry &result) -> void {
  // A schema may reference itself, directly or through a chain of in-place
  // applicators. Following such a cycle forever exhausts the stack, so we cut
  // it as soon as we meet a location already on the path we came in through.
  // The mark only lives as long as that path, as two references to a common
  // target are not a cycle, and each of them still has to be accounted for
  const PathGuard guard{visited, entry.pointer, is_static};
  if (!guard.first_visit()) {
    return;
  }

  const auto &subschema{sourcemeta::core::get(schema, entry.pointer)};
  if (!subschema.is_object()) {
    return;
  }

  const auto &subschema_vocabularies{frame.vocabularies(entry, resolver)};

  for (const auto &property : subschema.as_object()) {
    if (property.first == current && entry.pointer == root.pointer) {
      continue;
    }
    if (keywords.contains(property.first)) {
      // In 2019-09, `additionalItems` takes no effect without `items`
      if (subschema_vocabularies.contains(
              sourcemeta::core::SchemaVocabularies::Known::
                  JSON_SCHEMA_2019_09_APPLICATOR) &&
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
      case sourcemeta::core::SchemaKeywordType::Reference: {
        const auto reference{
            frame.dereference(entry, make_weak_pointer(property.first))};
        if (reference.first == sourcemeta::core::SchemaReferenceType::Static &&
            reference.second.has_value()) {
          // Recurse into a dedicated entry so that whether this reference's
          // target contributes any dynamic dependency can be read directly,
          // rather than inferred from whether it grew the shared deduplicated
          // set, which misses every reference after the first to a common
          // target
          SchemaUnevaluatedEntry nested;
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              reference.second.value().get(), is_static, visited, nested);

          // Whatever the target contributes gets recorded at the location of
          // the target itself, which tells the applicators this reference sits
          // under nothing about it. Record the reference as a dependency of
          // its own too, so that they can still tell that reaching through it
          // leads to one, and therefore that they cannot short-circuit. Only
          // the dynamic dependencies are consulted that way, whereas the
          // static ones name the keyword locations that evaluate, which a
          // reference is not one of
          if (!is_static && !nested.dynamic_dependencies.empty()) {
            result.dynamic_dependencies.emplace(
                entry.pointer.concat(make_weak_pointer(property.first)));
          }

          result.unresolved = result.unresolved || nested.unresolved;
          result.static_dependencies.merge(nested.static_dependencies);
          result.dynamic_dependencies.merge(nested.dynamic_dependencies);
        } else if (reference.first ==
                   sourcemeta::core::SchemaReferenceType::Dynamic) {
          result.unresolved = true;
        }

        break;
      }

      // Static
      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlace:
        // TODO(C++23): Use std::views::enumerate when available in libc++
        for (std::size_t index = 0; index < property.second.size(); index++) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, make_weak_pointer(property.first, index))
                  .value()
                  .get(),
              is_static, visited, result);
        }

        break;

      // Dynamic
      case sourcemeta::core::SchemaKeywordType::ApplicatorElementsInPlaceSome:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, make_weak_pointer(property.first, index))
                    .value()
                    .get(),
                false, visited, result);
          }
        }

        break;
      case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseAnyItem:
        [[fallthrough]];
      case sourcemeta::core::SchemaKeywordType::ApplicatorValueTraverseParent:
        [[fallthrough]];
      case sourcemeta::core::SchemaKeywordType::ApplicatorValueInPlaceMaybe:
        if ((property.second.is_object() || property.second.is_boolean())) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, make_weak_pointer(property.first))
                  .value()
                  .get(),
              false, visited, result);
        }

        break;
      case sourcemeta::core::SchemaKeywordType::
          ApplicatorValueOrElementsInPlace:
        if (property.second.is_array()) {
          for (std::size_t index = 0; index < property.second.size(); index++) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame.traverse(entry, make_weak_pointer(property.first, index))
                    .value()
                    .get(),
                false, visited, result);
          }
        } else if ((property.second.is_object() ||
                    property.second.is_boolean())) {
          find_adjacent_dependencies(
              current, schema, frame, walker, resolver, keywords, root,
              frame.traverse(entry, make_weak_pointer(property.first))
                  .value()
                  .get(),
              false, visited, result);
        }

        break;
      case sourcemeta::core::SchemaKeywordType::ApplicatorMembersInPlaceSome:
        if (property.second.is_object()) {
          for (const auto &pair : property.second.as_object()) {
            find_adjacent_dependencies(
                current, schema, frame, walker, resolver, keywords, root,
                frame
                    .traverse(entry,
                              make_weak_pointer(property.first, pair.first))
                    .value()
                    .get(),
                false, visited, result);
          }
        }

        break;

      // Anything else does not contribute to the dependency list
      default:
        break;
    }
  }
}

inline auto register_under_all_bases(
    SchemaUnevaluatedEntries &result,
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::SchemaFrame::Location &location,
    const sourcemeta::core::JSON::String &keyword,
    const SchemaUnevaluatedEntry &value) -> void {
  result.emplace(frame.uri(location, make_weak_pointer(keyword)), value);
  frame.for_each_location(
      [&](const sourcemeta::core::SchemaReferenceType, const std::string_view,
          const sourcemeta::core::SchemaFrame::Location &alternate) -> void {
        if (alternate.pointer != location.pointer ||
            alternate.base == location.base) {
          return;
        }

        if (alternate.type !=
                sourcemeta::core::SchemaFrame::LocationType::Subschema &&
            alternate.type !=
                sourcemeta::core::SchemaFrame::LocationType::Resource &&
            alternate.type !=
                sourcemeta::core::SchemaFrame::LocationType::Anchor) {
          return;
        }

        result.emplace(frame.uri(alternate, make_weak_pointer(keyword)), value);
      });
}

// TODO: Refactor this entire function using `SchemaFrame`'s new `Instances`
// mode. We can loop over every subschema that defines `unevaluatedProperties`
// or `unevaluatedItems`, find all other subschemas with the same unresolved
// instance location (static dependency) or conditional equivalent unresolved
// instance location (dynamic dependency) and see if those ones define any of
// the dependent keywords.
inline auto unevaluated(const sourcemeta::core::JSON &schema,
                        const sourcemeta::core::SchemaFrame &frame,
                        const sourcemeta::core::SchemaWalker &walker,
                        const sourcemeta::core::SchemaResolver &resolver)
    -> SchemaUnevaluatedEntries {
  SchemaUnevaluatedEntries result;

  frame.for_each_subschema(
      [&](const sourcemeta::core::SchemaFrame::Location &location) -> void {
        const auto &subschema{sourcemeta::core::get(schema, location.pointer)};
        assert((subschema.is_object() || subschema.is_boolean()));
        if (!subschema.is_object()) {
          return;
        }

        const bool has_unevaluated_properties{
            subschema.defines("unevaluatedProperties")};
        const bool has_unevaluated_items{subschema.defines("unevaluatedItems")};
        if (!has_unevaluated_properties && !has_unevaluated_items) {
          return;
        }

        const auto &subschema_vocabularies{
            frame.vocabularies(location, resolver)};

        // The same pointer may be reachable through alternate identifiers whose
        // dynamic anchors carry a different base, so we register the entry
        // under each of them
        if (has_unevaluated_properties) {
          if ((subschema_vocabularies.contains(
                   sourcemeta::core::SchemaVocabularies::Known::
                       JSON_SCHEMA_2020_12_UNEVALUATED) &&
               subschema_vocabularies.contains(
                   sourcemeta::core::SchemaVocabularies::Known::
                       JSON_SCHEMA_2020_12_APPLICATOR)) ||
              subschema_vocabularies.contains(
                  sourcemeta::core::SchemaVocabularies::Known::
                      JSON_SCHEMA_2019_09_APPLICATOR)) {
            std::set<std::pair<sourcemeta::core::WeakPointer, bool>> visited;
            SchemaUnevaluatedEntry unevaluated;
            find_adjacent_dependencies(
                "unevaluatedProperties", schema, frame, walker, resolver,
                {"properties", "patternProperties", "additionalProperties",
                 "unevaluatedProperties"},
                location, location, true, visited, unevaluated);
            register_under_all_bases(result, frame, location,
                                     UNEVALUATED_PROPERTIES, unevaluated);
          }
        }

        if (has_unevaluated_items) {
          std::set<std::pair<sourcemeta::core::WeakPointer, bool>> visited;
          SchemaUnevaluatedEntry unevaluated;
          if (subschema_vocabularies.contains(
                  sourcemeta::core::SchemaVocabularies::Known::
                      JSON_SCHEMA_2020_12_UNEVALUATED) &&
              subschema_vocabularies.contains(
                  sourcemeta::core::SchemaVocabularies::Known::
                      JSON_SCHEMA_2020_12_APPLICATOR)) {
            find_adjacent_dependencies(
                "unevaluatedItems", schema, frame, walker, resolver,
                {"prefixItems", "items", "contains", "unevaluatedItems"},
                location, location, true, visited, unevaluated);
            register_under_all_bases(result, frame, location, UNEVALUATED_ITEMS,
                                     unevaluated);
          } else if (subschema_vocabularies.contains(
                         sourcemeta::core::SchemaVocabularies::Known::
                             JSON_SCHEMA_2019_09_APPLICATOR)) {
            find_adjacent_dependencies(
                "unevaluatedItems", schema, frame, walker, resolver,
                {"items", "additionalItems", "unevaluatedItems"}, location,
                location, true, visited, unevaluated);
            register_under_all_bases(result, frame, location, UNEVALUATED_ITEMS,
                                     unevaluated);
          }
        }
      });

  return result;
}

} // namespace sourcemeta::blaze

#endif
