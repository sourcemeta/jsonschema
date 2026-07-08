#ifndef SOURCEMETA_CORE_JOSE_VERIFY_H_
#define SOURCEMETA_CORE_JOSE_VERIFY_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwk.h>
#include <sourcemeta/core/jose_jwks.h>
#include <sourcemeta/core/jose_jwt.h>
// NOLINTEND(misc-include-cleaner)

#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <span>        // std::span
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// The claim validation errors that claim checking can return, one per check
/// performed rather than an exhaustive list of registered claims.
enum class JWTClaimError : std::uint8_t {
  /// The issuer claim is missing or does not match the expected value.
  Issuer,
  /// The subject claim is missing or does not match the expected value.
  Subject,
  /// The audience claim is missing or does not contain the expected value.
  Audience,
  /// The expiration time claim is missing or the token has expired.
  Expiration,
  /// The not-before time claim is malformed or lies in the future.
  NotBefore,
  /// The issued-at time claim is malformed or lies in the future.
  IssuedAt
};

/// @ingroup jose
/// Validate the registered claims of a JSON Web Token against the expected
/// issuer and audience at a given time, returning the first failing check or no
/// value when every check passes. The expiration claim is required (RFC 9068
/// Section 2.2), and the subject is checked only when an expected value is
/// supplied. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
/// #include <chrono>
/// #include <string>
///
/// const std::string input{
///     "eyJhbGciOiJSUzI1NiJ9."
///     "eyJpc3MiOiJhY21lIiwiYXVkIjoiY2xpZW50IiwiZXhwIjoyMDAwMDAwMDAwfQ.c2ln"};
/// const auto token{sourcemeta::core::JWT::from(input)};
/// assert(token.has_value());
/// const auto error{sourcemeta::core::jwt_check_claims(
///     token.value(), "acme", "client",
///     std::chrono::system_clock::from_time_t(1500000000))};
/// assert(!error.has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwt_check_claims(
    const JWT &token, const std::string_view expected_issuer,
    const std::string_view expected_audience,
    const std::chrono::system_clock::time_point now,
    const std::chrono::seconds clock_skew = std::chrono::seconds{0},
    const std::optional<std::string_view> expected_subject = std::nullopt)
    -> std::optional<JWTClaimError>;

/// @ingroup jose
/// Verify a JSON Web Signature given its algorithm, its signing input, and its
/// decoded signature against a JSON Web Key, returning false rather than
/// throwing for an unrecognized algorithm, a key whose type or curve cannot
/// serve the algorithm, a key declaring a contradicting algorithm, or a
/// signature that does not verify. The signing input is the exact bytes the
/// signature was computed over, which carry no constraint on their content. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::JWK::from(sourcemeta::core::parse_json(
///     R"JSON({ "kty": "RSA", "n": "", "e": "" })JSON"))};
/// assert(!key.has_value() ||
///        !sourcemeta::core::jws_verify_signature(
///            sourcemeta::core::JWSAlgorithm::RS256, "header.payload",
///            "signature", key.value()));
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jws_verify_signature(const std::optional<JWSAlgorithm> algorithm,
                          const std::string_view signing_input,
                          const std::string_view signature, const JWK &key)
    -> bool;

/// @ingroup jose
/// Verify the signature of a JSON Web Token against a JSON Web Key, returning
/// false rather than throwing whenever the token does not carry a confirmed
/// valid signature for the key. This includes an unrecognized algorithm, a key
/// whose type or curve cannot serve the algorithm, a key declaring a
/// contradicting algorithm, and a signature that does not verify. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <cassert>
/// #include <string>
///
/// const std::string input{
///     "eyJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY21lIn0.c2ln"};
/// const auto token{sourcemeta::core::JWT::from(input)};
/// assert(token.has_value());
/// const auto key{sourcemeta::core::JWK::from(
///     sourcemeta::core::parse_json(R"JSON({
///       "kty": "RSA", "n": "", "e": ""
///     })JSON"))};
/// assert(!key.has_value() ||
///        !sourcemeta::core::jwt_verify_signature(token.value(), key.value()));
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwt_verify_signature(const JWT &token, const JWK &key) -> bool;

/// @ingroup jose
/// The steps of full token verification that can fail, in the order they are
/// evaluated.
enum class JWTVerificationError : std::uint8_t {
  /// The token's algorithm is missing or absent from the allow-list.
  AlgorithmNotAllowed,
  /// No key in the set could be selected or verified the signature.
  UnknownKey,
  /// The named key was found but its signature did not verify.
  Signature,
  /// The token type does not match the expected media type.
  Type,
  /// The issuer claim is missing or does not match the expected value.
  Issuer,
  /// The subject claim is missing or does not match the expected value.
  Subject,
  /// The audience claim is missing or does not contain the expected value.
  Audience,
  /// The expiration time claim is missing or the token has expired.
  Expiration,
  /// The not-before time claim is malformed or lies in the future.
  NotBefore,
  /// The issued-at time claim is malformed or lies in the future.
  IssuedAt
};

/// @ingroup jose
/// Verify a JSON Web Token end to end against a key set, in the mandated order:
/// the algorithm must be in the allow-list, a key is selected by its identifier
/// or, when absent, tried against every compatible key, the signature must
/// verify, and the claims must pass. Returns no value when the token is fully
/// valid, or the first failing step. The type check enforces the access token
/// profile (RFC 9068 Section 2.1) only when an expected type is supplied. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
/// #include <array>
/// #include <cassert>
/// #include <chrono>
/// #include <string>
///
/// const std::string input{
///     "eyJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY21lIn0.c2ln"};
/// const auto token{sourcemeta::core::JWT::from(input)};
/// assert(token.has_value());
/// const auto keys{sourcemeta::core::JWKS::from(
///     sourcemeta::core::parse_json(R"JSON({ "keys": [] })JSON"))};
/// assert(keys.has_value());
/// const std::array allowed{sourcemeta::core::JWSAlgorithm::RS256};
/// const auto error{sourcemeta::core::jwt_verify(
///     token.value(), keys.value(), allowed, "acme", "client",
///     std::chrono::system_clock::from_time_t(1500000000))};
/// assert(error.has_value());
/// ```
SOURCEMETA_CORE_JOSE_EXPORT
auto jwt_verify(
    const JWT &token, const JWKS &keys,
    const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view expected_issuer,
    const std::string_view expected_audience,
    const std::chrono::system_clock::time_point now,
    const std::chrono::seconds clock_skew = std::chrono::seconds{0},
    const std::optional<std::string_view> expected_subject = std::nullopt,
    const std::optional<std::string_view> expected_type = std::nullopt)
    -> std::optional<JWTVerificationError>;

} // namespace sourcemeta::core

#endif
