#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#include <sourcemeta/jsontoolkit/jsonschema_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema_anchor.h>
#include <sourcemeta/jsontoolkit/jsonschema_bundle.h>
#include <sourcemeta/jsontoolkit/jsonschema_error.h>
#include <sourcemeta/jsontoolkit/jsonschema_reference.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <map>      // std::map
#include <optional> // std::optional
#include <string>   // std::string

/// @defgroup jsonschema JSON Schema
/// @brief A set of JSON Schema utilities across draft versions.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// ```

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema
///
/// This function returns true if the given JSON instance is of a
/// schema-compatible type: an object or a boolean. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document{true};
/// assert(sourcemeta::jsontoolkit::is_schema(document));
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto is_schema(const JSON &schema) -> bool;

/// @ingroup jsonschema
/// The strategy to follow when attempting to identify a schema
enum class IdentificationStrategy : std::uint8_t {
  /// Only proceed if we can guarantee the identifier is valid
  Strict,

  /// Attempt to guess even if we don't know the base dialect
  Loose
};

/// @ingroup jsonschema
///
/// This function returns the URI identifier of the given schema, if any. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// std::optional<std::string> id{sourcemeta::jsontoolkit::identify(
///   document, sourcemeta::jsontoolkit::official_resolver)};
/// assert(id.has_value());
/// assert(id.value() == "https://sourcemeta.com/example-schema");
/// ```
///
/// You can opt-in to a loose identification strategy to attempt to play a
/// guessing game. Often useful if you have a schema without a dialect and you
/// want to at least try to get something.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto identify(
    const JSON &schema, const SchemaResolver &resolver,
    const IdentificationStrategy strategy = IdentificationStrategy::Strict,
    const std::optional<std::string> &default_dialect = std::nullopt,
    const std::optional<std::string> &default_id = std::nullopt)
    -> std::optional<std::string>;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::jsontoolkit::identify if you know the base dialect
/// of the schema.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
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
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// sourcemeta::jsontoolkit::anonymize(document,
///   "https://json-schema.org/draft/2020-12/schema");
///
/// std::optional<std::string> id{sourcemeta::jsontoolkit::identify(
///   document, sourcemeta::jsontoolkit::official_resolver)};
/// assert(!id.has_value());
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto anonymize(JSON &schema, const std::string &base_dialect) -> void;

/// @ingroup jsonschema
///
/// This function sets the identifier of a schema, replacing the existing one,
/// if any. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema"
/// })JSON");
///
/// sourcemeta::jsontoolkit::reidentify(document,
///   "https://example.com/my-new-id",
///   sourcemeta::jsontoolkit::official_resolver);
///
/// std::optional<std::string> id{sourcemeta::jsontoolkit::identify(
///   document, sourcemeta::jsontoolkit::official_resolver)};
/// assert(id.has_value());
/// assert(id.value() == "https://example.com/my-new-id");
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto reidentify(
    JSON &schema, const std::string &new_identifier,
    const SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect = std::nullopt) -> void;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::jsontoolkit::reidentify if you know the base
/// dialect of the schema.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto reidentify(JSON &schema, const std::string &new_identifier,
                const std::string &base_dialect) -> void;

/// @ingroup jsonschema
///
/// Get the dialect URI that corresponds to a JSON Schema instance.
/// The result is empty if the dialect cannot be determined. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const std::optional<std::string>
///   dialect{sourcemeta::jsontoolkit::dialect(document)};
/// assert(dialect.has_value());
/// assert(dialect.value() ==
///   "https://json-schema.org/draft/2020-12/schema");
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto dialect(const JSON &schema,
             const std::optional<std::string> &default_dialect = std::nullopt)
    -> std::optional<std::string>;

/// @ingroup jsonschema
///
/// Get the metaschema document that describes the given schema. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <iostream>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const sourcemeta::jsontoolkit::JSON metaschema{
///   sourcemeta::jsontoolkit::metaschema(
///     document, sourcemeta::jsontoolkit::official_resolver)};
///
/// sourcemeta::jsontoolkit::prettify(metaschema, std::cout);
/// std::cout << std::endl;
/// ```
///
/// This function will throw if the metaschema cannot be determined or resolved.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
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
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const std::optional<std::string> base_dialect{
///   sourcemeta::jsontoolkit::base_dialect(
///     document, sourcemeta::jsontoolkit::official_resolver)};
///
/// assert(base_dialect.has_value());
/// assert(base_dialect.value() ==
/// "https://json-schema.org/draft/2020-12/schema");
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
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
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "object"
/// })JSON");
///
/// const std::map<std::string, bool> vocabularies{
///   sourcemeta::jsontoolkit::vocabularies(
///     document, sourcemeta::jsontoolkit::official_resolver)};
///
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/core"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/applicator"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/unevaluated"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/validation"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/meta-data"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/format-annotation"));
/// assert(vocabularies.at("https://json-schema.org/draft/2020-12/vocab/content"));
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto vocabularies(const JSON &schema, const SchemaResolver &resolver,
                  const std::optional<std::string> &default_dialect =
                      std::nullopt) -> std::map<std::string, bool>;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::jsontoolkit::vocabularies based on the base
/// dialect and dialect URI.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto vocabularies(const SchemaResolver &resolver,
                  const std::string &base_dialect, const std::string &dialect)
    -> std::map<std::string, bool>;

/// @ingroup jsonschema
///
/// An opinionated JSON Schema aware key comparison for use with
/// sourcemeta::jsontoolkit::prettify or sourcemeta::jsontoolkit::stringify for
/// formatting purposes. For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::jsontoolkit::JSON document =
///   sourcemeta::jsontoolkit::parse(
///     "{ \"type\": \"string\", \"minLength\": 3 }");
/// std::ostringstream stream;
/// sourcemeta::jsontoolkit::prettify(document, stream,
///   sourcemeta::jsontoolkit::schema_format_compare);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto schema_format_compare(const JSON::String &left, const JSON::String &right)
    -> bool;

} // namespace sourcemeta::jsontoolkit

#endif
