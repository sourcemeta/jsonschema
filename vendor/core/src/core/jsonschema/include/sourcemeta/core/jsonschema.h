#ifndef SOURCEMETA_CORE_JSONSCHEMA_H_
#define SOURCEMETA_CORE_JSONSCHEMA_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonschema_bundle.h>
#include <sourcemeta/core/jsonschema_error.h>
#include <sourcemeta/core/jsonschema_frame.h>
#include <sourcemeta/core/jsonschema_transform.h>
#include <sourcemeta/core/jsonschema_types.h>
#include <sourcemeta/core/jsonschema_walker.h>
// NOLINTEND(misc-include-cleaner)

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <optional>    // std::optional, std::nullopt
#include <set>         // std::set
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup jsonschema JSON Schema
/// @brief A set of JSON Schema utilities across draft versions.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonschema
/// A default resolver that relies on built-in official schemas.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_resolver(std::string_view identifier) -> std::optional<JSON>;

/// @ingroup jsonschema
/// A default schema walker with support for a wide range of drafs
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_walker(std::string_view keyword, const Vocabularies &vocabularies)
    -> const SchemaWalkerResult &;

/// @ingroup jsonschema
///
/// Calculate the priority of a keyword that determines the ordering in which a
/// JSON Schema implementation should evaluate keyword on a subschema. It does
/// so based on the keyword dependencies expressed in the schema walker. The
/// higher the priority, the more the evaluation of such keyword must be
/// delayed.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "prefixItems": [ true, true ],
///   "items": false
/// })JSON");
///
/// const auto vocabularies{
///   sourcemeta::core::vocabularies(
///     document, sourcemeta::core::schema_resolver)};
///
/// assert(sourcemeta::core::schema_keyword_priority(
///   "prefixItems", vocabularies,
///   sourcemeta::core::schema_walker) == 0);
///
/// // The "items" keyword must be evaluated after the "prefixItems" keyword
/// assert(sourcemeta::core::schema_keyword_priority(
///   "items", vocabularies,
///   sourcemeta::core::schema_walker) == 1);
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_keyword_priority(std::string_view keyword,
                             const Vocabularies &vocabularies,
                             const SchemaWalker &walker) -> std::uint64_t;

/// @ingroup jsonschema
///
/// This function returns true if the given JSON instance is of a
/// schema-compatible type: an object or a boolean. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document{true};
/// assert(sourcemeta::core::is_schema(document));
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto is_schema(const JSON &schema) -> bool;

/// @ingroup jsonschema
///
/// This function returns true if the given JSON instance is a schema
/// semantically equivalent to the empty schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document{true};
/// assert(sourcemeta::core::is_empty_schema(document));
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto is_empty_schema(const JSON &schema) -> bool;

/// @ingroup jsonschema
///
/// This function returns the URI identifier of the given schema, if any. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// std::optional<std::string> id{sourcemeta::core::identify(
///   document, sourcemeta::core::schema_resolver)};
/// assert(id.has_value());
/// assert(id.value() == "https://sourcemeta.com/example-schema");
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto identify(const JSON &schema, const SchemaResolver &resolver,
              const std::optional<std::string> &default_dialect = std::nullopt,
              const std::optional<std::string> &default_id = std::nullopt)
    -> std::optional<std::string>;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::core::identify if you know the base dialect
/// of the schema.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto identify(const JSON &schema, const std::string &base_dialect,
              const std::optional<std::string> &default_id = std::nullopt)
    -> std::optional<std::string>;

/// @ingroup jsonschema
///
/// This function removes the top-level URI identifier of the given schema, if
/// any, given you know its base dialect. It is the caller responsibility to
/// ensure the schema doesn't perform relative references that might have
/// depended on such top-level identifier. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// sourcemeta::core::anonymize(document,
///   "https://json-schema.org/draft/2020-12/schema");
///
/// std::optional<std::string> id{sourcemeta::core::identify(
///   document, sourcemeta::core::schema_resolver)};
/// assert(!id.has_value());
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto anonymize(JSON &schema, const std::string &base_dialect) -> void;

/// @ingroup jsonschema
///
/// This function sets the identifier of a schema, replacing the existing one,
/// if any. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// sourcemeta::core::reidentify(document,
///   "https://example.com/my-new-id",
///   sourcemeta::core::schema_resolver);
///
/// std::optional<std::string> id{sourcemeta::core::identify(
///   document, sourcemeta::core::schema_resolver)};
/// assert(id.has_value());
/// assert(id.value() == "https://example.com/my-new-id");
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto reidentify(
    JSON &schema, const std::string &new_identifier,
    const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect = std::nullopt) -> void;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::core::reidentify if you know the base
/// dialect of the schema.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto reidentify(JSON &schema, const std::string &new_identifier,
                const std::string &base_dialect) -> void;

/// @ingroup jsonschema
///
/// Get the dialect URI that corresponds to a JSON Schema instance.
/// The result is empty if the dialect cannot be determined. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const std::optional<std::string>
///   dialect{sourcemeta::core::dialect(document)};
/// assert(dialect.has_value());
/// assert(dialect.value() ==
///   "https://json-schema.org/draft/2020-12/schema");
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto dialect(const JSON &schema,
             const std::optional<std::string> &default_dialect = std::nullopt)
    -> std::optional<std::string>;

/// @ingroup jsonschema
///
/// Get the metaschema document that describes the given schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const sourcemeta::core::JSON metaschema{
///   sourcemeta::core::metaschema(
///     document, sourcemeta::core::schema_resolver)};
///
/// sourcemeta::core::prettify(metaschema, std::cout);
/// std::cout << std::endl;
/// ```
///
/// This function will throw if the metaschema cannot be determined or resolved.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto metaschema(
    const JSON &schema, const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect = std::nullopt) -> JSON;

/// @ingroup jsonschema
///
/// Get the URI of the base dialect that applies to the given schema. If you set
/// a default dialect URI, this will be used if the given schema does not
/// declare the `$schema` keyword. The result of this function is unset
/// if its base dialect could not be determined. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const std::optional<std::string> base_dialect{
///   sourcemeta::core::base_dialect(
///     document, sourcemeta::core::schema_resolver)};
///
/// assert(base_dialect.has_value());
/// assert(base_dialect.value() ==
///   "https://json-schema.org/draft/2020-12/schema");
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto base_dialect(const JSON &schema, const SchemaResolver &resolver,
                  const std::optional<std::string> &default_dialect =
                      std::nullopt) -> std::optional<std::string>;

/// @ingroup jsonschema
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
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const auto vocabularies{
///   sourcemeta::core::vocabularies(
///     document, sourcemeta::core::schema_resolver)};
///
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/core"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/applicator"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/unevaluated"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/validation"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/meta-data"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/format-annotation"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/content"));
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto vocabularies(const JSON &schema, const SchemaResolver &resolver,
                  const std::optional<std::string> &default_dialect =
                      std::nullopt) -> Vocabularies;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::core::vocabularies based on the base
/// dialect and dialect URI.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto vocabularies(const SchemaResolver &resolver,
                  const std::string &base_dialect, const std::string &dialect)
    -> Vocabularies;

/// @ingroup jsonschema
///
/// Format a JSON Schema document by reordering all object properties throughout
/// the entire document according to an opinionated JSON Schema aware ordering.
/// This function modifies the schema in-place. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
/// #include <sstream>
///
/// sourcemeta::core::JSON schema =
///   sourcemeta::core::parse_json(
///     "{ \"type\": \"string\", \"minLength\": 3 }");
/// sourcemeta::core::format(schema, sourcemeta::core::schema_walker,
///                          sourcemeta::core::schema_resolver);
/// std::ostringstream stream;
/// sourcemeta::core::prettify(schema, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto format(JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            const std::optional<JSON::String> &default_dialect = std::nullopt)
    -> void;

/// @ingroup jsonschema
///
/// Given a schema identifier, this function creates a JSON Schema wrapper that
/// references such schema. This is useful when trying to validate an instance
/// against a specific subset of a schema, as the wrapper allows you to make use
/// of JSON Schema referencing to get there without reinventing the wheel. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON result =
///   sourcemeta::core::wrap("https://www.example.com#/foo/bar");
///
/// sourcemeta::core::prettify(result, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto wrap(const JSON::String &identifier) -> JSON;

/// @ingroup jsonschema
///
/// Wrap a schema to only access one of its subschemas. This is useful if you
/// want to perform validation only a specific part of the schemaw without
/// having to reinvent the wheel. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "type": "string" }
/// })JSON");
///
/// const sourcemeta::core::JSON result =
///   sourcemeta::core::wrap(document, { "items" },
///     sourcemeta::core::schema_resolver);
///
/// sourcemeta::core::prettify(result, std::cerr);
/// std::cerr << "\n";
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto wrap(const JSON &schema, const Pointer &pointer,
          const SchemaResolver &resolver,
          const std::optional<std::string> &default_dialect = std::nullopt)
    -> JSON;

/// @ingroup jsonschema
///
/// Parse the value of a JSON Schema `type` keyword (which can be a string or
/// an array of strings) into a set of native JSON types. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const auto type{sourcemeta::core::parse_json(R"JSON([ "string", "null"
/// ])JSON")}; const auto types{sourcemeta::core::parse_schema_type(type)};
/// assert(types.test(
///     static_cast<std::size_t>(sourcemeta::core::JSON::Type::String)));
/// assert(types.test(
///     static_cast<std::size_t>(sourcemeta::core::JSON::Type::Null)));
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto parse_schema_type(const JSON &type) -> JSON::TypeSet;

} // namespace sourcemeta::core

#endif
