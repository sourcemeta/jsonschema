#include <sourcemeta/core/jsonschema.h>

#include <cassert> // assert
#include <sstream> // std::ostringstream
#include <utility> // std::move

namespace {

auto embed_schema(sourcemeta::core::JSON &root,
                  const sourcemeta::core::Pointer &container,
                  const std::string &identifier,
                  const sourcemeta::core::JSON &target) -> void {
  auto *current{&root};
  for (const auto &token : container) {
    if (token.is_property()) {
      current->assign_if_missing(token.to_property(),
                                 sourcemeta::core::JSON::make_object());
      current = &current->at(token.to_property());
    } else {
      assert(current->is_array() && current->size() >= token.to_index());
      current = &current->at(token.to_index());
    }
  }

  if (!current->is_object()) {
    throw sourcemeta::core::SchemaError(
        "Could not bundle to a container path that is not an object");
  }

  std::ostringstream key;
  key << identifier;
  // Ensure we get a definitions entry that does not exist
  while (current->defines(key.str())) {
    key << "/x";
  }

  current->assign(key.str(), target);
}

auto is_official_metaschema_reference(const sourcemeta::core::Pointer &pointer,
                                      const std::string &destination) -> bool {
  return !pointer.empty() && pointer.back().is_property() &&
         pointer.back().to_property() == "$schema" &&
         sourcemeta::core::schema_official_resolver(destination).has_value();
}

auto bundle_schema(sourcemeta::core::JSON &root,
                   const sourcemeta::core::Pointer &container,
                   const sourcemeta::core::JSON &subschema,
                   sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   const std::optional<std::string> &default_dialect,
                   const sourcemeta::core::SchemaFrame::Paths &paths,
                   const std::size_t depth = 0) -> void {
  // Keep in mind that the resulting frame does miss some information. For
  // example, when we recurse to framing embedded schemas, we will frame them
  // without keeping their new relationship to their parent (after embedding if
  // to the container location). However, that's fine for the purpose of this
  // function, given we don't pass the frame back to the caller
  if (depth == 0) {
    frame.analyse(
        subschema, walker, resolver, default_dialect, std::nullopt,
        // We only want to frame in "wrapper" mode for the top level object
        paths);
  } else {
    frame.analyse(subschema, walker, resolver, default_dialect);
  }

  // Otherwise, given recursion, we would be modifying the
  // references list *while* looping on it
  const auto references_copy = frame.references();
  for (const auto &[key, reference] : references_copy) {
    if (frame.traverse(reference.destination).has_value() ||

        // We don't want to bundle official schemas, as we can expect
        // virtually all implementations to understand them out of the box
        is_official_metaschema_reference(key.second, reference.destination)) {
      continue;
    }

    // If we can't find the destination but there is a base and we can
    // find base, then we are facing an unresolved fragment
    if (reference.base.has_value() &&
        frame.traverse(reference.base.value()).has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    if (!reference.base.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    assert(reference.base.has_value());
    const auto identifier{reference.base.value()};
    const auto remote{resolver(identifier)};
    if (!remote.has_value()) {
      if (frame.traverse(identifier).has_value()) {
        throw sourcemeta::core::SchemaReferenceError(
            reference.destination, key.second,
            "Could not resolve schema reference");
      }

      throw sourcemeta::core::SchemaResolutionError(
          identifier, "Could not resolve the requested schema");
    }

    // Otherwise, if the target schema does not declare an inline identifier,
    // references to that identifier from the outer schema won't resolve.
    sourcemeta::core::JSON copy{remote.value()};

    if (!sourcemeta::core::is_schema(copy)) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, key.second,
          "The JSON document is not a valid JSON Schema");
    }

    const auto dialect{sourcemeta::core::dialect(copy, default_dialect)};
    if (!dialect.has_value()) {
      throw sourcemeta::core::SchemaReferenceError(
          identifier, key.second,
          "The JSON document is not a valid JSON Schema");
    }

    if (copy.is_object()) {
      // Always insert an identifier, as a schema might refer to another schema
      // using another URI (i.e. due to relying on HTTP re-directions, etc)
      sourcemeta::core::reidentify(copy, identifier, resolver, default_dialect);
    }

    embed_schema(root, container, identifier, copy);
    bundle_schema(root, container, copy, frame, walker, resolver,
                  default_dialect, paths, depth + 1);
  }
}

} // namespace

namespace sourcemeta::core {

auto bundle(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect,
            const std::optional<Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> void {
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};

  if (default_container.has_value()) {
    // This is undefined behavior
    assert(!default_container.value().empty());
    bundle_schema(schema, default_container.value(), schema, frame, walker,
                  resolver, default_dialect, paths);
    return;
  }

  const auto vocabularies{
      sourcemeta::core::vocabularies(schema, resolver, default_dialect)};
  if (vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/core") ||
      vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/core")) {
    bundle_schema(schema, {"$defs"}, schema, frame, walker, resolver,
                  default_dialect, paths);
  } else if (vocabularies.contains("http://json-schema.org/draft-07/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-07/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-06/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-06/hyper-schema#") ||
             vocabularies.contains("http://json-schema.org/draft-04/schema#") ||
             vocabularies.contains(
                 "http://json-schema.org/draft-04/hyper-schema#")) {
    bundle_schema(schema, {"definitions"}, schema, frame, walker, resolver,
                  default_dialect, paths);
  } else {
    // We don't attempt to bundle on dialects where we
    // don't know where to put the embedded schemas
    throw sourcemeta::core::SchemaError(
        "Could not determine how to perform bundling in this dialect");
  }
}

auto bundle(const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect,
            const std::optional<Pointer> &default_container,
            const SchemaFrame::Paths &paths) -> sourcemeta::core::JSON {
  sourcemeta::core::JSON copy = schema;
  bundle(copy, walker, resolver, default_dialect, default_container, paths);
  return copy;
}

} // namespace sourcemeta::core
