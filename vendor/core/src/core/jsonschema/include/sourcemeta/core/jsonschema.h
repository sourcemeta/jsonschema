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
/// across dialects, and framing a schema into the locations and references it
/// declares. Evaluation is only one of the consumers of these utilities,
/// alongside bundling, linting, transformation, code generation, and
/// documentation tooling.
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
/// A shortcut to sourcemeta::core::schema_reidentify if you know the base
/// dialect of the schema.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto schema_reidentify(sourcemeta::core::JSON &schema,
                       std::string_view new_identifier,
                       const SchemaBaseDialect base_dialect) -> void;

} // namespace sourcemeta::core

#endif
