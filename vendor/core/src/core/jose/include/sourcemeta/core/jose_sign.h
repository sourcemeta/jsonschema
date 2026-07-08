#ifndef SOURCEMETA_CORE_JOSE_SIGN_H_
#define SOURCEMETA_CORE_JOSE_SIGN_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwk_private.h>

#include <sourcemeta/core/json.h>

#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// Produce the raw signature bytes for a signing input under an algorithm, the
/// signing counterpart to signature verification. Returns no value for a key
/// whose type or curve cannot serve the algorithm, a key declaring a
/// contradicting algorithm, or a key whose material never formed a usable
/// private key. The signing input is the exact bytes to sign, which carry no
/// constraint on their content. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::JWKPrivate::from_pem(pem)};
/// assert(key.has_value());
/// const auto signature{sourcemeta::core::jws_sign(
///     sourcemeta::core::JWSAlgorithm::RS256, "header.payload", key.value())};
/// assert(signature.has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jws_sign(const JWSAlgorithm algorithm,
              const std::string_view signing_input, const JWKPrivate &key)
    -> std::optional<std::string>;

/// @ingroup jose
/// Build and sign a JSON Web Token in compact serialization (RFC 7519, RFC
/// 7515) from a header and a payload, returning the base64url header, payload,
/// and signature joined by dots. The algorithm is taken from the header
/// algorithm parameter (RFC 7515 Section 4.1.1). Returns no value when the
/// header or payload is not an object, the header names no supported algorithm,
/// or the key cannot produce the
/// signature. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <sourcemeta/core/json.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::JWKPrivate::from_pem(pem)};
/// assert(key.has_value());
/// const auto token{sourcemeta::core::jwt_sign(
///     sourcemeta::core::parse_json(R"({ "alg": "RS256" })"),
///     sourcemeta::core::parse_json(R"({ "iss": "acme" })"), key.value())};
/// assert(token.has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwt_sign(const JSON &header, const JSON &payload, const JWKPrivate &key)
    -> std::optional<std::string>;

} // namespace sourcemeta::core

#endif
