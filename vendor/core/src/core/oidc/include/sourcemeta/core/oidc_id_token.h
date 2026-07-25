#ifndef SOURCEMETA_CORE_OIDC_ID_TOKEN_H_
#define SOURCEMETA_CORE_OIDC_ID_TOKEN_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono::system_clock, std::chrono::seconds
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Extract the ID Token from a token endpoint response document, returning no
/// value when the `id_token` member is absent or not a string (OpenID Connect
/// Core 1.0 Section 3.1.3.3). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto response{sourcemeta::core::parse_json(
///     R"JSON({"access_token":"at","id_token":"eyJ..."})JSON")};
/// const auto id_token{sourcemeta::core::oidc_parse_id_token(response)};
/// assert(id_token.has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_parse_id_token(const JSON &token_response)
    -> std::optional<std::string>;

/// @ingroup oidc
/// The identity a validated ID Token asserts, owning its strings so it outlives
/// the token it was extracted from (OpenID Connect Core 1.0 Section 2).
struct OIDCIdentity {
  /// The subject identifier, locally unique and never reassigned at the issuer
  /// (OpenID Connect Core 1.0 Section 2).
  std::string subject;
  /// The issuer that produced the token (OpenID Connect Core 1.0 Section 2).
  std::string issuer;
  /// The authentication context class reference, when present (OpenID Connect
  /// Core 1.0 Section 2).
  std::optional<std::string> authentication_context_class;
  /// The time the end user authenticated, when present (OpenID Connect Core 1.0
  /// Section 2).
  std::optional<std::chrono::system_clock::time_point> authentication_time;
};

/// @ingroup oidc
/// The OpenID Connect specific checks a Relying Party layers on top of the base
/// JSON Web Token verification when validating an ID Token (OpenID Connect Core
/// 1.0 Section 3.1.3.7).
struct OIDCValidationOptions {
  /// The nonce sent in the authentication request, which the token must echo
  /// when supplied (OpenID Connect Core 1.0 Section 3.1.3.7 step 11).
  std::optional<std::string_view> nonce;
  /// The acceptable authentication context class references, checked when
  /// non-empty (OpenID Connect Core 1.0 Section 3.1.3.7 step 12).
  std::span<const std::string_view> acceptable_authentication_context_classes;
  /// The maximum authentication age, requiring a fresh `auth_time` when set
  /// (OpenID Connect Core 1.0 Section 3.1.3.7 step 13).
  std::optional<std::chrono::seconds> maximum_authentication_age;
  /// The maximum age of the `iat` claim, checked when set (OpenID Connect Core
  /// 1.0 Section 3.1.3.7 step 10).
  std::optional<std::chrono::seconds> maximum_issued_at_age;
  /// The access token to bind through `at_hash`, verified against the claim
  /// when both are present (OpenID Connect Core 1.0 Section 3.1.3.6).
  std::optional<std::string_view> access_token;
  /// The authorization code to bind through `c_hash`, verified against the
  /// claim when both are present (OpenID Connect Core 1.0 Section 3.3.2.11).
  std::optional<std::string_view> code;
  /// Whether the `at_hash` claim is required, the case for the implicit and
  /// hybrid flows that return an access token from the authorization endpoint.
  /// When set, the claim must be present and verified against `access_token`,
  /// which must also be supplied (OpenID Connect Core 1.0 Sections 3.2.2.10 and
  /// 3.3.2.11).
  bool require_access_token_hash{false};
  /// Whether the `c_hash` claim is required, the case for the hybrid
  /// `code id_token` flow. When set, the claim must be present and verified
  /// against `code`, which must also be supplied (OpenID Connect Core 1.0
  /// Section 3.3.2.11).
  bool require_code_hash{false};
};

/// @ingroup oidc
/// Validate an ID Token against a key set at a given time, returning the
/// asserted identity or no value when any check fails (OpenID Connect Core 1.0
/// Section 3.1.3.7). The base JSON Web Token verification (signature under a
/// pinned algorithm, issuer, audience, expiration, and skew) runs first, then
/// the OpenID Connect steps: the subject and issued-at are required, an `azp`
/// matching the client is required when the audience carries more than one
/// value, the nonce is echoed when one was sent, the authentication context and
/// age constraints hold, and a required binding hash is present and matches.
/// The algorithm allow-list is pinned by the caller and must never contain
/// `none`. For example:
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
/// const auto identity{sourcemeta::core::oidc_validate_id_token(
///     token.value(), keys.value(), allowed, "https://issuer.example",
///     "client-id", std::chrono::system_clock::now())};
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_validate_id_token(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view issuer, const std::string_view client_id,
    const std::chrono::system_clock::time_point now,
    const OIDCValidationOptions &options = {},
    const JWTClockSkew clock_skew = {}) -> std::optional<OIDCIdentity>;

/// @ingroup oidc
/// Validate an ID Token against a caching key set provider, returning the
/// asserted identity or no value when any check fails (OpenID Connect Core 1.0
/// Section 3.1.3.7). The provider supplies the keys, the current time, and the
/// clock skew, and the OpenID Connect steps run against the same clock the
/// signature verification used. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
///
/// const auto token{sourcemeta::core::JWT::from(compact)};
/// const std::array allowed{sourcemeta::core::JWSAlgorithm::RS256};
/// assert(token.has_value());
/// const auto identity{sourcemeta::core::oidc_validate_id_token(
///     provider, token.value(), allowed, "https://issuer.example",
///     "client-id")};
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_validate_id_token(
    JWKSProvider &provider, const JWT &token,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view issuer, const std::string_view client_id,
    const OIDCValidationOptions &options = {}) -> std::optional<OIDCIdentity>;

/// @ingroup oidc
/// The claims an OpenID Provider mints into an ID Token (OpenID Connect
/// Core 1.0 Section 2). The access token and code, when supplied, are hashed
/// into the `at_hash` and `c_hash` claims under the signing algorithm.
struct OIDCIdTokenClaims {
  /// The issuer identifier (OpenID Connect Core 1.0 Section 2), REQUIRED.
  std::string_view issuer;
  /// The subject identifier (OpenID Connect Core 1.0 Section 2), REQUIRED.
  std::string_view subject;
  /// The audience, the client the token is for (OpenID Connect Core 1.0
  /// Section 2), REQUIRED.
  std::string_view audience;
  /// The time the token was issued (OpenID Connect Core 1.0 Section 2),
  /// REQUIRED.
  std::chrono::system_clock::time_point issued_at;
  /// The time the token expires (OpenID Connect Core 1.0 Section 2), REQUIRED.
  std::chrono::system_clock::time_point expiration;
  /// The nonce echoed from the authentication request (OpenID Connect Core 1.0
  /// Section 2).
  std::optional<std::string_view> nonce;
  /// The authorized party, set when the audience carries more than one value
  /// (OpenID Connect Core 1.0 Section 2).
  std::optional<std::string_view> authorized_party;
  /// The authentication context class reference (OpenID Connect Core 1.0
  /// Section 2).
  std::optional<std::string_view> authentication_context_class;
  /// The time the end user authenticated (OpenID Connect Core 1.0 Section 2).
  std::optional<std::chrono::system_clock::time_point> authentication_time;
  /// The access token to bind through `at_hash` (OpenID Connect Core 1.0
  /// Section 3.1.3.6).
  std::optional<std::string_view> access_token;
  /// The authorization code to bind through `c_hash` (OpenID Connect Core 1.0
  /// Section 3.3.2.11).
  std::optional<std::string_view> code;
};

/// @ingroup oidc
/// Mint and sign an ID Token from a claim set under a signing key and
/// algorithm, returning the compact serialization or no value when the key
/// cannot produce the signature (OpenID Connect Core 1.0 Section 2). The
/// `at_hash` and `c_hash` claims are computed and embedded when the access
/// token or code is supplied. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
/// #include <chrono>
///
/// sourcemeta::core::OIDCIdTokenClaims claims;
/// claims.issuer = "https://issuer.example";
/// claims.subject = "user-1";
/// claims.audience = "client-id";
/// claims.issued_at = std::chrono::system_clock::now();
/// claims.expiration = claims.issued_at + std::chrono::hours{1};
/// const auto token{sourcemeta::core::oidc_mint_id_token(
///     claims, key, sourcemeta::core::JWSAlgorithm::RS256)};
/// assert(token.has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_mint_id_token(const OIDCIdTokenClaims &claims, const JWKPrivate &key,
                        const JWSAlgorithm algorithm)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
