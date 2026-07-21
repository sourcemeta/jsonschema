#ifndef SOURCEMETA_CORE_OAUTH_RANDOM_H_
#define SOURCEMETA_CORE_OAUTH_RANDOM_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <array> // std::array

namespace sourcemeta::core {

/// @ingroup oauth
/// Mint a random token, the base64url encoding of 32 cryptographically random
/// octets, for an unguessable but non-confidential value such as a `state` or
/// `nonce` (RFC 6749 Section 10.10). The 43 bytes are not null terminated, so
/// pass them onward as a view of `data()` and `size()` rather than as a C
/// string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string_view>
///
/// const auto token{sourcemeta::core::oauth_random_token()};
/// const std::string_view view{token.data(), token.size()};
/// assert(view.size() == 43);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_random_token() -> std::array<char, 43>;

} // namespace sourcemeta::core

#endif
