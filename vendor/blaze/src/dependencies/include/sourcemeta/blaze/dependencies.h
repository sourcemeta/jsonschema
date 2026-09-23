#ifndef SOURCEMETA_BLAZE_DEPENDENCIES_H_
#define SOURCEMETA_BLAZE_DEPENDENCIES_H_

/// @defgroup dependencies Dependencies
/// @brief Report the external references of JSON Schemas.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/dependencies.h>
/// ```

#ifndef SOURCEMETA_BLAZE_DEPENDENCIES_EXPORT
#include <sourcemeta/blaze/dependencies_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/core/jsonschema.h>

#include <cstdint>     // std::uint64_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

/// @ingroup dependencies
/// A callback to get dependency information
/// - Origin URI (empty if none)
/// - Pointer (reference keyword from the origin)
/// - Target URI
/// - Target schema
using DependencyCallback =
    std::function<void(std::string_view, const sourcemeta::core::WeakPointer &,
                       std::string_view, const sourcemeta::core::JSON &)>;

/// @ingroup dependencies
///
/// This function recursively traverses and reports the external references in a
/// schema. References to official schemas are reported but not traversed into,
/// as official schemas can only reference other official schemas. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <sourcemeta/blaze/dependencies.h>
///
/// // A custom resolver that knows about an additional schema
/// static auto test_resolver(std::string_view identifier)
///     -> sourcemeta::core::SchemaResolverResult {
///   if (identifier == "https://www.example.com/test") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "$id": "https://www.example.com/test",
///       "$schema": "https://json-schema.org/draft/2020-12/schema",
///       "type": "string"
///     })JSON");
///   } else {
///     return sourcemeta::core::schema_resolver(identifier);
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
///   sourcemeta::core::schema_walker, test_resolver,
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
/// that one limit, throwing sourcemeta::core::SchemaFrameLimitError once it
/// runs out. See sourcemeta::core::SchemaFrame for what the unit counts and
/// what it leaves to the caller
SOURCEMETA_BLAZE_DEPENDENCIES_EXPORT
auto dependencies(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaWalker &walker,
    const sourcemeta::core::SchemaResolver &resolver,
    const DependencyCallback &callback, std::string_view default_dialect = "",
    std::string_view default_id = "",
    const sourcemeta::core::SchemaFrame::Paths &paths =
        {sourcemeta::core::EMPTY_WEAK_POINTER},
    std::uint64_t max_locations = std::numeric_limits<std::uint64_t>::max())
    -> void;

} // namespace sourcemeta::blaze

#endif
