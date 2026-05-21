#ifndef SOURCEMETA_JSONBINPACK_COMPILER_H_
#define SOURCEMETA_JSONBINPACK_COMPILER_H_

#ifndef SOURCEMETA_JSONBINPACK_COMPILER_EXPORT
#include <sourcemeta/jsonbinpack/compiler_export.h>
#endif

/// @defgroup compiler Compiler
/// @brief The built-time schema compiler of JSON BinPack
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/jsonbinpack/compiler.h>
/// ```

#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/json.h>

#include <string_view> // std::string_view

namespace sourcemeta::jsonbinpack {

/// @ingroup compiler
///
/// Compile a JSON Schema into an encoding schema. Keep in mind this function
/// mutates the input schema. For example:
///
/// ```cpp
/// #include <sourcemeta/binpack/compiler.h>
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// #include <iostream>
///
/// auto schema{sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON")};
///
/// sourcemeta::jsonbinpack::compile(
///     schema, sourcemeta::blaze::schema_walker,
///     sourcemeta::blaze::schema_resolver);
///
/// sourcemeta::core::prettify(schema, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_JSONBINPACK_COMPILER_EXPORT
auto compile(sourcemeta::core::JSON &schema,
             const sourcemeta::blaze::SchemaWalker &walker,
             const sourcemeta::blaze::SchemaResolver &resolver,
             std::string_view default_dialect = "") -> void;

/// @ingroup compiler
///
/// Transform a JSON Schema into its canonical form to prepare it for
/// compilation. Keep in mind this function mutates the input schema. Also, the
/// `compile` function already performs canonicalization. This function is
/// exposed mainly for debugging and testing purposes. For example:
///
/// ```cpp
/// #include <sourcemeta/binpack/compiler.h>
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// #include <iostream>
///
/// auto schema{sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON")};
///
/// sourcemeta::jsonbinpack::canonicalize(
///     schema, sourcemeta::blaze::schema_walker,
///     sourcemeta::blaze::schema_resolver);
///
/// sourcemeta::core::prettify(schema, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_JSONBINPACK_COMPILER_EXPORT
auto canonicalize(sourcemeta::core::JSON &schema,
                  const sourcemeta::blaze::SchemaWalker &walker,
                  const sourcemeta::blaze::SchemaResolver &resolver,
                  std::string_view default_dialect = "") -> void;

} // namespace sourcemeta::jsonbinpack

#endif
