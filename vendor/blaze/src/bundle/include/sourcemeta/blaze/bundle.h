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
#include <sourcemeta/blaze/frame.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
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
/// schema. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/bundle.h>
/// #include <sourcemeta/blaze/foundation.h>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> std::optional<sourcemeta::core::JSON> {
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
SOURCEMETA_BLAZE_BUNDLE_EXPORT
auto dependencies(const sourcemeta::core::JSON &schema,
                  const SchemaWalker &walker, const SchemaResolver &resolver,
                  const DependencyCallback &callback,
                  std::string_view default_dialect = "",
                  std::string_view default_id = "",
                  const SchemaFrame::Paths &paths = {
                      sourcemeta::core::empty_weak_pointer}) -> void;

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
///     -> std::optional<sourcemeta::core::JSON> {
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
SOURCEMETA_BLAZE_BUNDLE_EXPORT
auto bundle(sourcemeta::core::JSON &schema, const SchemaWalker &walker,
            const SchemaResolver &resolver, const BundleMode mode,
            std::string_view default_dialect = "",
            std::string_view default_id = "",
            const std::optional<sourcemeta::core::Pointer> &default_container =
                std::nullopt,
            const SchemaFrame::Paths &paths = {
                sourcemeta::core::empty_weak_pointer}) -> void;

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
///     -> std::optional<sourcemeta::core::JSON> {
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
SOURCEMETA_BLAZE_BUNDLE_EXPORT
auto bundle(
    const sourcemeta::core::JSON &schema, const SchemaWalker &walker,
    const SchemaResolver &resolver, const BundleMode mode,
    std::string_view default_dialect = "", std::string_view default_id = "",
    const std::optional<sourcemeta::core::Pointer> &default_container =
        std::nullopt,
    const SchemaFrame::Paths &paths = {sourcemeta::core::empty_weak_pointer})
    -> sourcemeta::core::JSON;

} // namespace sourcemeta::blaze

#endif
