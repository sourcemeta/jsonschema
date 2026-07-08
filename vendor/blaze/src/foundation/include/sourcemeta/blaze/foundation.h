#ifndef SOURCEMETA_BLAZE_FOUNDATION_H_
#define SOURCEMETA_BLAZE_FOUNDATION_H_

#ifndef SOURCEMETA_BLAZE_FOUNDATION_EXPORT
#include <sourcemeta/blaze/foundation_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/foundation_error.h>
#include <sourcemeta/blaze/foundation_types.h>
#include <sourcemeta/blaze/foundation_walker.h>
// NOLINTEND(misc-include-cleaner)

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

/// @defgroup foundation Foundation
/// @brief A set of JSON Schema utilities across dialects.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/foundation.h>
/// ```

namespace sourcemeta::blaze {

/// @ingroup foundation
/// A default resolver that relies on built-in official schemas.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_resolver(const std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON>;

/// @ingroup foundation
/// Check if a given identifier corresponds to a known built-in schema
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto is_known_schema(const std::string_view identifier) noexcept -> bool;

/// @ingroup foundation
/// Check if a given URI corresponds to an official schema released by the
/// JSON Schema organisation
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto is_official_schema(const std::string_view identifier) noexcept -> bool;

/// @ingroup foundation
/// A default schema walker with support for a wide range of drafts
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_walker(const std::string_view keyword,
                   const Vocabularies &vocabularies)
    -> const SchemaWalkerResult &;

/// @ingroup foundation
/// Stringify a base dialect to its URI
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto to_string(const SchemaBaseDialect base_dialect) -> std::string_view;

/// @ingroup foundation
/// Parse a base dialect URI to its enum representation
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto to_base_dialect(const std::string_view base_dialect)
    -> std::optional<SchemaBaseDialect>;

/// @ingroup foundation
///
/// This function returns true if the given JSON instance is of a
/// schema-compatible type: an object or a boolean. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document{true};
/// assert(sourcemeta::blaze::is_schema(document));
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto is_schema(const sourcemeta::core::JSON &schema) -> bool;

/// @ingroup foundation
///
/// This function returns true if the given JSON instance is a schema
/// semantically equivalent to the empty schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document{true};
/// assert(sourcemeta::blaze::is_empty_schema(document));
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto is_empty_schema(const sourcemeta::core::JSON &schema) -> bool;

/// @ingroup foundation
///
/// This function returns the URI identifier of the given schema, or an empty
/// string view if the schema has no identifier. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// const auto id{sourcemeta::blaze::identify(
///   document, sourcemeta::blaze::schema_resolver)};
/// assert(!id.empty());
/// assert(id == "https://sourcemeta.com/example-schema");
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto identify(const sourcemeta::core::JSON &schema,
              const SchemaResolver &resolver,
              std::string_view default_dialect = "",
              std::string_view default_id = "",
              bool allow_dialect_override = true) -> std::string_view;

/// @ingroup foundation
///
/// A shortcut to sourcemeta::blaze::identify if you know the base dialect
/// of the schema.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto identify(const sourcemeta::core::JSON &schema,
              const SchemaBaseDialect base_dialect,
              std::string_view default_id = "") -> std::string_view;

/// @ingroup foundation
///
/// This function removes the top-level URI identifier of the given schema, if
/// any, given you know its base dialect. It is the caller responsibility to
/// ensure the schema doesn't perform relative references that might have
/// depended on such top-level identifier. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// sourcemeta::blaze::anonymize(document,
///   sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12);
///
/// const auto id{sourcemeta::blaze::identify(
///   document, sourcemeta::blaze::schema_resolver)};
/// assert(id.empty());
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto anonymize(sourcemeta::core::JSON &schema,
               const SchemaBaseDialect base_dialect) -> void;

/// @ingroup foundation
///
/// This function sets the identifier of a schema, replacing the existing one,
/// if any. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// sourcemeta::blaze::reidentify(document,
///   "https://example.com/my-new-id",
///   sourcemeta::blaze::schema_resolver);
///
/// const auto id{sourcemeta::blaze::identify(
///   document, sourcemeta::blaze::schema_resolver)};
/// assert(!id.empty());
/// assert(id == "https://example.com/my-new-id");
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto reidentify(sourcemeta::core::JSON &schema, std::string_view new_identifier,
                const SchemaResolver &resolver,
                std::string_view default_dialect = "") -> void;

/// @ingroup foundation
///
/// A shortcut to sourcemeta::blaze::reidentify if you know the base
/// dialect of the schema.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto reidentify(sourcemeta::core::JSON &schema, std::string_view new_identifier,
                const SchemaBaseDialect base_dialect) -> void;

/// @ingroup foundation
///
/// Get the dialect URI that corresponds to a JSON Schema instance.
/// The result is empty if the dialect cannot be determined. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const auto dialect{sourcemeta::blaze::dialect(document)};
/// assert(!dialect.empty());
/// assert(dialect == "https://json-schema.org/draft/2020-12/schema");
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto dialect(const sourcemeta::core::JSON &schema,
             std::string_view default_dialect = "",
             bool allow_dialect_override = true) -> std::string_view;

/// @ingroup foundation
///
/// Try to locate the meta-schema that the given schema declares from within
/// the schema itself, as self-contained schemas embed the meta-schemas they
/// depend on. The result points into the given document and is null if no
/// valid embedded meta-schema could be found. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON schema =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://example.com/meta",
///   "$defs": {
///     "https://example.com/meta": {
///       "$id": "https://example.com/meta",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "object"
///     }
///   }
/// })JSON");
///
/// const auto *metaschema{sourcemeta::blaze::metaschema_try_embedded(
///   schema, "https://example.com/meta",
///   sourcemeta::blaze::schema_resolver)};
///
/// assert(metaschema);
/// assert(metaschema == &schema.at("$defs").at("https://example.com/meta"));
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto metaschema_try_embedded(const sourcemeta::core::JSON &schema,
                             std::string_view identifier,
                             const SchemaResolver &resolver)
    -> const sourcemeta::core::JSON *;

/// @ingroup foundation
///
/// Get the metaschema document that describes the given schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const sourcemeta::core::JSON metaschema{
///   sourcemeta::blaze::metaschema(
///     document, sourcemeta::blaze::schema_resolver)};
///
/// sourcemeta::core::prettify(metaschema, std::cout);
/// std::cout << std::endl;
/// ```
///
/// This function will throw if the metaschema cannot be determined or resolved.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto metaschema(const sourcemeta::core::JSON &schema,
                const SchemaResolver &resolver,
                std::string_view default_dialect = "")
    -> sourcemeta::core::JSON;

/// @ingroup foundation
///
/// Get the base dialect that applies to the given schema. If you set
/// a default dialect URI, this will be used if the given schema does not
/// declare the `$schema` keyword. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const auto base_dialect{
///   sourcemeta::blaze::base_dialect(
///     document, sourcemeta::blaze::schema_resolver)};
///
/// assert(base_dialect.has_value());
/// assert(base_dialect.value() ==
///   sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12);
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto base_dialect(const sourcemeta::core::JSON &schema,
                  const SchemaResolver &resolver,
                  std::string_view default_dialect = "",
                  bool allow_dialect_override = true)
    -> std::optional<SchemaBaseDialect>;

/// @ingroup foundation
///
/// Parse the `$vocabulary` keyword from a given schema, if set. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$vocabulary": {
///     "https://json-schema.org/draft/2020-12/vocab/core": true,
///     "https://json-schema.org/draft/2020-12/vocab/applicator": true
///   }
/// })JSON");
///
/// const auto result{
///   sourcemeta::blaze::parse_vocabularies(
///     document, sourcemeta::blaze::schema_resolver)};
///
/// assert(result.has_value());
/// assert(result->size() == 2);
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto parse_vocabularies(const sourcemeta::core::JSON &schema,
                        const SchemaResolver &resolver,
                        std::string_view default_dialect = "")
    -> std::optional<Vocabularies>;

/// @ingroup foundation
///
/// A shortcut to sourcemeta::blaze::parse_vocabularies when the base dialect
/// is already known.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto parse_vocabularies(const sourcemeta::core::JSON &schema,
                        const SchemaBaseDialect base_dialect)
    -> std::optional<Vocabularies>;

/// @ingroup foundation
///
/// List the vocabularies that a specific schema makes use of. If you set a
/// default dialect URI, this will be used if the given schema does not
/// declare the
/// `$schema` keyword. The resulting map values are set to `true` or `false`
/// depending on whether the corresponding vocabulary is required or optional,
/// respectively. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const auto vocabularies{
///   sourcemeta::blaze::vocabularies(
///     document, sourcemeta::blaze::schema_resolver)};
///
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/core"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/applicator"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/unevaluated"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/validation"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/meta-data"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/format-annotation"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/content"));
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto vocabularies(const sourcemeta::core::JSON &schema,
                  const SchemaResolver &resolver,
                  std::string_view default_dialect = "") -> Vocabularies;

/// @ingroup foundation
///
/// A shortcut to sourcemeta::blaze::vocabularies based on the base
/// dialect and dialect URI.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto vocabularies(const SchemaResolver &resolver,
                  const SchemaBaseDialect base_dialect,
                  std::string_view dialect) -> Vocabularies;

/// @ingroup foundation
///
/// Parse the value of a JSON Schema `type` keyword (which can be a string or
/// an array of strings) into a set of native JSON types. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// const auto type{sourcemeta::core::parse_json(R"JSON([ "string", "null"
/// ])JSON")}; const auto types{sourcemeta::blaze::parse_schema_type(type)};
/// assert(types.test(
///     static_cast<std::size_t>(sourcemeta::core::JSON::Type::String)));
/// assert(types.test(
///     static_cast<std::size_t>(sourcemeta::core::JSON::Type::Null)));
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto parse_schema_type(const sourcemeta::core::JSON &type)
    -> sourcemeta::core::JSON::TypeSet;

} // namespace sourcemeta::blaze

#endif
