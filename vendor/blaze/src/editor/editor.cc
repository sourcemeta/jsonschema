#include <sourcemeta/blaze/editor.h>

#include <sourcemeta/blaze/foundation.h>

#include "helpers.h"

#include <cassert> // assert
#include <format>  // std::format
#include <map>     // std::map

namespace {

// Note that we don't take into account dynamic resources behind conditionals,
// etc. We probably should, but the complexity of this transformation would
// massively grow, plus such case is quite uncommon in practice.
// See https://arxiv.org/abs/2503.11288 for an academic study of this topic
auto top_dynamic_anchor_location(
    const sourcemeta::blaze::SchemaFrame &frame,
    const sourcemeta::core::WeakPointer &current,
    const std::string_view fragment,
    const sourcemeta::core::JSON::String &default_uri)
    -> std::optional<
        std::reference_wrapper<const sourcemeta::core::WeakPointer>> {
  // Get the location object of where we are at the moment
  const auto uri{frame.uri(current)};
  assert(uri.has_value());
  const auto match{frame.traverse(uri.value().get())};
  assert(match.has_value());
  const auto &location{match.value().get()};

  // Try to locate an anchor with the given name on the current base
  assert(!fragment.starts_with('#'));
  sourcemeta::core::JSON::String anchor_uri{location.base};
  anchor_uri += '#';
  anchor_uri += fragment;
  const auto anchor{frame.traverse(anchor_uri)};

  if (location.parent.has_value()) {
    // If there is a parent resource, keep looking there, but update the default
    // if the current resource has the dynamic anchor we want
    return top_dynamic_anchor_location(frame, location.parent.value(), fragment,
                                       anchor.has_value() ? anchor_uri
                                                          : default_uri);

    // If we are at the top of the schema and it declares the dynamic anchor, we
    // should use that
  } else if (anchor.has_value()) {
    return std::cref(anchor.value().get().pointer);

    // Otherwise, if we are at the top and the dynamic anchor is not there, use
    // the default we have so far
  } else {
    const auto default_location{frame.traverse(default_uri)};
    assert(default_location.has_value());
    return std::cref(default_location.value().get().pointer);
  }
}

} // namespace

namespace sourcemeta::blaze {

// Collected information about a reference to modify
struct ReferenceChange {
  sourcemeta::core::Pointer pointer;
  sourcemeta::core::JSON::String new_value;
  sourcemeta::core::JSON::String keyword;
  bool rename_to_ref;
};

// Collected information about a subschema to modify
struct SubschemaChange {
  sourcemeta::core::Pointer pointer;
  sourcemeta::blaze::SchemaBaseDialect base_dialect;
  bool add_schema_declaration;
  bool erase_2020_12_keywords;
  bool erase_2019_09_keywords;
};

auto for_editor(sourcemeta::core::JSON &schema,
                const sourcemeta::blaze::SchemaWalker &walker,
                const sourcemeta::blaze::SchemaResolver &resolver,
                std::string_view default_dialect) -> void {
  // (1) Frame the schema and collect all changes we need to make
  std::vector<ReferenceChange> reference_changes;
  std::vector<SubschemaChange> subschema_changes;

  {
    sourcemeta::blaze::SchemaFrame frame{
        sourcemeta::blaze::SchemaFrame::Mode::References, schema, walker,
        resolver, default_dialect};

    // Otherwise the input is not bundled
    assert(frame.standalone());

    // Note that `std::unordered_map` is slower here due to high collision rates
    // from the simple pointer hashes
    std::map<sourcemeta::core::WeakPointer, std::string_view> pointer_to_uri;
    frame.for_each_location(
        [&pointer_to_uri](
            const sourcemeta::blaze::SchemaReferenceType,
            const std::string_view uri,
            const sourcemeta::blaze::SchemaFrame::Location &location) -> void {
          pointer_to_uri.emplace(location.pointer, uri);
        });

    // Collect reference changes
    frame.for_each_reference(
        [&](const sourcemeta::blaze::SchemaReferenceType type,
            const sourcemeta::core::WeakPointer &origin,
            const sourcemeta::blaze::SchemaFrame::Reference &reference)
            -> void {
          assert(!origin.empty());
          assert(origin.back().is_property());
          const auto &keyword{origin.back().to_property()};

          if (type == sourcemeta::blaze::SchemaReferenceType::Dynamic) {
            if (reference.fragment.has_value()) {
              const auto destination{top_dynamic_anchor_location(
                  frame, origin, reference.fragment.value(),
                  reference.destination)};
              if (!destination.has_value()) {
                return;
              }

              reference_changes.push_back(
                  {.pointer = sourcemeta::core::to_pointer(origin),
                   .new_value =
                       sourcemeta::core::to_uri(destination.value().get())
                           .recompose(),
                   .keyword = keyword,
                   .rename_to_ref = true});
            } else {
              reference_changes.push_back(
                  {.pointer = sourcemeta::core::to_pointer(origin),
                   .new_value = "",
                   .keyword = keyword,
                   .rename_to_ref = true});
            }
          } else {
            if (keyword == "$schema") {
              // Use pre-built index instead of O(n) frame.uri() scan
              const auto uri_it{pointer_to_uri.find(origin)};
              assert(uri_it != pointer_to_uri.end());
              const auto location{frame.traverse(uri_it->second)};
              assert(location.has_value());
              reference_changes.push_back(
                  {.pointer = sourcemeta::core::to_pointer(origin),
                   .new_value =
                       std::format("{}", location.value().get().base_dialect),
                   .keyword = keyword,
                   .rename_to_ref = false});
              return;
            }

            const auto result{frame.traverse(reference.destination)};
            if (result.has_value()) {
              const bool should_rename =
                  keyword == "$dynamicRef" || keyword == "$recursiveRef";
              reference_changes.push_back(
                  {.pointer = sourcemeta::core::to_pointer(origin),
                   .new_value =
                       sourcemeta::core::to_uri(result.value().get().pointer)
                           .recompose(),
                   .keyword = keyword,
                   .rename_to_ref = should_rename});
            } else {
              reference_changes.push_back(
                  {.pointer = sourcemeta::core::to_pointer(origin),
                   .new_value = reference.destination,
                   .keyword = keyword,
                   .rename_to_ref = false});
            }
          }
        });

    // Collect subschema changes
    frame.for_each_subschema(

        [&](const sourcemeta::blaze::SchemaFrame::Location &location) -> void {
          const auto &subschema{
              sourcemeta::core::get(schema, location.pointer)};
          if (subschema.is_boolean()) {
            return;
          }

          const bool add_schema =
              location.pointer.empty() && !subschema.defines("$schema");
          const auto &vocabularies{frame.vocabularies(location, resolver)};

          subschema_changes.push_back(
              {.pointer = sourcemeta::core::to_pointer(location.pointer),
               .base_dialect = location.base_dialect,
               .add_schema_declaration = add_schema,
               .erase_2020_12_keywords =
                   vocabularies.contains(sourcemeta::blaze::SchemaVocabularies::
                                             Known::JSON_Schema_2020_12_Core),
               .erase_2019_09_keywords =
                   vocabularies.contains(sourcemeta::blaze::SchemaVocabularies::
                                             Known::JSON_Schema_2019_09_Core)});
        });
  }

  // (2) Apply reference changes
  for (const auto &change : reference_changes) {
    if (!change.new_value.empty()) {
      sourcemeta::core::set(schema, change.pointer,
                            sourcemeta::core::JSON{change.new_value});
    }
    if (change.rename_to_ref) {
      sourcemeta::core::get(schema, change.pointer.initial())
          .rename(change.keyword, "$ref");
    }
  }

  // (3) Apply subschema changes
  for (const auto &change : subschema_changes) {
    auto &subschema{sourcemeta::core::get(schema, change.pointer)};

    if (change.add_schema_declaration) {
      subschema.assign_assume_new("$schema", sourcemeta::core::JSON{std::format(
                                                 "{}", change.base_dialect)});
    }

    sourcemeta::blaze::anonymize(subschema, change.base_dialect);

    if (change.erase_2020_12_keywords) {
      subschema.erase_keys({"$vocabulary", "$anchor", "$dynamicAnchor"});
    } else if (change.erase_2019_09_keywords) {
      subschema.erase_keys({"$vocabulary", "$anchor", "$recursiveAnchor"});
    }
  }
}

} // namespace sourcemeta::blaze
