#ifndef SOURCEMETA_CORE_EXTENSION_EDITORSCHEMA_H_
#define SOURCEMETA_CORE_EXTENSION_EDITORSCHEMA_H_

/// @defgroup editorschema EditorSchema
/// @brief A JSON Schema compatibility layer for code editors
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/editorschema.h>
/// ```

#ifndef SOURCEMETA_CORE_EDITORSCHEMA_EXPORT
#include <sourcemeta/core/editorschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup editorschema
///
/// This function aims to transform the schema (in potentially non-strictly
/// compliant manners) to workaround JSON Schema limitations of popular code
/// editors.
///
/// Keep in mind that this is not a real solution, but a workaround, and there
/// might be edge cases that we cannot cover. The real solution is for popular
/// editors to fix their JSON Schema language support.
///
/// Note that the input schema is expected to be already bundled.
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// auto schema = sourcemeta::core::parse_json(R"JSON({
///   "$id": "https://www.example.com/schema",
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$ref": "another"
/// })JSON");
///
/// sourcemeta::core::bundle(schema,
///   sourcemeta::core::schema_walker,
///   sourcemeta::core::schema_resolver);
/// sourcemeta::core::for_editor(schema,
///   sourcemeta::core::schema_walker,
///   sourcemeta::core::schema_resolver);
/// ```
SOURCEMETA_CORE_EDITORSCHEMA_EXPORT
auto for_editor(JSON &schema, const SchemaWalker &walker,
                const SchemaResolver &resolver,
                std::string_view default_dialect = "") -> void;

} // namespace sourcemeta::core

#endif
