#ifndef SOURCEMETA_BLAZE_CONVERT_H_
#define SOURCEMETA_BLAZE_CONVERT_H_

/// @defgroup convert Convert
/// @brief Convert a JSON Schema from one dialect to another.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/convert.h>
/// ```

#ifndef SOURCEMETA_BLAZE_CONVERT_EXPORT
#include <sourcemeta/blaze/convert_export.h>
#endif

#include <sourcemeta/blaze/convert_error.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

/// @ingroup convert
/// The dialect to convert a schema to
enum class ConvertTarget : std::uint8_t {
  /// JSON Schema Draft 4
  Draft4,

  /// JSON Schema Draft 6
  Draft6,

  /// JSON Schema Draft 7
  Draft7,

  /// JSON Schema 2019-09
  Draft201909,

  /// JSON Schema 2020-12
  Draft202012,
};

/// @ingroup convert
/// Convert the given schema, in place, to the given dialect. Only upgrades are
/// supported, so a schema already on that dialect or a newer one is left as
/// is. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/convert.h>
/// #include <sourcemeta/core/jsonschema.h>
///
/// auto schema = sourcemeta::core::parse_json(R"JSON({
///   "$schema": "http://json-schema.org/draft-04/schema#",
///   "type": "string"
/// })JSON");
///
/// sourcemeta::blaze::convert(schema, sourcemeta::core::schema_walker,
///                            sourcemeta::core::schema_resolver,
///                            sourcemeta::blaze::ConvertTarget::Draft202012);
/// ```
SOURCEMETA_BLAZE_CONVERT_EXPORT
auto convert(sourcemeta::core::JSON &schema,
             const sourcemeta::core::SchemaWalker &walker,
             const sourcemeta::core::SchemaResolver &resolver,
             const ConvertTarget target,
             const std::string_view default_dialect = "",
             const std::string_view default_id = "",
             const bool is_metaschema = false) -> void;

} // namespace sourcemeta::blaze

#endif
