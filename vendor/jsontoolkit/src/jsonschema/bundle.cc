#include <sourcemeta/jsontoolkit/jsonschema.h>

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
      vocabularies.contains("http://json-schema.org/draft-06/schema#") ||
      vocabularies.contains("http://json-schema.org/draft-04/schema#")) {
    return "definitions";
  }

  // We don't attempt to bundle on dialects where we
  // don't know where to put the embedded schemas
  throw sourcemeta::jsontoolkit::SchemaError(
      "Cannot determine how to bundle on this dialect");
}

// TODO: Turn it into an official function to set a schema identifier
auto upsert_id(sourcemeta::jsontoolkit::JSON &target,
               const std::string &identifier,
               const sourcemeta::jsontoolkit::SchemaResolver &resolver,
               const std::optional<std::string> &default_dialect) -> void {
  const auto dialect{sourcemeta::jsontoolkit::dialect(target, default_dialect)};
  assert(dialect.has_value());
  const auto base_dialect{
      sourcemeta::jsontoolkit::base_dialect(target, resolver, dialect).get()};
  const auto vocabularies{sourcemeta::jsontoolkit::vocabularies(
                              resolver, base_dialect.value(), dialect.value())
                              .get()};
  assert(!vocabularies.empty());

  // Always insert an identifier, as a schema might refer to another schema
  // using another URI (i.e. due to relying on HTTP re-directions, etc)

  if (target.is_object()) {
    if (vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/core") ||
        vocabularies.contains(
            "https://json-schema.org/draft/2019-09/vocab/core") ||
        vocabularies.contains("http://json-schema.org/draft-07/schema#") ||
        vocabularies.contains("http://json-schema.org/draft-06/schema#")) {
      target.assign("$id", sourcemeta::jsontoolkit::JSON{identifier});
    } else if (vocabularies.contains(
                   "http://json-schema.org/draft-04/schema#") ||
               vocabularies.contains(
                   "http://json-schema.org/draft-03/schema#") ||
               vocabularies.contains(
                   "http://json-schema.org/draft-02/schema#") ||
               vocabularies.contains(
                   "http://json-schema.org/draft-01/schema#") ||
               vocabularies.contains(
                   "http://json-schema.org/draft-00/schema#")) {
      target.assign("id", sourcemeta::jsontoolkit::JSON{identifier});
    } else {
      throw sourcemeta::jsontoolkit::SchemaError(
          "Cannot determine how to bundle on this dialect");
    }
  }

  assert(sourcemeta::jsontoolkit::id(target, base_dialect.value()).has_value());
}

auto embed_schema(sourcemeta::jsontoolkit::JSON &definitions,
                  const std::string &identifier,
                  const sourcemeta::jsontoolkit::JSON &target) -> void {
  std::ostringstream key;
  key << identifier;
  // Ensure we get a definitions entry that does not exist
  while (definitions.defines(key.str())) {
    key << "/x";
  }

  definitions.assign(key.str(), target);
}

auto bundle_schema(sourcemeta::jsontoolkit::JSON &root,
                   const std::string &container,
                   const sourcemeta::jsontoolkit::JSON &subschema,
                   sourcemeta::jsontoolkit::ReferenceFrame &frame,
                   const sourcemeta::jsontoolkit::SchemaWalker &walker,
                   const sourcemeta::jsontoolkit::SchemaResolver &resolver,
                   const std::optional<std::string> &default_dialect) -> void {
  sourcemeta::jsontoolkit::ReferenceMap references;
  sourcemeta::jsontoolkit::frame(subschema, frame, references, walker, resolver,
                                 default_dialect)
      .wait();

  for (const auto &[key, reference] : references) {
    if (frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
                        reference.destination}) ||
        frame.contains({sourcemeta::jsontoolkit::ReferenceType::Dynamic,
                        reference.destination})) {
      continue;
    }

    root.assign_if_missing(container,
                           sourcemeta::jsontoolkit::JSON::make_object());

    if (!reference.base.has_value()) {
      throw sourcemeta::jsontoolkit::SchemaReferenceError(
          reference.destination, key.second,
          "Could not resolve schema reference");
    }

    assert(reference.base.has_value());
    const auto identifier{reference.base.value()};
    const auto remote{resolver(identifier).get()};
    if (!remote.has_value()) {
      if (frame.contains(
              {sourcemeta::jsontoolkit::ReferenceType::Static, identifier}) ||
          frame.contains(
              {sourcemeta::jsontoolkit::ReferenceType::Dynamic, identifier})) {
        throw sourcemeta::jsontoolkit::SchemaReferenceError(
            reference.destination, key.second,
            "Could not resolve schema reference");
      }

      throw sourcemeta::jsontoolkit::SchemaResolutionError(
          identifier, "Could not resolve schema");
    }

    // Otherwise, if the target schema does not declare an inline identifier,
    // references to that identifier from the outer schema won't resolve.
    sourcemeta::jsontoolkit::JSON copy{remote.value()};
    upsert_id(copy, identifier, resolver, default_dialect);

    embed_schema(root.at(container), identifier, copy);
    bundle_schema(root, container, copy, frame, walker, resolver,
                  default_dialect);
  }
}

} // namespace

namespace sourcemeta::jsontoolkit {

auto bundle(sourcemeta::jsontoolkit::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
#ifdef NDEBUG
            const BundleOptions,
#else
            const BundleOptions options,
#endif
            const std::optional<std::string> &default_dialect)
    -> std::future<void> {
  assert(options == BundleOptions::Default);
  const auto vocabularies{
      sourcemeta::jsontoolkit::vocabularies(schema, resolver, default_dialect)
          .get()};
  sourcemeta::jsontoolkit::ReferenceFrame frame;
  bundle_schema(schema, definitions_keyword(vocabularies), schema, frame,
                walker, resolver, default_dialect);
  return std::promise<void>{}.get_future();
}

auto bundle(const sourcemeta::jsontoolkit::JSON &schema,
            const SchemaWalker &walker, const SchemaResolver &resolver,
            const BundleOptions options,
            const std::optional<std::string> &default_dialect)
    -> std::future<sourcemeta::jsontoolkit::JSON> {
  sourcemeta::jsontoolkit::JSON copy = schema;
  bundle(copy, walker, resolver, options, default_dialect).wait();
  std::promise<sourcemeta::jsontoolkit::JSON> promise;
  promise.set_value(std::move(copy));
  return promise.get_future();
}

} // namespace sourcemeta::jsontoolkit
