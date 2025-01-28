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

namespace sourcemeta::core {

// TODO: Revise the category names to make incompatibilities more obvious

/// @ingroup alterschema
/// The category of a built-in transformation rule
enum class AlterSchemaCategory {
  /// Rules that detect clear anti-patterns that should not be happening on the
  /// first place
  AntiPattern,

  /// Rules that simplify the given schema for both human readability and
  /// performance
  Simplify,

  /// Rules that take advantage of syntax sugar to improve human readability of
  /// a schema. As its name implies, this category is incompatible with
  /// `Desugar`.
  SyntaxSugar,

  /// Rules that simplify keywords that are syntax sugar to other keywords,
  /// potentially decreasing human readability in favor of explicitness
  /// As its name implies, this category is incompatible with `SyntaxSugar`.
  Desugar,

  /// Rules that remove schema redundancies that do not contribute to the
  /// schema.
  /// This category is incompatible with `Implicit`
  Redundant,

  /// Rules that surface implicit constraints. This category is incompatible
  /// with `Redundant`
  Implicit,

  /// Rules that remove keywords that are superfluous and take no effect on the
  /// given schema
  Superfluous
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
///   sourcemeta::core::AlterSchemaCategory::SyntaxSugar);
///
/// auto schema = sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "foo": 1,
///   "items": {
///     "type": "string",
///     "foo": 2
///   }
/// })JSON");
///
/// bundle.apply(schema, sourcemeta::core::default_schema_walker,
///              sourcemeta::core::official_resolver);
/// ```
SOURCEMETA_CORE_ALTERSCHEMA_EXPORT
auto add(SchemaTransformer &bundle, const AlterSchemaCategory category) -> void;

} // namespace sourcemeta::core

#endif
