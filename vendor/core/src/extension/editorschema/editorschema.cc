#include <sourcemeta/core/editorschema.h>

#include <cassert> // assert

namespace {

// Note that we don't take into account dynamic resources behind conditionals,
// etc. We probably should, but the complexity of this transformation would
// massively grow, plus such case is quite uncommon in practice.
// See https://arxiv.org/abs/2503.11288 for an academic study of this topic
auto top_dynamic_anchor_location(
    const sourcemeta::core::SchemaFrame &frame,
    const sourcemeta::core::Pointer &current,
    const sourcemeta::core::JSON::String &fragment,
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
  const auto anchor_uri{location.base + "#" + fragment};
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

auto for_editor(JSON &schema, const SchemaWalker &walker,
                const SchemaResolver &resolver,
                const std::optional<std::string> &default_dialect) -> void {
  // (1) Bring in all of the references
  bundle(schema, walker, resolver, default_dialect);

  // (2) Re-frame before changing anything
  SchemaFrame frame{SchemaFrame::Mode::References};
  frame.analyse(schema, walker, resolver, default_dialect);

  // (3) Pre-process all subschemas
  for (const auto &entry : frame.locations()) {
    if (entry.second.type != SchemaFrame::LocationType::Resource &&
        entry.second.type != SchemaFrame::LocationType::Subschema) {
      continue;
    }

    auto &subschema{get(schema, entry.second.pointer)};
    if (subschema.is_boolean()) {
      continue;
    }

    // Make sure that the top-level schema ALWAYS has a `$schema` declaration
    if (entry.second.pointer.empty() && !subschema.defines("$schema")) {
      subschema.assign("$schema", JSON{entry.second.base_dialect});
    }

    // Get rid of the keywords we don't want anymore
    anonymize(subschema, entry.second.base_dialect);
    const auto vocabularies{frame.vocabularies(entry.second, resolver)};
    if (vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/core")) {
      subschema.erase_keys({"$vocabulary", "$anchor", "$dynamicAnchor"});
    } else if (vocabularies.contains(
                   "https://json-schema.org/draft/2019-09/vocab/core")) {
      subschema.erase_keys({"$vocabulary", "$anchor", "$recursiveAnchor"});
    }
  }

  // (4) Fix-up static and dynamic references
  for (const auto &[key, reference] : frame.references()) {
    assert(!key.second.empty());
    assert(key.second.back().is_property());
    const auto &keyword{key.second.back().to_property()};

    if (key.first == SchemaReferenceType::Dynamic) {
      if (reference.fragment.has_value()) {
        auto destination{top_dynamic_anchor_location(frame, key.second,
                                                     reference.fragment.value(),
                                                     reference.destination)};
        if (!destination.has_value()) {
          continue;
        }

        set(schema, key.second,
            JSON{to_uri(std::move(destination).value()).recompose()});
      }

      get(schema, key.second.initial()).rename(keyword, "$ref");
    } else {
      // The `$schema` keyword is not allowed to take relative URIs (for
      // example, pointers going from the root). Because we remove identifiers,
      // the only sane thing we can do here is default it to the base dialect,
      // which editors will likely understand
      if (keyword == "$schema") {
        const auto uri{frame.uri(key.second)};
        assert(uri.has_value());
        const auto origin{frame.traverse(uri.value().get())};
        assert(origin.has_value());
        set(schema, key.second, JSON{origin.value().get().base_dialect});
        continue;
      }

      // As we get rid of identifiers, we rephrase every reference to be the URI
      // representation of the JSON Pointer to the destination from the root
      const auto result{frame.traverse(reference.destination)};
      if (result.has_value()) {
        set(schema, key.second,
            JSON{to_uri(result.value().get().pointer).recompose()});

        // If we have a dynamic reference to a static location,
        // we can just rename the keyword
        if (keyword == "$dynamicRef" || keyword == "$recursiveRef") {
          get(schema, key.second.initial()).rename(keyword, "$ref");
        }

      } else {
        set(schema, key.second, JSON{reference.destination});
      }
    }
  }
}

} // namespace sourcemeta::core
