#include <sourcemeta/core/jsonschema.h>

#include <cassert> // assert
#include <map>     // std::map
#include <sstream> // std::ostringstream
#include <utility> // std::move

namespace {

auto definitions_keyword(const std::map<std::string, bool> &vocabularies)
    -> std::string {
  if (vocabularies.contains(
          "https://json-schema.org/draft/2020-12/vocab/core") ||
      vocabularies.contains(
          "https://json-schema.org/draft/2019-09/vocab/core")) {
    return "$defs";
  }

  if (vocabularies.contains("http://json-schema.org/draft-07/schema#") ||
      vocabularies.contains("http://json-schema.org/draft-07/hyper-schema#") ||
      vocabularies.contains("http://json-schema.org/draft-06/schema#") ||
      vocabularies.contains("http://json-schema.org/draft-06/hyper-schema#") ||
      vocabularies.contains("http://json-schema.org/draft-04/schema#") ||
      vocabularies.contains("http://json-schema.org/draft-04/hyper-schema#")) {
    return "definitions";
  }

  // We don't attempt to bundle on dialects where we
  // don't know where to put the embedded schemas
  throw sourcemeta::core::SchemaError(
      "Cannot determine how to bundle on this dialect");
}

auto embed_schema(sourcemeta::core::JSON &definitions,
                  const std::string &identifier,
                  const sourcemeta::core::JSON &target) -> void {
  std::ostringstream key;
  key << identifier;
  // Ensure we get a definitions entry that does not exist
  while (definitions.defines(key.str())) {
    key << "/x";
  }

  definitions.assign(key.str(), target);
}

auto is_official_metaschema_reference(const sourcemeta::core::Pointer &pointer,
                                      const std::string &destination) -> bool {
  return !pointer.empty() && pointer.back().is_property() &&
         pointer.back().to_property() == "$schema" &&
         sourcemeta::core::schema_official_resolver(destination).has_value();
}

auto bundle_schema(sourcemeta::core::JSON &root, const std::string &container,
                   const sourcemeta::core::JSON &subschema,
                   sourcemeta::core::SchemaFrame &frame,
                   const sourcemeta::core::SchemaWalker &walker,
                   const sourcemeta::core::SchemaResolver &resolver,
                   const std::optional<std::string> &default_dialect) -> void {
  frame.analyse(subschema, walker, resolver, default_dialect);
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

    root.assign_if_missing(container, sourcemeta::core::JSON::make_object());

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
      throw sourcemeta::core::SchemaResolutionError(
          identifier, "The JSON document is not a valid JSON Schema");
    }

    const auto dialect{sourcemeta::core::dialect(copy, default_dialect)};
    if (!dialect.has_value()) {
      throw sourcemeta::core::SchemaResolutionError(
          identifier, "The JSON document is not a valid JSON Schema");
    }

    if (copy.is_object()) {
      // Always insert an identifier, as a schema might refer to another schema
      // using another URI (i.e. due to relying on HTTP re-directions, etc)
      sourcemeta::core::reidentify(copy, identifier, resolver, default_dialect);
    }

    embed_schema(root.at(container), identifier, copy);
    bundle_schema(root, container, copy, frame, walker, resolver,
                  default_dialect);
  }
}

} // namespace

namespace sourcemeta::core {

auto bundle(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect) -> void {
  const auto vocabularies{
      sourcemeta::core::vocabularies(schema, resolver, default_dialect)};
  sourcemeta::core::SchemaFrame frame;
  bundle_schema(schema, definitions_keyword(vocabularies), schema, frame,
                walker, resolver, default_dialect);
}

auto bundle(const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::JSON {
  sourcemeta::core::JSON copy = schema;
  bundle(copy, walker, resolver, default_dialect);
  return copy;
}

} // namespace sourcemeta::core
