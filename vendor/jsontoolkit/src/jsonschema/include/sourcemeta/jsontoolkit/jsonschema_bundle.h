#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_BUNDLE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_BUNDLE_H_

#include "jsonschema_export.h"

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <future>   // std::future
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string

namespace sourcemeta::jsontoolkit {

// TODO: Optionally let users bundle the metaschema too

/// @ingroup jsonschema
/// A set of options that modify the behavior of bundling
enum class BundleOptions {
  /// Perform standard JSON Schema bundling
  Default,

  /// Perform standard JSON Schema bundling but without making
  /// use of identifiers. This is helpful for delivering
  /// schemas to some non-compliant implementations that do not
  /// recognize identifiers (like Visua Studio Code at the time
  /// of this writing)
  WithoutIdentifiers
};

/// @ingroup jsonschema
///
/// This function bundles a JSON Schema (starting from Draft 4) by embedding
/// every remote reference into the top level schema resource, handling circular
/// dependencies and more. This overload mutates the input schema.  For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
///   std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
///   if (identifier == "https://www.example.com/test") {
///     promise.set_value(sourcemeta::jsontoolkit::parse(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON"));
///   } else {
///     promise.set_value(
///         sourcemeta::jsontoolkit::official_resolver(identifier).get());
///   }
///
///   return promise.get_future();
/// }
///
/// sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::jsontoolkit::bundle(document,
///   sourcemeta::jsontoolkit::default_schema_walker, test_resolver).wait();
///
/// const sourcemeta::jsontoolkit::JSON expected =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" },
///   "$defs": {
///     "https://www.example.com/test": {
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     }
///   }
/// })JSON");
///
/// assert(document == expected);
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto bundle(sourcemeta::jsontoolkit::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const BundleOptions options = BundleOptions::Default,
            const std::optional<std::string> &default_dialect = std::nullopt)
    -> std::future<void>;

/// @ingroup jsonschema
///
/// This function bundles a JSON Schema (starting from Draft 4) by embedding
/// every remote reference into the top level schema resource, handling circular
/// dependencies and more. This overload returns a new schema, without mutating
/// the input schema. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
///   std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
///   if (identifier == "https://www.example.com/test") {
///     promise.set_value(sourcemeta::jsontoolkit::parse(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON"));
///   } else {
///     promise.set_value(
///         sourcemeta::jsontoolkit::official_resolver(identifier).get());
///   }
///
///   return promise.get_future();
/// }
///
/// const sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// const sourcemeta::jsontoolkit::JSON result =
///   sourcemeta::jsontoolkit::bundle(document,
///     sourcemeta::jsontoolkit::default_schema_walker, test_resolver).get();
///
/// const sourcemeta::jsontoolkit::JSON expected =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" },
///   "$defs": {
///     "https://www.example.com/test": {
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     }
///   }
/// })JSON");
///
/// assert(result == expected);
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto bundle(const sourcemeta::jsontoolkit::JSON &schema,
            const SchemaWalker &walker, const SchemaResolver &resolver,
            const BundleOptions options = BundleOptions::Default,
            const std::optional<std::string> &default_dialect = std::nullopt)
    -> std::future<sourcemeta::jsontoolkit::JSON>;

} // namespace sourcemeta::jsontoolkit

#endif
