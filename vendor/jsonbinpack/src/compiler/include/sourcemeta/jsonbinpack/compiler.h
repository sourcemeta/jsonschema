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

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::jsonbinpack {

/// @ingroup compiler
///
/// Compile a JSON Schema into an encoding schema. Keep in mind this function
/// mutates the input schema. For example:
///
/// ```cpp
/// #include <sourcemeta/binpack/compiler.h>
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// #include <iostream>
///
/// auto schema{sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON")};
///
/// sourcemeta::jsonbinpack::compile(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver);
///
/// sourcemeta::jsontoolkit::prettify(schema, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_JSONBINPACK_COMPILER_EXPORT
auto compile(sourcemeta::jsontoolkit::JSON &schema,
             const sourcemeta::jsontoolkit::SchemaWalker &walker,
             const sourcemeta::jsontoolkit::SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect = std::nullopt)
    -> void;

/// @ingroup compiler
///
/// Transform a JSON Schema into its canonical form to prepare it for
/// compilation. Keep in mind this function mutates the input schema. Also, the
/// `compile` function already performs canonicalization. This function is
/// exposed mainly for debugging and testing purposes. For example:
///
/// ```cpp
/// #include <sourcemeta/binpack/compiler.h>
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
///
/// #include <iostream>
///
/// auto schema{sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON")};
///
/// sourcemeta::jsonbinpack::canonicalize(
///     schema, sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver);
///
/// sourcemeta::jsontoolkit::prettify(schema, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_JSONBINPACK_COMPILER_EXPORT
auto canonicalize(
    sourcemeta::jsontoolkit::JSON &schema,
    const sourcemeta::jsontoolkit::SchemaWalker &walker,
    const sourcemeta::jsontoolkit::SchemaResolver &resolver,
    const std::optional<std::string> &default_dialect = std::nullopt) -> void;

} // namespace sourcemeta::jsonbinpack

#endif
