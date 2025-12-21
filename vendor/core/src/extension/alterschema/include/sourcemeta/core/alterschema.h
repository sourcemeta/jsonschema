#ifndef SOURCEMETA_CORE_EXTENSION_ALTERSCHEMA_H_
#define SOURCEMETA_CORE_EXTENSION_ALTERSCHEMA_H_

/// @defgroup alterschema AlterSchema
/// @brief A growing collection of JSON Schema transformation rules.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/alterschema.h>
/// ```

#ifndef SOURCEMETA_CORE_ALTERSCHEMA_EXPORT
#include <sourcemeta/core/alterschema_export.h>
#endif

#include <sourcemeta/core/jsonschema.h>

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

/// @ingroup alterschema
/// The category of a built-in transformation rule
enum class AlterSchemaMode : std::uint8_t {
  /// Rules that simplify the given schema for both human readability and
  /// performance
  Linter,

  /// Rules that surface implicit constraints and simplifies keywords that
  /// are syntax sugar to other keywords, potentially decreasing human
  /// readability in favor of explicitness
  Canonicalizer,
};

/// @ingroup alterschema
/// Add a set of built-in schema transformation rules given a category. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// #include <sourcemeta/core/alterschema.h>
///
/// sourcemeta::core::SchemaTransformer bundle;
///
/// sourcemeta::core::add(bundle,
///   sourcemeta::core::AlterSchemaMode::Linter);
///
/// auto schema = sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "foo": 1,
///   "items": {
///     "type": "string",
///     "foo": 2
///   }
/// })JSON");
///
/// bundle.apply(schema, sourcemeta::core::schema_walker,
///              sourcemeta::core::schema_resolver);
/// ```
SOURCEMETA_CORE_ALTERSCHEMA_EXPORT
auto add(SchemaTransformer &bundle, const AlterSchemaMode mode) -> void;

} // namespace sourcemeta::core

#endif
