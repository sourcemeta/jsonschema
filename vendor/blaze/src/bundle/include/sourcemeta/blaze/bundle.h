#ifndef SOURCEMETA_BLAZE_BUNDLE_H_
#define SOURCEMETA_BLAZE_BUNDLE_H_

/// @defgroup bundle Bundle
/// @brief Bundle JSON Schemas by inlining their external references.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/bundle.h>
/// ```

#ifndef SOURCEMETA_BLAZE_BUNDLE_EXPORT
#include <sourcemeta/blaze/bundle_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/blaze/foundation.h>

#include <cstdint>     // std::uint8_t, std::uint64_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

/// @ingroup bundle
/// A callback to get dependency information
/// - Origin URI (empty if none)
/// - Pointer (reference keyword from the origin)
/// - Target URI
/// - Target schema
using DependencyCallback =
    std::function<void(std::string_view, const sourcemeta::core::WeakPointer &,
                       std::string_view, const sourcemeta::core::JSON &)>;

/// @ingroup bundle
/// The strategies that the bundling process can follow
enum class BundleMode : std::uint8_t {
  /// Embed every external reference, including any non-official
  /// meta-schemas that the schema or its dependencies declare, along
  /// with the dependencies of those meta-schemas
  NonOfficialMetaschemas,
  /// Embed every external reference, skipping meta-schema
  /// declarations entirely
  References
};

/// @ingroup bundle
///
/// This function recursively traverses and reports the external references in a
/// schema. References to official schemas are reported but not traversed into,
/// as official schemas can only reference other official schemas. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/bundle.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> sourcemeta::blaze::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::blaze::schema_resolver(identifier);
///   }
/// }
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::blaze::dependencies(document,
///   sourcemeta::blaze::schema_walker, test_resolver,
///   [](const auto &origin,
///      const auto &pointer,
///      const auto &target,
///      const auto &schema) {
///     // Do something with the information
///   });
/// ```
///
/// How many schemas this ends up analysing follows from what the resolver
/// hands back rather than from the schema the caller passed in, so pass
/// `max_locations` to bound it. Every frame that this constructs spends from
/// that one limit, throwing sourcemeta::blaze::SchemaFrameLimitError once it
/// runs out. See sourcemeta::blaze::SchemaFrame for what the unit counts and
/// what it leaves to the caller
SOURCEMETA_BLAZE_BUNDLE_EXPORT
auto dependencies(
    const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver, const DependencyCallback &callback,
    std::string_view default_dialect = "", std::string_view default_id = "",
    const SchemaFrame::Paths &paths = {sourcemeta::core::EMPTY_WEAK_POINTER},
    std::uint64_t max_locations = std::numeric_limits<std::uint64_t>::max())
    -> void;

/// @ingroup bundle
///
/// This function bundles a JSON Schema (starting from Draft 4) by embedding
/// every remote reference into the top level schema resource, handling circular
/// dependencies and more. This overload mutates the input schema.  For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/bundle.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> sourcemeta::blaze::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::blaze::schema_resolver(identifier);
///   }
/// }
///
/// sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// sourcemeta::blaze::bundle(document,
///   sourcemeta::blaze::schema_walker, test_resolver,
///   sourcemeta::blaze::BundleMode::NonOfficialMetaschemas);
///
/// const sourcemeta::core::JSON expected =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" },
///   "$defs": {
///     "https://www.example.com/test": {
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     }
///   }
/// })JSON");
///
/// assert(document == expected);
/// ```
///
/// How many schemas this ends up embedding follows from what the resolver
/// hands back rather than from the schema the caller passed in, so pass
/// `max_locations` to bound it. Every frame that bundling constructs spends
/// from that one limit, throwing sourcemeta::blaze::SchemaFrameLimitError once
/// it runs out, which bounds how many remote schemas this embeds and how deep
/// it recurses along with how much framing it does. Note that a remote is
/// copied out of the resolver before anything charges for it, so the limit
/// bounds how many oversized schemas get copied rather than whether one does
SOURCEMETA_BLAZE_BUNDLE_EXPORT
auto bundle(
    sourcemeta::core::JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver, const BundleMode mode,
    std::string_view default_dialect = "", std::string_view default_id = "",
    const std::optional<sourcemeta::core::Pointer> &default_container =
        std::nullopt,
    const SchemaFrame::Paths &paths = {sourcemeta::core::EMPTY_WEAK_POINTER},
    std::uint64_t max_locations = std::numeric_limits<std::uint64_t>::max())
    -> void;

/// @ingroup bundle
///
/// This function bundles a JSON Schema (starting from Draft 4) by embedding
/// every remote reference into the top level schema resource, handling circular
/// dependencies and more. This overload returns a new schema, without mutating
/// the input schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/bundle.h>
/// #include <sourcemeta/blaze/foundation.h>
/// #include <cassert>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> sourcemeta::blaze::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::blaze::schema_resolver(identifier);
///   }
/// }
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" }
/// })JSON");
///
/// const sourcemeta::core::JSON result =
///   sourcemeta::blaze::bundle(document,
///     sourcemeta::blaze::schema_walker, test_resolver,
///     sourcemeta::blaze::BundleMode::NonOfficialMetaschemas);
///
/// const sourcemeta::core::JSON expected =
///     sourcemeta::core::parse_json(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$ref": "https://www.example.com/test" },
///   "$defs": {
///     "https://www.example.com/test": {
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     }
///   }
/// })JSON");
///
/// assert(result == expected);
/// ```
///
/// As with the mutating overload, pass `max_locations` to bound how much
/// analysis an untrusted schema and whatever the resolver hands back for it
/// may cost
SOURCEMETA_BLAZE_BUNDLE_EXPORT
auto bundle(
    const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver, const BundleMode mode,
    std::string_view default_dialect = "", std::string_view default_id = "",
    const std::optional<sourcemeta::core::Pointer> &default_container =
        std::nullopt,
    const SchemaFrame::Paths &paths = {sourcemeta::core::EMPTY_WEAK_POINTER},
    std::uint64_t max_locations = std::numeric_limits<std::uint64_t>::max())
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
