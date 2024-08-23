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

#include "linter_export.h"

#include <sourcemeta/alterschema/engine.h>

namespace sourcemeta::alterschema {

/// @ingroup linter
/// The category of a built-in transformation rule
enum class LinterCategory {
  /// Rules that make use of newer features within the same dialect
  Modernize,

  /// Rules that detect common anti-patterns
  AntiPattern,

  /// Rules that simplify the given schema
  Simplify,

  /// Rules that remove schema redundancies
  Redundant
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
///   sourcemeta::alterschema::LinterCategory::Modernize);
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
