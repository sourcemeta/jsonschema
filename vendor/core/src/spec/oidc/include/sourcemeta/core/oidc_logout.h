#ifndef SOURCEMETA_CORE_OIDC_LOGOUT_H_
#define SOURCEMETA_CORE_OIDC_LOGOUT_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/jose.h>

#include <chrono>      // std::chrono::system_clock
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// An RP-Initiated Logout request (OpenID Connect RP-Initiated Logout 1.0
/// Section 2). Each field is a non-owning view.
struct OIDCLogoutRequest {
  /// The ID Token previously issued, hinting which session to end (OpenID
  /// Connect RP-Initiated Logout 1.0 Section 2).
  std::string_view id_token_hint;
  /// A hint about the end user, when no ID Token hint is available (OpenID
  /// Connect RP-Initiated Logout 1.0 Section 2).
  std::string_view logout_hint;
  /// The client identifier (OpenID Connect RP-Initiated Logout 1.0 Section 2).
  std::string_view client_id;
  /// Where to redirect after logout, matched against the registered set (OpenID
  /// Connect RP-Initiated Logout 1.0 Section 2).
  std::string_view post_logout_redirect_uri;
  /// The opaque state echoed back to the client (OpenID Connect RP-Initiated
  /// Logout 1.0 Section 2).
  std::string_view state;
  /// The space-delimited preferred UI locales (OpenID Connect RP-Initiated
  /// Logout 1.0 Section 2).
  std::string_view ui_locales;
};

/// @ingroup oidc
/// Build an RP-Initiated Logout request URL from an end session endpoint and a
/// request (OpenID Connect RP-Initiated Logout 1.0 Section 2). Each present
/// parameter is percent-escaped, an existing query on the endpoint is honored,
/// and the sink is appended to and never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OIDCLogoutRequest request;
/// request.id_token_hint = "eyJ...";
/// request.post_logout_redirect_uri = "https://client.example/after";
/// std::string url;
/// sourcemeta::core::oidc_build_logout_url(
///     "https://server.example/logout", request, url);
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_build_logout_url(const std::string_view end_session_endpoint,
                           const OIDCLogoutRequest &request, std::string &sink)
    -> void;

/// @ingroup oidc
/// Validate a Back-Channel Logout token against a key set at a given time
/// (OpenID Connect Back-Channel Logout 1.0 Section 2.6). The signature is
/// verified under a pinned algorithm, the issuer and audience must match, the
/// `iat` must be present and not in the future, the `exp` must be present and
/// not have passed, the `typ` header when present must be `logout+jwt`, the
/// token must carry a `sub` or `sid`, an `events` object with the back-channel
/// logout member, a `jti`, and it must not carry a `nonce`. Replay rejection by
/// `jti` is the caller's responsibility. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
/// #include <chrono>
///
/// const auto token{sourcemeta::core::JWT::from(compact)};
/// const auto keys{sourcemeta::core::JWKS::from(key_set)};
/// const std::array allowed{sourcemeta::core::JWSAlgorithm::RS256};
/// assert(token.has_value() && keys.has_value());
/// const auto valid{sourcemeta::core::oidc_validate_logout_token(
///     token.value(), keys.value(), allowed, "https://issuer.example",
///     "client-id", std::chrono::system_clock::now())};
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_validate_logout_token(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view issuer, const std::string_view client_id,
    const std::chrono::system_clock::time_point now,
    const JWTClockSkew clock_skew = {}) -> bool;

/// @ingroup oidc
/// Whether a Front-Channel Logout `iss` and `sid` pair is used in a valid
/// combination, which is both present or both absent (OpenID Connect
/// Front-Channel Logout 1.0 Section 3). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_front_channel_pairing_is_valid(
///     "https://issuer.example", "session-1"));
/// assert(!sourcemeta::core::oidc_front_channel_pairing_is_valid(
///     "https://issuer.example", ""));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_front_channel_pairing_is_valid(const std::string_view issuer,
                                         const std::string_view session_id)
    -> bool;

/// @ingroup oidc
/// Compute a Session Management `session_state` value from the client, the
/// client origin, the OpenID Provider browser state, and a salt (OpenID Connect
/// Session Management 1.0 Section 4.2). The value is the base64url of the
/// SHA-256 of the space-joined inputs, a dot, and the salt. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto state{sourcemeta::core::oidc_session_state(
///     "client-id", "https://client.example", "browser-state", "salt")};
/// assert(!state.empty());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_session_state(const std::string_view client_id,
                        const std::string_view origin,
                        const std::string_view provider_browser_state,
                        const std::string_view salt) -> std::string;

} // namespace sourcemeta::core

#endif
