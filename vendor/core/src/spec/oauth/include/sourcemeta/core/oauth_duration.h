#ifndef SOURCEMETA_CORE_OAUTH_DURATION_H_
#define SOURCEMETA_CORE_OAUTH_DURATION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <chrono>   // std::chrono::seconds
#include <optional> // std::optional

namespace sourcemeta::core {

/// @ingroup oauth
/// Read an integer member of a JSON object as a duration in seconds, rejecting
/// a negative value as malformed, and one past the range of the duration so a
/// bad lifetime or interval cannot reach a caller narrowed. The OAuth and
/// OpenID Connect documents spell every lifetime and interval this way, so the
/// two families read them through this one function rather than each deciding
/// for itself what a malformed member looks like. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json("{ \"expires_in\": 1800
/// }")}; const auto hash{sourcemeta::core::JSON::Object::hash("expires_in")};
/// const auto value{
///     sourcemeta::core::oauth_json_seconds_member(document, "expires_in",
///     hash)};
/// assert(value.has_value());
/// assert(value.value() == std::chrono::seconds{1800});
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_json_seconds_member(const JSON &data, const JSON::StringView name,
                               const JSON::Object::hash_type hash)
    -> std::optional<std::chrono::seconds>;

} // namespace sourcemeta::core

#endif
