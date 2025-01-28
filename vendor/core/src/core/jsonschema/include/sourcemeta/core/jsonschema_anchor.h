#ifndef SOURCEMETA_CORE_JSONSCHEMA_ANCHOR_H_
#define SOURCEMETA_CORE_JSONSCHEMA_ANCHOR_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema_resolver.h>

#include <cstdint>  // std::uint8_t
#include <map>      // std::map
#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::core {

/// @ingroup jsonschema
/// The anchor type
enum class AnchorType : std::uint8_t { Static, Dynamic, All };

/// @ingroup jsonschema
///
/// This function returns the anchors of the given schema, if any. This function
/// also supports the old pre 2019-09 `id` and `$id` anchor form. A given
/// subschema might have more than two anchors if, for example, declares both
/// `$anchor` and `$dynamicAnchor` (in 2020-12). For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse(R"JSON({
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "$id": "https://sourcemeta.com/example-schema",
///   "$anchor": "foo"
/// })JSON");
///
/// const auto anchors{sourcemeta::core::anchors(
///   document, sourcemeta::core::official_resolver)};
/// assert(anchors.size() == 1);
/// assert(anchors.contains("foo"));
/// // This is a static anchor
/// assert(anchors.at("foo") == sourcemeta::core::AnchorType::Static);
/// ```
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto anchors(const JSON &schema, const SchemaResolver &resolver,
             const std::optional<std::string> &default_dialect = std::nullopt)
    -> std::map<std::string, AnchorType>;

/// @ingroup jsonschema
///
/// This function is a shortcut to sourcemeta::core::anchors if you have
/// already computed the vocabularies that the given schema makes use of.
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto anchors(const JSON &schema,
             const std::map<std::string, bool> &vocabularies)
    -> std::map<std::string, AnchorType>;

} // namespace sourcemeta::core

#endif
