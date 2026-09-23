#ifndef SOURCEMETA_CORE_JSONSCHEMA_H_
#define SOURCEMETA_CORE_JSONSCHEMA_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonschema_error.h>
#include <sourcemeta/core/jsonschema_frame.h>
#include <sourcemeta/core/jsonschema_types.h>
// NOLINTEND(misc-include-cleaner)

#include <cstdint>     // std::uint8_t, std::uint64_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

/// @defgroup jsonschema JSON Schema
/// @brief The foundational utilities for operating on JSON Schema documents
/// across dialects.
///
/// This module is not a JSON Schema evaluator and does not aim to become one.
/// It offers the building blocks that any operation on a schema needs,
/// independently of what that operation is: identification, dialect and
/// vocabulary detection, resolution of remote schemas, keyword classification
/// across dialects, framing a schema into the locations and references it
/// declares, and bundling a schema into a self-contained document. Evaluation
/// is only one of the consumers of these utilities, alongside linting,
/// transformation, code generation, and documentation tooling.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonschema
/// A default resolver that relies on built-in official schemas. The schemas
/// are parsed once and handed back by reference, so they must not outlive the
/// program.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_resolver(const std::string_view identifier) -> SchemaResolverResult;

/// @ingroup jsonschema
/// Check if a given identifier corresponds to a known built-in schema
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_is_known(const std::string_view identifier) noexcept -> bool;

/// @ingroup jsonschema
/// Check if a given URI corresponds to an official schema released by the
/// JSON Schema organisation
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_is_official(const std::string_view identifier) noexcept -> bool;

/// @ingroup jsonschema
/// A default schema walker with support for a wide range of drafts
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_walker(const std::string_view keyword,
                   const SchemaVocabularies &vocabularies)
    -> const SchemaWalkerResult &;

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
/// sourcemeta::core::schema_reidentify(document,
///   "https://example.com/my-new-id",
///   sourcemeta::core::schema_resolver);
///
/// assert(document.at("$id").to_string() ==
///   "https://example.com/my-new-id");
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_reidentify(sourcemeta::core::JSON &schema,
                       std::string_view new_identifier,
                       const SchemaResolver &resolver,
                       std::string_view default_dialect = "") -> void;

/// @ingroup jsonschema
///
/// The keyword that carries a schema identifier in the given base dialect.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::schema_identifier_keyword(
///     sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12) == "$id");
/// assert(sourcemeta::core::schema_identifier_keyword(
///     sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4) == "id");
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_identifier_keyword(const SchemaBaseDialect base_dialect)
    -> std::string_view;

/// @ingroup jsonschema
///
/// A shortcut to sourcemeta::core::schema_reidentify if you know the base
/// dialect of the schema.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_reidentify(sourcemeta::core::JSON &schema,
                       std::string_view new_identifier,
                       const SchemaBaseDialect base_dialect) -> void;

/// @ingroup jsonschema
///
/// This function reorders the properties of every subschema that the given
/// frame reports, following an opinionated JSON Schema aware order, modifying
/// the schema in place. Note that doing so invalidates the given frame, as the
/// locations it holds point into the schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// #include <iostream>
/// #include <sstream>
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "type": "string",
///   "minLength": 3,
///   "$schema": "https://json-schema.org/draft/2020-12/schema"
/// })JSON");
///
/// const sourcemeta::core::SchemaFrame frame{
///   sourcemeta::core::SchemaFrame::Mode::Locations, document,
///   sourcemeta::core::schema_walker,
///   sourcemeta::core::schema_resolver};
///
/// sourcemeta::core::schema_format(document, frame);
///
/// std::ostringstream stream;
/// sourcemeta::core::prettify(document, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_format(sourcemeta::core::JSON &schema, const SchemaFrame &frame)
    -> void;

/// @ingroup jsonschema
/// Everything bundling takes beyond the schema and how to read it
struct SchemaBundleOptions {
  /// A callback to report which schema got embedded and where, as the
  /// identifier that schema answers to and a pointer from the root of the
  /// schema being bundled. A schema that declares one of its own answers to
  /// that rather than to whichever URI it was resolved by, which are not
  /// always the same. The two are given separately because bundling picks a
  /// key that is free rather than one that matches, so the last token of that
  /// pointer is not always the identifier either
  using Callback = std::function<void(std::string_view,
                                      const sourcemeta::core::WeakPointer &)>;

  /// The strategies that the bundling process can follow
  enum class Mode : std::uint8_t {
    /// Embed every external reference, including any non-official
    /// meta-schemas that the schema or its dependencies declare, along
    /// with the dependencies of those meta-schemas
    NonOfficialMetaschemas,
    /// Embed every external reference, skipping meta-schema
    /// declarations entirely
    References
  };

  /// The strategy to follow
  Mode mode{Mode::NonOfficialMetaschemas};
  /// Where to embed what bundling pulls in
  std::optional<sourcemeta::core::Pointer> default_container;
  /// The paths to bundle within a schema wrapper
  SchemaFrame::Paths paths{sourcemeta::core::EMPTY_WEAK_POINTER};
  /// The base URI that the document was retrieved from, which a relative
  /// reference within any of the given paths resolves against. As with
  /// sourcemeta::core::SchemaFrame, this does not claim that the document
  /// declares an identifier, so bundling never writes it into the document
  std::string_view default_base;
  /// The maximum number of frame locations that analysis may register. How
  /// many schemas bundling ends up embedding follows from what the resolver
  /// hands back rather than from the schema the caller passed in, and every
  /// frame that bundling constructs spends from this one limit, throwing
  /// sourcemeta::core::SchemaFrameLimitError once it runs out. Note that a
  /// remote is copied out of the resolver before anything charges for it, so
  /// this bounds how many oversized schemas get copied rather than whether
  /// one does
  std::uint64_t max_locations{std::numeric_limits<std::uint64_t>::max()};
  /// A callback to report where each schema got embedded, which is the only
  /// way to know what a later call has to frame when bundling into a
  /// container that the dialect does not otherwise traverse
  Callback callback;
};

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
///     -> sourcemeta::core::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_resolver(identifier);
///   }
/// }
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::core::schema_bundle(document,
///   sourcemeta::core::schema_walker, test_resolver);
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
auto schema_bundle(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
                   const SchemaResolver &resolver,
                   std::string_view default_dialect = "",
                   std::string_view default_id = "",
                   const SchemaBundleOptions &options = {}) -> void;

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
///     -> sourcemeta::core::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_resolver(identifier);
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
///   sourcemeta::core::schema_bundle(document,
///     sourcemeta::core::schema_walker, test_resolver);
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
auto schema_bundle(const sourcemeta::core::JSON &schema,
                   const SchemaWalker &walker, const SchemaResolver &resolver,
                   std::string_view default_dialect = "",
                   std::string_view default_id = "",
                   const SchemaBundleOptions &options = {})
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::core

#endif
