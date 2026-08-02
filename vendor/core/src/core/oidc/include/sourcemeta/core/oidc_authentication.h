#ifndef SOURCEMETA_CORE_OIDC_AUTHENTICATION_H_
#define SOURCEMETA_CORE_OIDC_AUTHENTICATION_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/oidc_profile.h>

#include <array>       // std::array
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Generate a fresh nonce for an authentication request, an unguessable value
/// bound to the session and returned in the ID Token (OpenID Connect Core 1.0
/// Section 15.5.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto nonce{sourcemeta::core::oidc_nonce()};
/// assert(nonce.size() == 43);
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_nonce() -> std::array<char, 43>;

/// @ingroup oidc
/// An OpenID Connect authentication request, the OAuth authorization request
/// extended with the OpenID Connect parameters (OpenID Connect Core 1.0
/// Section 3.1.2.1). Each field is a non-owning view.
struct OIDCAuthenticationRequest {
  /// The client identifier (OpenID Connect Core 1.0 Section 3.1.2.1), REQUIRED.
  std::string_view client_id;
  /// The redirection URI (OpenID Connect Core 1.0 Section 3.1.2.1), REQUIRED.
  std::string_view redirect_uri;
  /// The space-delimited scope, which must contain `openid` (OpenID Connect
  /// Core 1.0 Section 3.1.2.1), REQUIRED.
  std::string_view scope;
  /// The response type, defaulting to `code` (OpenID Connect Core 1.0
  /// Section 3.1.2.1).
  std::string_view response_type;
  /// The opaque CSRF state value (OpenID Connect Core 1.0 Section 3.1.2.1).
  std::string_view state;
  /// The PKCE code challenge (RFC 7636 Section 4.3).
  std::string_view code_challenge;
  /// The PKCE code challenge method (RFC 7636 Section 4.3).
  std::string_view code_challenge_method;
  /// The string bound to the session and returned in the ID Token (OpenID
  /// Connect Core 1.0 Section 3.1.2.1).
  std::string_view nonce;
  /// How the provider displays the authentication interface (OpenID Connect
  /// Core 1.0 Section 3.1.2.1).
  std::string_view display;
  /// The space-delimited prompt values, where `none` must appear alone (OpenID
  /// Connect Core 1.0 Section 3.1.2.1).
  std::string_view prompt;
  /// The maximum authentication age in seconds (OpenID Connect Core 1.0
  /// Section 3.1.2.1).
  std::string_view max_age;
  /// The space-delimited preferred UI locales (OpenID Connect Core 1.0
  /// Section 3.1.2.1).
  std::string_view ui_locales;
  /// The ID Token previously issued, hinting the session (OpenID Connect Core
  /// 1.0 Section 3.1.2.1).
  std::string_view id_token_hint;
  /// A hint about the login identifier (OpenID Connect Core 1.0
  /// Section 3.1.2.1).
  std::string_view login_hint;
  /// The space-delimited requested authentication context class references
  /// (OpenID Connect Core 1.0 Section 3.1.2.1).
  std::string_view acr_values;
  /// The serialized `claims` request parameter (OpenID Connect Core 1.0
  /// Section 5.5).
  std::string_view claims;
  /// The request object carried by value as a signed JWT (OpenID Connect Core
  /// 1.0 Section 6.1).
  std::string_view request;
  /// The request object carried by reference (OpenID Connect Core 1.0
  /// Section 6.2).
  std::string_view request_uri;
  /// The response mode override (OAuth 2.0 Multiple Response Type Encoding
  /// Practices).
  std::string_view response_mode;
};

/// @ingroup oidc
/// Build an OpenID Connect authentication request URL from an endpoint and a
/// request, returning whether the request is well formed (OpenID Connect Core
/// 1.0 Section 3.1.2.1). The `client_id` and `redirect_uri` are REQUIRED, the
/// `scope` must contain `openid`, when `prompt` carries `none` it must be the
/// only value, and `offline_access` cannot pair with a `none` prompt. The
/// `response_type` is limited by the profile, which permits the Authorization
/// Code flow by default and additionally the Hybrid `code id_token` flow under
/// `OIDCProfile::Legacy`, any flow that returns an ID Token requires a `nonce`,
/// and `OIDCProfile::Strict` requires a `code_challenge` with the `S256` method
/// (PKCE). The OpenID Connect parameters are appended to the OAuth
/// authorization query, percent-escaped, and the sink is appended to and never
/// cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
/// #include <string>
///
/// sourcemeta::core::OIDCAuthenticationRequest request;
/// request.client_id = "s6BhdRkqt3";
/// request.redirect_uri = "https://client.example/cb";
/// request.scope = "openid profile";
/// request.nonce = "n-0S6_WzA2Mj";
/// request.code_challenge = "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM";
/// request.code_challenge_method = "S256";
/// std::string url;
/// assert(sourcemeta::core::oidc_build_authentication_url(
///     "https://server.example/authorize", request, url));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_build_authentication_url(
    const std::string_view endpoint, const OIDCAuthenticationRequest &request,
    std::string &sink, const OIDCProfile profile = OIDCProfile::Strict) -> bool;

/// @ingroup oidc
/// A convenience for the common authorization code flow authentication request,
/// enforcing `scope=openid`, `response_type=code`, and the PKCE S256 method,
/// returning the URL or no value when the request is malformed (OpenID Connect
/// Core 1.0 Section 3.1.2.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto url{sourcemeta::core::oidc_authorization_url(
///     "https://server.example/authorize", "s6BhdRkqt3",
///     "https://client.example/cb", "xyz",
///     "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM", "n-0S6_WzA2Mj")};
/// assert(url.has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_authorization_url(const std::string_view authorization_endpoint,
                            const std::string_view client_id,
                            const std::string_view redirect_uri,
                            const std::string_view state,
                            const std::string_view code_challenge,
                            const std::string_view nonce)
    -> std::optional<std::string>;

/// @ingroup oidc
/// Parse the query of an OpenID Connect authentication request at the provider
/// into the result, returning whether it is well formed (OpenID Connect Core
/// 1.0 Section 3.1.2.1). The `client_id`, `redirect_uri`, and `response_type`
/// are REQUIRED, the scope must contain `openid`, a `none` prompt must appear
/// alone, an `offline_access` scope that cannot yield a refresh token, such as
/// one paired with a `none` prompt, is dropped rather than rejected (OpenID
/// Connect Core 1.0 Section 11), the `response_type` is limited by the profile,
/// and `OIDCProfile::Strict` requires a `code_challenge` with the `S256` method
/// (PKCE). Each recognized value is form-decoded, borrowing from
/// the input when it carries no escape and otherwise from the storage arena,
/// which the caller owns and reuses across parses. The result is reset first,
/// then borrows from the input and the storage, so both must outlive it. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
/// #include <string>
///
/// std::string storage;
/// sourcemeta::core::OIDCAuthenticationRequest request;
/// assert(sourcemeta::core::oidc_parse_authentication_request(
///     "response_type=code&client_id=s6BhdRkqt3&"
///     "redirect_uri=https%3A%2F%2Fclient.example%2Fcb&scope=openid&"
///     "nonce=n-0S6&code_challenge=E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM&"
///     "code_challenge_method=S256",
///     storage, request));
/// assert(request.nonce == "n-0S6");
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_parse_authentication_request(
    const std::string_view query, std::string &storage,
    OIDCAuthenticationRequest &result,
    const OIDCProfile profile = OIDCProfile::Strict) -> bool;

} // namespace sourcemeta::core

#endif
