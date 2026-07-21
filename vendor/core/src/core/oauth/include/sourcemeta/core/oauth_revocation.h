#ifndef SOURCEMETA_CORE_OAUTH_REVOCATION_H_
#define SOURCEMETA_CORE_OAUTH_REVOCATION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/http.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The `access_token` token type hint (RFC 7009 Section 2.1), reused by token
/// introspection (RFC 7662 Section 2.1).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_HINT_ACCESS_TOKEN{
    "access_token"};
/// @ingroup oauth
/// The `refresh_token` token type hint (RFC 7009 Section 2.1), reused by token
/// introspection (RFC 7662 Section 2.1).
inline constexpr std::string_view OAUTH_TOKEN_TYPE_HINT_REFRESH_TOKEN{
    "refresh_token"};

/// @ingroup oauth
/// A non-owning view of a token revocation or introspection request (RFC 7009
/// Section 2.1, RFC 7662 Section 2.1). The token borrows from the input or the
/// decode arena, and an absent hint is an empty view.
struct OAuthTokenLookupRequest {
  /// The token to act on (RFC 7009 Section 2.1).
  std::string_view token;
  /// The hint about the token type, one of the hint vocabulary values, empty
  /// when absent (RFC 7009 Section 2.1).
  std::string_view token_type_hint;
};

/// @ingroup oauth
/// Append a token revocation request body (RFC 7009 Section 2.1) to the sink.
/// The `token` is required, and the `token_type_hint` is emitted only when
/// present. No `client_id` is emitted, so the caller composes a client
/// authentication builder into the same sink. The token is secret, so the sink
/// is a wiping string, and it is appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_revocation_request(
///     "45ghiukldjahdnhzdauz", "", body);
/// assert(body == "token=45ghiukldjahdnhzdauz");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_revocation_request(const std::string_view token,
                                    const std::string_view token_type_hint,
                                    SecureString &sink) -> void;

/// @ingroup oauth
/// Parse a token revocation request body (RFC 7009 Section 2.1) into the
/// result, returning whether it is well formed. The token is required, a
/// duplicated parameter fails (RFC 6749 Section 3.2), and every other
/// parameter, such as the client authentication ones, is passed to the
/// callback. The token is secret, so it is decoded into a wiping arena the
/// caller owns and reuses, and which must not alias the body. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString storage;
/// sourcemeta::core::OAuthTokenLookupRequest request;
/// assert(sourcemeta::core::oauth_parse_revocation_request(
///     "token=45ghiukldjahdnhzdauz&token_type_hint=refresh_token", storage,
///     request, [](std::string_view, std::string_view) {}));
/// assert(request.token == "45ghiukldjahdnhzdauz");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_parse_revocation_request(
    const std::string_view body, SecureString &storage,
    OAuthTokenLookupRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool;

/// @ingroup oauth
/// The outcome a client draws from a token revocation response (RFC 7009
/// Section 2.2).
enum class OAuthRevocationOutcome : std::uint8_t {
  /// The revocation succeeded, which an unknown token also produces (RFC 7009
  /// Section 2.2).
  Success,
  /// The server is temporarily unable to respond and the request may be
  /// retried, honoring any `Retry-After` (RFC 7009 Section 2.2.1).
  Retry,
  /// The request failed, and the body carries a token endpoint error code
  /// (RFC 7009 Section 2.2.1).
  Error
};

/// @ingroup oauth
/// Map a revocation response status code to its outcome: a 200 is a success,
/// including for an unknown token (RFC 7009 Section 2.2), a 503 is retryable
/// (RFC 7009 Section 2.2.1), and any other status is an error. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_revocation_outcome(HTTP_STATUS_OK) ==
///        sourcemeta::core::OAuthRevocationOutcome::Success);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_revocation_outcome(const HTTPStatus status) noexcept
    -> OAuthRevocationOutcome;

} // namespace sourcemeta::core

#endif
