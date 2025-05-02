#ifndef SOURCEMETA_BLAZE_COMPILER_UNEVALUATED_H
#define SOURCEMETA_BLAZE_COMPILER_UNEVALUATED_H

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <map> // std::map
#include <set> // std::set

// TODO: Eventually this file should dissapear and move this analysis as part of
// framing

namespace sourcemeta::blaze {

/// @ingroup compiler
struct SchemaUnevaluatedEntry {
  /// The absolute pointers of the static keyword dependencies
  std::set<sourcemeta::core::Pointer> static_dependencies;
  /// The absolute pointers of the static keyword dependencies
  std::set<sourcemeta::core::Pointer> dynamic_dependencies;
  /// Whether the entry cannot be fully resolved, which means
  /// there might be unknown dynamic dependencies
  bool unresolved{false};
};

/// @ingroup compiler
/// The flattened set of unevaluated cases in the schema by absolute URI
using SchemaUnevaluatedEntries =
    std::map<sourcemeta::core::JSON::String, SchemaUnevaluatedEntry>;

/// @ingroup compiler
///
/// This function performs a static analysis pass on `unevaluatedProperties` and
/// `unevaluatedItems` occurences throughout the entire schema (if any).
/// ```
auto SOURCEMETA_BLAZE_COMPILER_EXPORT
unevaluated(const sourcemeta::core::JSON &schema,
            const sourcemeta::core::SchemaFrame &frame,
            const sourcemeta::core::SchemaWalker &walker,
            const sourcemeta::core::SchemaResolver &resolver)
    -> SchemaUnevaluatedEntries;

} // namespace sourcemeta::blaze

#endif
