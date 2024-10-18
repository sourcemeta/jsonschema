#ifndef SOURCEMETA_ALTERSCHEMA_LINTER_H_
#define SOURCEMETA_ALTERSCHEMA_LINTER_H_

/// @defgroup linter Linter
/// @brief A growing collection of linter rules.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/alterschema/linter.h>
/// ```

#ifndef SOURCEMETA_ALTERSCHEMA_LINTER_EXPORT
#include <sourcemeta/alterschema/linter_export.h>
#endif

#include <sourcemeta/alterschema/engine.h>

namespace sourcemeta::alterschema {

// TODO: Revise the category names to make incompatibilities more obvious

/// @ingroup linter
/// The category of a built-in transformation rule
enum class LinterCategory {
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

/// @ingroup linter
/// Add a set of built-in linter rules given a category. For example:
///
/// ```cpp
/// #include <sourcemeta/alterschema/engine.h>
/// #include <sourcemeta/alterschema/linter.h>
///
/// sourcemeta::alterschema::Bundle bundle;
///
/// sourcemeta::alterschema::add(bundle,
///   sourcemeta::alterschema::LinterCategory::SyntaxSugar);
///
/// auto schema = sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "foo": 1,
///   "items": {
///     "type": "string",
///     "foo": 2
///   }
/// })JSON");
///
/// bundle.apply(schema, sourcemeta::jsontoolkit::default_schema_walker,
///              sourcemeta::jsontoolkit::official_resolver);
/// ```
SOURCEMETA_ALTERSCHEMA_LINTER_EXPORT
auto add(Bundle &bundle, const LinterCategory category) -> void;

} // namespace sourcemeta::alterschema

#endif
