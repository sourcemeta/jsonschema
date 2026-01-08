#include <sourcemeta/core/editorschema.h>

#include <cassert> // assert

namespace {

// Note that we don't take into account dynamic resources behind conditionals,
// etc. We probably should, but the complexity of this transformation would
// massively grow, plus such case is quite uncommon in practice.
// See https://arxiv.org/abs/2503.11288 for an academic study of this topic
auto top_dynamic_anchor_location(
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::Pointer &current, const std::string_view fragment,
    const sourcemeta::core::JSON::String &default_uri)
    -> std::optional<sourcemeta::core::Pointer> {
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
    return top_dynamic_anchor_location(
        frame, to_pointer(location.parent.value()), fragment,
        anchor.has_value() ? anchor_uri : default_uri);

    // If we are at the top of the schema and it declares the dynamic anchor, we
    // should use that
  } else if (anchor.has_value()) {
    return anchor.value().get().pointer;

    // Otherwise, if we are at the top and the dynamic anchor is not there, use
    // the default we have so far
  } else {
    const auto default_location{frame.traverse(default_uri)};
    assert(default_location.has_value());
    return default_location.value().get().pointer;
  }
}

} // namespace

namespace sourcemeta::core {

// Collected information about a reference to modify
struct ReferenceChange {
  Pointer pointer;
  JSON::String new_value;
  JSON::String keyword;
  bool rename_to_ref;
};

// Collected information about a subschema to modify
struct SubschemaChange {
  Pointer pointer;
  SchemaBaseDialect base_dialect;
  bool add_schema_declaration;
  bool erase_2020_12_keywords;
  bool erase_2019_09_keywords;
};

auto for_editor(JSON &schema, const SchemaWalker &walker,
                const SchemaResolver &resolver,
                std::string_view default_dialect) -> void {
  // (1) Bring in all of the references
  bundle(schema, walker, resolver, default_dialect);

  // (2) Frame the schema and collect all changes we need to make
  std::vector<ReferenceChange> reference_changes;
  std::vector<SubschemaChange> subschema_changes;

  {
    SchemaFrame frame{SchemaFrame::Mode::References};
    frame.analyse(schema, walker, resolver, default_dialect);

    // Collect reference changes
    for (const auto &[key, reference] : frame.references()) {
      assert(!key.second.empty());
      assert(key.second.back().is_property());
      const auto &keyword{key.second.back().to_property()};

      if (key.first == SchemaReferenceType::Dynamic) {
        if (reference.fragment.has_value()) {
          auto destination{top_dynamic_anchor_location(
              frame, key.second, reference.fragment.value(),
              reference.destination)};
          if (!destination.has_value()) {
            continue;
          }

          reference_changes.push_back(
              {key.second, to_uri(std::move(destination).value()).recompose(),
               keyword, true});
        } else {
          reference_changes.push_back({key.second, "", keyword, true});
        }
      } else {
        if (keyword == "$schema") {
          const auto uri{frame.uri(key.second)};
          assert(uri.has_value());
          const auto origin{frame.traverse(uri.value().get())};
          assert(origin.has_value());
          reference_changes.push_back(
              {key.second,
               JSON::String{to_string(origin.value().get().base_dialect)},
               keyword, false});
          continue;
        }

        const auto result{frame.traverse(reference.destination)};
        if (result.has_value()) {
          const bool should_rename =
              keyword == "$dynamicRef" || keyword == "$recursiveRef";
          reference_changes.push_back(
              {key.second, to_uri(result.value().get().pointer).recompose(),
               keyword, should_rename});
        } else {
          reference_changes.push_back(
              {key.second, reference.destination, keyword, false});
        }
      }
    }

    // Collect subschema changes
    for (const auto &entry : frame.locations()) {
      if (entry.second.type != SchemaFrame::LocationType::Resource &&
          entry.second.type != SchemaFrame::LocationType::Subschema) {
        continue;
      }

      const auto &subschema{get(schema, entry.second.pointer)};
      if (subschema.is_boolean()) {
        continue;
      }

      const bool add_schema =
          entry.second.pointer.empty() && !subschema.defines("$schema");
      const auto vocabularies{frame.vocabularies(entry.second, resolver)};

      subschema_changes.push_back(
          {entry.second.pointer, entry.second.base_dialect, add_schema,
           vocabularies.contains(Vocabularies::Known::JSON_Schema_2020_12_Core),
           vocabularies.contains(
               Vocabularies::Known::JSON_Schema_2019_09_Core)});
    }
  }

  // (3) Apply reference changes
  for (const auto &change : reference_changes) {
    if (!change.new_value.empty()) {
      set(schema, change.pointer, JSON{change.new_value});
    }
    if (change.rename_to_ref) {
      get(schema, change.pointer.initial()).rename(change.keyword, "$ref");
    }
  }

  // (4) Apply subschema changes
  for (const auto &change : subschema_changes) {
    auto &subschema{get(schema, change.pointer)};

    if (change.add_schema_declaration) {
      subschema.assign_assume_new(
          "$schema", JSON{JSON::String{to_string(change.base_dialect)}});
    }

    anonymize(subschema, change.base_dialect);

    if (change.erase_2020_12_keywords) {
      subschema.erase_keys({"$vocabulary", "$anchor", "$dynamicAnchor"});
    } else if (change.erase_2019_09_keywords) {
      subschema.erase_keys({"$vocabulary", "$anchor", "$recursiveAnchor"});
    }
  }
}

} // namespace sourcemeta::core
