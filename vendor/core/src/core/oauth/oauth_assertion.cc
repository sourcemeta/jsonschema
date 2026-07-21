#include <sourcemeta/core/oauth_assertion.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwt.h>
#include <sourcemeta/core/jose_sign.h>
#include <sourcemeta/core/jose_verify.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_random.h>

#include "oauth_encode.h"

#include <algorithm>   // std::ranges::find_if, std::clamp
#include <chrono>      // std::chrono::seconds, std::chrono::duration_cast
#include <cstdint>     // std::int64_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_ALG{JSON::Object::hash("alg"sv)};
const auto HASH_KID{JSON::Object::hash("kid"sv)};
const auto HASH_ISS{JSON::Object::hash("iss"sv)};
const auto HASH_SUB{JSON::Object::hash("sub"sv)};
const auto HASH_AUD{JSON::Object::hash("aud"sv)};
const auto HASH_EXP{JSON::Object::hash("exp"sv)};
const auto HASH_IAT{JSON::Object::hash("iat"sv)};
const auto HASH_JTI{JSON::Object::hash("jti"sv)};

auto to_epoch_seconds(const std::chrono::system_clock::time_point point)
    -> std::int64_t {
  return static_cast<std::int64_t>(
      std::chrono::duration_cast<std::chrono::seconds>(point.time_since_epoch())
          .count());
}

auto map_verification_error(const JWTVerificationError error)
    -> OAuthAssertionError {
  switch (error) {
    case JWTVerificationError::AlgorithmNotAllowed:
      return OAuthAssertionError::UnsupportedAlgorithm;
    case JWTVerificationError::UnknownKey:
      return OAuthAssertionError::UnknownKey;
    case JWTVerificationError::Signature:
      return OAuthAssertionError::Signature;
    case JWTVerificationError::Type:
      return OAuthAssertionError::Malformed;
    case JWTVerificationError::Issuer:
      return OAuthAssertionError::Issuer;
    case JWTVerificationError::Subject:
      return OAuthAssertionError::Subject;
    case JWTVerificationError::Audience:
      return OAuthAssertionError::Audience;
    case JWTVerificationError::Expiration:
      return OAuthAssertionError::Expired;
    case JWTVerificationError::NotBefore:
      return OAuthAssertionError::NotYetValid;
    case JWTVerificationError::IssuedAt:
      return OAuthAssertionError::IssuedInFuture;
  }

  std::unreachable();
}

// RFC 7519 Section 4.1.3: the audience is a string or an array of strings, so
// an array carrying a non-string element is a malformed claim the assertion is
// rejected for (RFC 7523 Section 3 check 10) rather than accepted because
// another element happens to match
auto audience_is_well_formed(const JWT &token) -> bool {
  if (!token.payload().is_object()) {
    return false;
  }

  const auto *audience{token.payload().try_at("aud"sv, HASH_AUD)};
  if (audience == nullptr || audience->is_string()) {
    return true;
  }

  if (!audience->is_array()) {
    return false;
  }

  for (const auto &element : audience->as_array()) {
    if (!element.is_string()) {
      return false;
    }
  }

  return true;
}

// The shared verification of a parsed JWT bearer assertion (RFC 7523
// Section 3): find the audience the assertion targets, delegate the signature
// and time checks to the JSON Web Token verifier, and reject a replayed
// identifier
auto verify_assertion(
    const JWT &token, const std::string_view expected_issuer,
    const std::optional<std::string_view> expected_subject,
    const std::span<const std::string_view> expected_audiences,
    const JWKS &keys, const std::chrono::system_clock::time_point now,
    const OAuthAssertionVerifyOptions &options)
    -> std::optional<OAuthAssertionError> {
  if (!audience_is_well_formed(token)) {
    return OAuthAssertionError::Audience;
  }

  // RFC 7523 Section 3 check 3 and RFC 3986 Section 6.2.1: the audience is
  // matched by Simple String Comparison, so an accepted value is compared to
  // the string or array claim without any normalization
  const auto match{std::ranges::find_if(
      expected_audiences, [&token](const std::string_view audience) -> bool {
        return token.has_audience(audience);
      })};
  if (match == expected_audiences.end()) {
    return OAuthAssertionError::Audience;
  }

  const auto error{jwt_verify(token, keys, options.allowed_algorithms,
                              expected_issuer, *match, now, options.clock_skew,
                              expected_subject, std::nullopt)};
  if (error.has_value()) {
    return map_verification_error(error.value());
  }

  // RFC 7523 Section 3 check 7: an identifier is tracked for as long as the
  // assertion would be accepted, scoped to the audience it was issued for. The
  // clock skew is added to the remaining lifetime so the entry outlives the
  // grace window in which the assertion is still accepted past its expiration
  if (options.replay_store != nullptr) {
    const auto identifier{token.token_id()};
    const auto expiration{token.expires_at()};
    if (identifier.has_value() && expiration.has_value()) {
      // The skew is clamped to the same non-negative bounded range the claim
      // check applies, so a negative value cannot shorten the window below the
      // acceptance window and a large one cannot overflow the store's expiry
      const auto skew{std::clamp(options.clock_skew,
                                 std::chrono::seconds::zero(),
                                 std::chrono::seconds{31556952})};
      const auto remaining{std::chrono::duration_cast<std::chrono::seconds>(
                               expiration.value() - now) +
                           skew};
      const auto window{remaining > std::chrono::seconds{0}
                            ? remaining
                            : std::chrono::seconds{0}};
      // The audience is keyed verbatim, since assertion audiences are compared
      // by Simple String Comparison and must not be URL-normalized (RFC 7523
      // Section 3, RFC 3986 Section 6.2.1)
      if (!options.replay_store->check_and_insert(identifier.value(), *match,
                                                  now, window, false)) {
        return OAuthAssertionError::Replay;
      }
    }
  }

  return std::nullopt;
}

} // namespace

auto oauth_build_assertion(const std::string_view issuer,
                           const std::string_view subject,
                           const std::string_view audience,
                           const std::chrono::seconds lifetime,
                           const std::chrono::system_clock::time_point now,
                           const JWKPrivate &key, const JWSAlgorithm algorithm)
    -> std::optional<std::string> {
  auto header{JSON::make_object()};
  header.assign_assume_new("alg", JSON{jws_algorithm_name(algorithm)},
                           HASH_ALG);
  // The key identifier lets the server select the verifying key (RFC 7515
  // Section 4.1.4)
  const auto key_id{key.key_id()};
  if (key_id.has_value()) {
    header.assign_assume_new("kid", JSON{key_id.value()}, HASH_KID);
  }

  auto payload{JSON::make_object()};
  payload.assign_assume_new("iss", JSON{issuer}, HASH_ISS);
  payload.assign_assume_new("sub", JSON{subject}, HASH_SUB);
  payload.assign_assume_new("aud", JSON{audience}, HASH_AUD);
  payload.assign_assume_new("exp", JSON{to_epoch_seconds(now + lifetime)},
                            HASH_EXP);
  payload.assign_assume_new("iat", JSON{to_epoch_seconds(now)}, HASH_IAT);
  const auto identifier{oauth_random_token()};
  payload.assign_assume_new(
      "jti", JSON{std::string_view{identifier.data(), identifier.size()}},
      HASH_JTI);

  return jwt_sign(header, payload, key);
}

auto oauth_build_client_assertion(
    const std::string_view client_id, const std::string_view audience,
    const std::chrono::seconds lifetime,
    const std::chrono::system_clock::time_point now, const JWKPrivate &key,
    const JWSAlgorithm algorithm) -> std::optional<std::string> {
  // RFC 7521 Section 5.2: a self-issued client authentication assertion has the
  // client identifier as both its issuer and its subject
  return oauth_build_assertion(client_id, client_id, audience, lifetime, now,
                               key, algorithm);
}

auto oauth_client_assertion(const std::string_view assertion,
                            SecureString &sink) -> void {
  oauth_append_form_parameter(sink, "client_assertion_type",
                              OAUTH_CLIENT_ASSERTION_TYPE_JWT_BEARER);
  oauth_append_form_parameter(sink, "client_assertion", assertion);
}

auto oauth_build_token_request_jwt_bearer(const std::string_view assertion,
                                          const std::string_view scope,
                                          SecureString &sink) -> void {
  oauth_append_form_parameter(sink, "grant_type", OAUTH_GRANT_TYPE_JWT_BEARER);
  oauth_append_form_parameter(sink, "assertion", assertion);
  if (!scope.empty()) {
    oauth_append_form_parameter(sink, "scope", scope);
  }
}

auto oauth_verify_client_assertion(
    const std::string_view assertion,
    const std::span<const std::string_view> expected_audiences,
    const std::string_view request_client_id, const JWKS &keys,
    const std::chrono::system_clock::time_point now,
    const OAuthAssertionVerifyOptions &options)
    -> std::optional<OAuthAssertionError> {
  const auto token{JWT::from(assertion)};
  if (!token.has_value()) {
    return OAuthAssertionError::Malformed;
  }

  const auto subject{token.value().subject()};
  if (!subject.has_value()) {
    return OAuthAssertionError::Subject;
  }

  // RFC 7521 Section 5.2: the issuer and subject are both the client
  // identifier, and RFC 7521 Section 4.2: a presented client_id must identify
  // the same client as the assertion subject
  const auto expected_subject{request_client_id.empty() ? subject.value()
                                                        : request_client_id};
  return verify_assertion(token.value(), subject.value(), expected_subject,
                          expected_audiences, keys, now, options);
}

auto oauth_verify_assertion_grant(
    const std::string_view assertion, const std::string_view expected_issuer,
    const std::span<const std::string_view> expected_audiences,
    const JWKS &keys, const std::chrono::system_clock::time_point now,
    const OAuthAssertionVerifyOptions &options)
    -> std::optional<OAuthAssertionError> {
  const auto token{JWT::from(assertion)};
  if (!token.has_value()) {
    return OAuthAssertionError::Malformed;
  }

  // RFC 7523 Section 3 check 2: a subject is REQUIRED, though for a grant it
  // identifies the accessor rather than being constrained to a known value
  if (!token.value().subject().has_value()) {
    return OAuthAssertionError::Subject;
  }

  return verify_assertion(token.value(), expected_issuer, std::nullopt,
                          expected_audiences, keys, now, options);
}

} // namespace sourcemeta::core
