#ifndef SOURCEMETA_BLAZE_FORMAT_H_
#define SOURCEMETA_BLAZE_FORMAT_H_

/// @defgroup format Format
/// @brief Format JSON Schemas by reordering keywords into a canonical order.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/format.h>
/// ```

#ifndef SOURCEMETA_BLAZE_FORMAT_EXPORT
#include <sourcemeta/blaze/format_export.h>
#endif

#include <sourcemeta/blaze/foundation.h>

#include <sourcemeta/core/json.h>

#include <string_view> // std::string_view

namespace sourcemeta::blaze {

/// @ingroup format
///
/// Format a JSON Schema document by reordering all object properties throughout
/// the entire document according to an opinionated JSON Schema aware ordering.
/// This function modifies the schema in-place. For example:
///
/// ```cpp
/// #include <sourcemeta/blaze/format.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// #include <sourcemeta/core/json.h>
///
/// #include <iostream>
/// #include <sstream>
///
/// sourcemeta::core::JSON schema =
///   sourcemeta::core::parse_json(
///     "{ \"type\": \"string\", \"minLength\": 3 }");
/// sourcemeta::blaze::format(schema, sourcemeta::blaze::schema_walker,
///                           sourcemeta::blaze::schema_resolver);
/// std::ostringstream stream;
/// sourcemeta::core::prettify(schema, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_BLAZE_FORMAT_EXPORT
auto format(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver,
            std::string_view default_dialect = "") -> void;

} // namespace sourcemeta::blaze

#endif
