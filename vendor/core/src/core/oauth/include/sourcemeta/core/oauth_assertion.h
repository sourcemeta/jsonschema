#ifndef SOURCEMETA_CORE_OAUTH_ASSERTION_H_
#define SOURCEMETA_CORE_OAUTH_ASSERTION_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwk_private.h>
#include <sourcemeta/core/jose_jwks.h>
#include <sourcemeta/core/oauth_dpop.h>

#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The grant type identifier of a JWT bearer authorization grant (RFC 7523
/// Section 2.1).
inline constexpr std::string_view OAUTH_GRANT_TYPE_JWT_BEARER{
    "urn:ietf:params:oauth:grant-type:jwt-bearer"};
/// @ingroup oauth
/// The client assertion type identifier of a JWT bearer client authentication
/// assertion (RFC 7523 Section 2.2).
inline constexpr std::string_view OAUTH_CLIENT_ASSERTION_TYPE_JWT_BEARER{
    "urn:ietf:params:oauth:client-assertion-type:jwt-bearer"};

/// @ingroup oauth
/// Build a JWT bearer assertion (RFC 7523 Section 3) signed with the given key
/// and algorithm, carrying the issuer, subject, and a single audience, an
/// expiration a lifetime past the given time, an issue time, and a random
/// identifier, or no value when the key cannot sign. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/jose.h>
/// #include <chrono>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::JWKPrivate::from_pem(pem)};
/// assert(key.has_value());
/// const auto assertion{sourcemeta::core::oauth_build_assertion(
///     "issuer", "subject", "https://server.example/token",
///     std::chrono::seconds{300}, std::chrono::system_clock::now(),
///     key.value(), sourcemeta::core::JWSAlgorithm::ES256)};
/// assert(assertion.has_value());
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_build_assertion(
    const std::string_view issuer, const std::string_view subject,
    const std::string_view audience, const std::chrono::seconds lifetime,
    const std::chrono::system_clock::time_point now, const JWKPrivate &key,
    const JWSAlgorithm algorithm) -> std::optional<std::string>;

/// @ingroup oauth
/// Build a JWT bearer client authentication assertion (RFC 7523 Section 3), a
/// self-issued assertion whose issuer and subject are both the client
/// identifier (RFC 7521 Section 5.2), or no value when the key cannot sign. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/jose.h>
/// #include <chrono>
/// #include <cassert>
///
/// const auto key{sourcemeta::core::JWKPrivate::from_pem(pem)};
/// assert(key.has_value());
/// const auto assertion{sourcemeta::core::oauth_build_client_assertion(
///     "s6BhdRkqt3", "https://server.example/token", std::chrono::seconds{300},
///     std::chrono::system_clock::now(), key.value(),
///     sourcemeta::core::JWSAlgorithm::ES256)};
/// assert(assertion.has_value());
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_build_client_assertion(
    const std::string_view client_id, const std::string_view audience,
    const std::chrono::seconds lifetime,
    const std::chrono::system_clock::time_point now, const JWKPrivate &key,
    const JWSAlgorithm algorithm) -> std::optional<std::string>;

/// @ingroup oauth
/// Append the client authentication assertion parameters (RFC 7521 Section 4.2)
/// to the sink, the assertion and its JWT bearer type. No `client_id` is
/// emitted, since the client is identified by the assertion subject. The
/// assertion is a credential, so the sink is a wiping string, appended to and
/// never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_client_assertion("eyJ...", body);
/// assert(std::string_view{body}.starts_with("client_assertion_type="));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_client_assertion(const std::string_view assertion,
                            SecureString &sink) -> void;

/// @ingroup oauth
/// Append a JWT bearer authorization grant token request body (RFC 7523
/// Section 2.1) to the sink, the grant type, the assertion, and the scope when
/// present. No `client_id` is emitted, so the caller composes a client
/// authentication builder into the same sink when the client authenticates. The
/// assertion is a credential, so the sink is a wiping string, appended to and
/// never cleared. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// sourcemeta::core::SecureString body;
/// sourcemeta::core::oauth_build_token_request_jwt_bearer("eyJ...", "read",
///                                                        body);
/// assert(std::string_view{body}.starts_with("grant_type="));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_build_token_request_jwt_bearer(const std::string_view assertion,
                                          const std::string_view scope,
                                          SecureString &sink) -> void;

/// @ingroup oauth
/// The reason a JWT bearer assertion failed verification (RFC 7523 Section 3).
enum class OAuthAssertionError : std::uint8_t {
  /// The assertion was not a well-formed JSON Web Token.
  Malformed,
  /// The algorithm was absent or outside the accepted set (RFC 7523 Section 5).
  UnsupportedAlgorithm,
  /// No key verified the signature.
  UnknownKey,
  /// The signature did not verify.
  Signature,
  /// The issuer claim was absent or did not match (RFC 7523 Section 3 check 1).
  Issuer,
  /// The subject claim was absent or did not match (RFC 7523 Section 3 check 2,
  /// RFC 7521 Section 4.2).
  Subject,
  /// The audience claim did not identify this server (RFC 7523 Section 3
  /// check 3).
  Audience,
  /// The assertion has expired (RFC 7523 Section 3 check 4).
  Expired,
  /// The assertion is not yet valid (RFC 7523 Section 3 check 5).
  NotYetValid,
  /// The issue time lies in the future (RFC 7523 Section 3 check 6).
  IssuedInFuture,
  /// The identifier was already seen within its window, a replay (RFC 7523
  /// Section 3 check 7).
  Replay
};

/// @ingroup oauth
/// The inputs to JWT bearer assertion verification beyond the request. The
/// accepted algorithms are matched exactly, so an empty set accepts none, and a
/// replay store, when set, rejects a reused identifier.
struct OAuthAssertionVerifyOptions {
  /// The algorithms accepted per local policy, a non-owning view whose backing
  /// storage must outlive the verification (RFC 7523 Section 5).
  std::span<const JWSAlgorithm> allowed_algorithms{};
  /// The tolerance applied to the expiration, not-before, and issue times.
  std::chrono::seconds clock_skew{std::chrono::seconds{0}};
  /// The store tracking assertion identifiers to reject a replay, ignored when
  /// null or when the assertion carries no identifier (RFC 7523 Section 3
  /// check 7).
  OAuthDPoPReplayStore *replay_store{nullptr};
};

/// @ingroup oauth
/// Verify a JWT bearer client authentication assertion (RFC 7523 Section 3,
/// RFC 7521 Section 5.2), returning the first failing check or no value when it
/// verifies. The issuer and subject must both be the client identifier, the
/// subject must match a presented `client_id` when one is given (RFC 7521
/// Section 4.2), and the audience must match one of the accepted values by
/// Simple String Comparison without normalization (RFC 3986 Section 6.2.1). A
/// failure is reported to the client as `invalid_client` (RFC 7521
/// Section 4.2.1). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/jose.h>
/// #include <array>
/// #include <chrono>
/// #include <cassert>
///
/// const std::array<std::string_view, 1>
/// audiences{{"https://server.example/token"}}; const std::array
/// allowed{sourcemeta::core::JWSAlgorithm::ES256};
/// sourcemeta::core::OAuthAssertionVerifyOptions options;
/// options.allowed_algorithms = allowed;
/// const auto error{sourcemeta::core::oauth_verify_client_assertion(
///     assertion, audiences, "s6BhdRkqt3", keys,
///     std::chrono::system_clock::now(), options)};
/// assert(!error.has_value() ||
///        error.value() == sourcemeta::core::OAuthAssertionError::Expired);
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_verify_client_assertion(
    const std::string_view assertion,
    const std::span<const std::string_view> expected_audiences,
    const std::string_view request_client_id, const JWKS &keys,
    const std::chrono::system_clock::time_point now,
    const OAuthAssertionVerifyOptions &options)
    -> std::optional<OAuthAssertionError>;

/// @ingroup oauth
/// Verify a JWT bearer authorization grant assertion (RFC 7523 Section 3),
/// returning the first failing check or no value when it verifies. The issuer
/// must match the expected one, the subject identifies the accessor and is only
/// required to be present, and the audience must match one of the accepted
/// values by Simple String Comparison without normalization (RFC 3986
/// Section 6.2.1). A failure is reported to the client as `invalid_grant`
/// (RFC 6749 Section 5.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/jose.h>
/// #include <array>
/// #include <chrono>
/// #include <cassert>
///
/// const std::array<std::string_view, 1>
/// audiences{{"https://server.example/token"}};
/// sourcemeta::core::OAuthAssertionVerifyOptions options;
/// const auto error{sourcemeta::core::oauth_verify_assertion_grant(
///     assertion, "https://issuer.example", audiences, keys,
///     std::chrono::system_clock::now(), options)};
/// assert(!error.has_value() ||
///        error.value() == sourcemeta::core::OAuthAssertionError::Issuer);
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_verify_assertion_grant(
    const std::string_view assertion, const std::string_view expected_issuer,
    const std::span<const std::string_view> expected_audiences,
    const JWKS &keys, const std::chrono::system_clock::time_point now,
    const OAuthAssertionVerifyOptions &options)
    -> std::optional<OAuthAssertionError>;

} // namespace sourcemeta::core

#endif
