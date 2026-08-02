#ifndef SOURCEMETA_CORE_OIDC_REQUEST_OBJECT_H_
#define SOURCEMETA_CORE_OIDC_REQUEST_OBJECT_H_

#ifndef SOURCEMETA_CORE_OIDC_EXPORT
#include <sourcemeta/core/oidc_export.h>
#endif

#include <sourcemeta/core/jose.h>
#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oidc
/// Whether the `request` and `request_uri` parameters are used in a valid
/// combination, which is at most one of them (OpenID Connect Core 1.0
/// Section 6). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oidc_request_object_pairing_is_valid(
///     "eyJ...", ""));
/// assert(!sourcemeta::core::oidc_request_object_pairing_is_valid(
///     "eyJ...", "https://client.example/request.jwt"));
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_request_object_pairing_is_valid(const std::string_view request,
                                          const std::string_view request_uri)
    -> bool;

/// @ingroup oidc
/// Build and sign a request object from a set of authentication request
/// parameters, returning the compact JWT or no value when the key cannot
/// produce the signature (OpenID Connect Core 1.0 Section 6.1). The parameters
/// should carry the `iss` set to the client and the `aud` set to the OpenID
/// Provider issuer. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <cassert>
///
/// const auto parameters{sourcemeta::core::parse_json(
///     R"JSON({"iss":"client","aud":"https://op.example","scope":"openid"})JSON")};
/// const auto object{sourcemeta::core::oidc_build_request_object(
///     parameters, key, sourcemeta::core::JWSAlgorithm::RS256)};
/// assert(object.has_value());
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_build_request_object(const JSON &parameters, const JWKPrivate &key,
                               const JWSAlgorithm algorithm)
    -> std::optional<std::string>;

/// @ingroup oidc
/// Verify a request object and return its parameters, or no value when the
/// signature does not verify under a pinned algorithm, the issuer is present
/// but is not the client, or the audience is missing or does not include the
/// OpenID Provider (OpenID Connect Core 1.0 Section 6.1 and Section 6.3). A
/// signed request object must carry an `aud` that includes the provider,
/// binding it to this provider so it cannot be replayed to another one. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oidc.h>
/// #include <array>
/// #include <cassert>
///
/// const auto token{sourcemeta::core::JWT::from(compact)};
/// const auto keys{sourcemeta::core::JWKS::from(client_keys)};
/// const std::array allowed{sourcemeta::core::JWSAlgorithm::RS256};
/// assert(token.has_value() && keys.has_value());
/// const auto parameters{sourcemeta::core::oidc_verify_request_object(
///     token.value(), keys.value(), allowed, "client", "https://op.example")};
/// ```
SOURCEMETA_CORE_OIDC_EXPORT
auto oidc_verify_request_object(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view client_id, const std::string_view provider_issuer)
    -> std::optional<JSON>;

} // namespace sourcemeta::core

#endif
