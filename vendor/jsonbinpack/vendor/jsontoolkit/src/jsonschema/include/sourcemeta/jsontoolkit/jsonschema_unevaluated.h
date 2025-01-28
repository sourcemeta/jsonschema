#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_UNEVALUATED_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_UNEVALUATED_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#include <sourcemeta/jsontoolkit/jsonschema_export.h>
#endif

#include <map>           // std::map
#include <string>        // std::string
#include <unordered_set> // std::set

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>

#include <sourcemeta/jsontoolkit/jsonschema_frame.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema
struct UnevaluatedEntry {
  /// The absolute pointers of the static keyword dependencies
  std::set<Pointer> static_dependencies;
  /// The absolute pointers of the static keyword dependencies
  std::set<Pointer> dynamic_dependencies;
  /// Whether the entry cannot be fully resolved, which means
  /// there might be unknown dynamic dependencies
  bool unresolved{false};
};

/// @ingroup jsonschema
/// The flattened set of unevaluated cases in the schema by absolute URI
using UnevaluatedEntries = std::map<std::string, UnevaluatedEntry>;

// TODO: Eventually generalize this to list every cross-dependency between
// keywords, supporting extensibility of custom vocabularies too

/// @ingroup jsonschema
///
/// This function performs a static analysis pass on `unevaluatedProperties` and
/// `unevaluatedItems` occurences throughout the entire schema (if any).
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "unevaluatedProperties": false
/// })JSON");
///
/// sourcemeta::jsontoolkit::Frame frame;
/// frame.analyse(document,
///   sourcemeta::jsontoolkit::default_schema_walker,
///   sourcemeta::jsontoolkit::official_resolver);
/// const auto result{sourcemeta::jsontoolkit::unevaluated(
///     schema, frame,
///     sourcemeta::jsontoolkit::default_schema_walker,
///     sourcemeta::jsontoolkit::official_resolver)};
///
/// assert(result.contains("#/unevaluatedProperties"));
/// assert(!result.at("#/unevaluatedProperties").dynamic);
/// assert(result.at("#/unevaluatedProperties").dependencies.empty());
/// ```
auto SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
unevaluated(const JSON &schema, const Frame &frame, const SchemaWalker &walker,
            const SchemaResolver &resolver) -> UnevaluatedEntries;

} // namespace sourcemeta::jsontoolkit

#endif
