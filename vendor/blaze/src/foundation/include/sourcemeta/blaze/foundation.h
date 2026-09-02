#ifndef SOURCEMETA_BLAZE_FOUNDATION_H_
#define SOURCEMETA_BLAZE_FOUNDATION_H_

#ifndef SOURCEMETA_BLAZE_FOUNDATION_EXPORT
#include <sourcemeta/blaze/foundation_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/foundation_error.h>
#include <sourcemeta/blaze/foundation_frame.h>
#include <sourcemeta/blaze/foundation_types.h>
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
/// A default resolver that relies on built-in official schemas. The schemas
/// are parsed once and handed back by reference, so they must not outlive the
/// program.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_resolver(const std::string_view identifier) -> SchemaResolverResult;

/// @ingroup foundation
/// Check if a given identifier corresponds to a known built-in schema
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_is_known(const std::string_view identifier) noexcept -> bool;

/// @ingroup foundation
/// Check if a given URI corresponds to an official schema released by the
/// JSON Schema organisation
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_is_official(const std::string_view identifier) noexcept -> bool;

/// @ingroup foundation
/// A default schema walker with support for a wide range of drafts
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_walker(const std::string_view keyword,
                   const SchemaVocabularies &vocabularies)
    -> const SchemaWalkerResult &;

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
/// sourcemeta::blaze::schema_reidentify(document,
///   "https://example.com/my-new-id",
///   sourcemeta::blaze::schema_resolver);
///
/// const auto id{sourcemeta::blaze::identify(
///   document, sourcemeta::blaze::schema_resolver)};
/// assert(!id.empty());
/// assert(id == "https://example.com/my-new-id");
/// ```
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_reidentify(sourcemeta::core::JSON &schema,
                       std::string_view new_identifier,
                       const SchemaResolver &resolver,
                       std::string_view default_dialect = "") -> void;

/// @ingroup foundation
///
/// A shortcut to sourcemeta::blaze::schema_reidentify if you know the base
/// dialect of the schema.
SOURCEMETA_BLAZE_FOUNDATION_EXPORT
auto schema_reidentify(sourcemeta::core::JSON &schema,
                       std::string_view new_identifier,
                       const SchemaBaseDialect base_dialect) -> void;

} // namespace sourcemeta::blaze

#endif
