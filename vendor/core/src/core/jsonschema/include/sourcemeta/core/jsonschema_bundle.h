#ifndef SOURCEMETA_CORE_JSONSCHEMA_BUNDLE_H
#define SOURCEMETA_CORE_JSONSCHEMA_BUNDLE_H

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonschema_frame.h>
#include <sourcemeta/core/jsonschema_types.h>
// NOLINTEND(misc-include-cleaner)

#include <functional> // std::function
#include <optional>   // std::optional, std::nullopt

namespace sourcemeta::core {

/// @ingroup jsonschema
/// A callback to get dependency information
/// - Origin URI
/// - Pointer (reference keyword from the origin)
/// - Target URI
/// - Target schema
using DependencyCallback =
    std::function<void(const std::optional<JSON::String> &, const Pointer &,
                       const JSON::String &, const JSON &)>;

/// @ingroup jsonschema
///
/// This function recursively traverses and reports the external references in a
/// schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> std::optional<sourcemeta::core::JSON> {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_official_resolver(identifier);
///   }
/// }
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::core::dependencies(document,
///   sourcemeta::core::schema_official_walker, test_resolver,
///   [](const auto &origin,
///      const auto &pointer,
///      const auto &target,
///      const auto &schema) {
///     // Do something with the information
///   });
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto dependencies(
    const JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver, const DependencyCallback &callback,
    const std::optional<std::string> &default_dialect = std::nullopt,
    const std::optional<std::string> &default_id = std::nullopt,
    const SchemaFrame::Paths &paths = {empty_pointer}) -> void;

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
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_official_resolver(identifier);
///   }
/// }
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::core::bundle(document,
///   sourcemeta::core::schema_official_walker, test_resolver);
///
/// const sourcemeta::core::JSON expected =
///     sourcemeta::core::parse_json(R"JSON({
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
auto bundle(JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect = std::nullopt,
            const std::optional<std::string> &default_id = std::nullopt,
            const std::optional<Pointer> &default_container = std::nullopt,
            const SchemaFrame::Paths &paths = {empty_pointer}) -> void;

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
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_official_resolver(identifier);
///   }
/// }
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// const sourcemeta::core::JSON result =
///   sourcemeta::core::bundle(document,
///     sourcemeta::core::schema_official_walker, test_resolver);
///
/// const sourcemeta::core::JSON expected =
///     sourcemeta::core::parse_json(R"JSON({
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
auto bundle(const JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<std::string> &default_dialect = std::nullopt,
            const std::optional<std::string> &default_id = std::nullopt,
            const std::optional<Pointer> &default_container = std::nullopt,
            const SchemaFrame::Paths &paths = {empty_pointer}) -> JSON;

} // namespace sourcemeta::core

#endif
