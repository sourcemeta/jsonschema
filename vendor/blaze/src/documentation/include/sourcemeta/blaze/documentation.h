#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_H_
#define SOURCEMETA_BLAZE_DOCUMENTATION_H_

/// @defgroup documentation Documentation
/// @brief Generate human-readable documentation from a JSON Schema
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/documentation.h>
/// ```

#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT
#include <sourcemeta/blaze/documentation_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <string> // std::string

namespace sourcemeta::blaze {

/// @ingroup documentation
///
/// Generate documentation JSON from an input JSON Schema. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/documentation.h>
///
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// const sourcemeta::core::JSON schema =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// const auto documentation{sourcemeta::blaze::to_documentation(
///     schema, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver)};
/// ```
[[nodiscard]] SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT auto
to_documentation(const sourcemeta::core::JSON &schema,
                 const sourcemeta::core::SchemaWalker &walker,
                 const sourcemeta::core::SchemaResolver &resolver)
    -> sourcemeta::core::JSON;

/// @ingroup documentation
/// Render a documentation JSON object as an HTML string
[[nodiscard]] SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT auto
to_html(const sourcemeta::core::JSON &documentation) -> std::string;

} // namespace sourcemeta::blaze

#endif
