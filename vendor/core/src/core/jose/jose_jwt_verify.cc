#include <sourcemeta/core/jose_verify.h>

#include <algorithm>   // std::ranges::find
#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto to_verification_error(const sourcemeta::core::JWTClaimError error)
    -> sourcemeta::core::JWTVerificationError {
  using sourcemeta::core::JWTClaimError;
  using sourcemeta::core::JWTVerificationError;
  switch (error) {
    case JWTClaimError::Issuer:
      return JWTVerificationError::Issuer;
    case JWTClaimError::Subject:
      return JWTVerificationError::Subject;
    case JWTClaimError::Audience:
      return JWTVerificationError::Audience;
    case JWTClaimError::Expiration:
      return JWTVerificationError::Expiration;
    case JWTClaimError::NotBefore:
      return JWTVerificationError::NotBefore;
    case JWTClaimError::IssuedAt:
      return JWTVerificationError::IssuedAt;
    case JWTClaimError::Lifetime:
      return JWTVerificationError::Lifetime;
  }

  std::unreachable();
}

} // namespace

namespace sourcemeta::core {

auto jwt_verify(const JWT &token, const JWKS &keys,
                const std::span<const JWSAlgorithm> allowed_algorithms,
                const std::string_view expected_issuer,
                const std::string_view expected_audience,
                const std::chrono::system_clock::time_point now,
                const JWTClockSkew clock_skew,
                const std::optional<std::string_view> expected_subject,
                const std::optional<std::string_view> expected_type,
                const std::optional<std::chrono::seconds> maximum_lifetime)
    -> std::optional<JWTVerificationError> {
  // RFC 8725 Section 3.1: "Libraries MUST enable the caller to specify a
  // supported set of algorithms and MUST NOT use any other algorithms when
  // performing cryptographic operations", so the allow-list is enforced before
  // any key is touched
  const auto algorithm{token.algorithm()};
  if (!algorithm.has_value() ||
      std::ranges::find(allowed_algorithms, algorithm.value()) ==
          allowed_algorithms.end()) {
    return JWTVerificationError::AlgorithmNotAllowed;
  }

  // A token names its key through `kid` (RFC 7515 Section 4.1.4). When it does
  // not, every key in the set is tried, since some providers omit it when they
  // publish a single key. A missing or non-verifying key is reported as unknown
  // rather than as a signature failure so that downstream can refetch the set,
  // except when the named key is present but its signature does not verify
  const auto key_id{token.key_id()};
  if (key_id.has_value()) {
    const auto *key{keys.find(key_id.value())};
    if (key == nullptr) {
      return JWTVerificationError::UnknownKey;
    }

    if (!jwt_verify_signature(token, *key)) {
      return JWTVerificationError::Signature;
    }
  } else {
    bool verified{false};
    for (const auto &key : keys) {
      if (jwt_verify_signature(token, key)) {
        verified = true;
        break;
      }
    }

    if (!verified) {
      return JWTVerificationError::UnknownKey;
    }
  }

  // The type is a header concern checked only on an authenticated token, which
  // is how the access token profile is enforced (RFC 9068 Section 2.1)
  if (expected_type.has_value() && !token.has_type(expected_type.value())) {
    return JWTVerificationError::Type;
  }

  const auto claim_error{jwt_check_claims(token, expected_issuer,
                                          expected_audience, now, clock_skew,
                                          expected_subject, maximum_lifetime)};
  if (claim_error.has_value()) {
    return to_verification_error(claim_error.value());
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
