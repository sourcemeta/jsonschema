#ifndef SOURCEMETA_CORE_JSONSCHEMA_BUNDLE_H_
#define SOURCEMETA_CORE_JSONSCHEMA_BUNDLE_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema_resolver.h>
#include <sourcemeta/core/jsonschema_walker.h>

#include <cstdint>  // std::uint8_t
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string

namespace sourcemeta::core {

// TODO: Optionally let users bundle the metaschema too

/// @ingroup jsonschema
///
/// This function bundles a JSON Schema (starting from Draft 4) by embedding
/// every remote reference into the top level schema resource, handling circular
/// dependencies and more. This overload mutates the input schema.  For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> std::optional<sourcemeta::core::JSON> {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::official_resolver(identifier);
///   }
/// }
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::core::bundle(document,
///   sourcemeta::core::default_schema_walker, test_resolver);
///
/// const sourcemeta::core::JSON expected =
///     sourcemeta::core::parse(R"JSON({
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
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto bundle(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect = std::nullopt)
    -> void;

/// @ingroup jsonschema
///
/// This function bundles a JSON Schema (starting from Draft 4) by embedding
/// every remote reference into the top level schema resource, handling circular
/// dependencies and more. This overload returns a new schema, without mutating
/// the input schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> std::optional<sourcemeta::core::JSON> {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::official_resolver(identifier);
///   }
/// }
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// const sourcemeta::core::JSON result =
///   sourcemeta::core::bundle(document,
///     sourcemeta::core::default_schema_walker, test_resolver);
///
/// const sourcemeta::core::JSON expected =
///     sourcemeta::core::parse(R"JSON({
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
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto bundle(const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect = std::nullopt)
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::core

#endif
