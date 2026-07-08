#ifndef SOURCEMETA_BLAZE_CANONICALIZER_H_
#define SOURCEMETA_BLAZE_CANONICALIZER_H_

/// @defgroup canonicalizer Canonicalizer
/// @brief Rewrite a JSON Schema into its analysis-canonical form.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/canonicalizer.h>
/// ```

#ifndef SOURCEMETA_BLAZE_CANONICALIZER_EXPORT
#include <sourcemeta/blaze/canonicalizer_export.h>
#endif

#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>

#include <sourcemeta/core/json.h>

#include <string_view> // std::string_view

namespace sourcemeta::blaze {

/// @ingroup canonicalizer
/// Rewrite the given schema, in place, into its analysis-canonical form. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/blaze/canonicalizer.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// auto schema = sourcemeta::core::parse_json(R"JSON({
///   "$schema": "http://json-schema.org/draft-03/schema#",
///   "type": "boolean"
/// })JSON");
///
/// sourcemeta::blaze::canonicalize(schema, sourcemeta::blaze::schema_walker,
///                                 sourcemeta::blaze::schema_resolver);
/// ```
SOURCEMETA_BLAZE_CANONICALIZER_EXPORT
auto canonicalize(sourcemeta::core::JSON &schema,
                  const sourcemeta::blaze::SchemaWalker &walker,
                  const sourcemeta::blaze::SchemaResolver &resolver,
                  const std::string_view default_dialect = "",
                  const std::string_view default_id = "") -> void;

} // namespace sourcemeta::blaze

#endif
