#include <sourcemeta/core/jose_verify.h>

#include <sourcemeta/core/text.h>

#include <algorithm>   // std::ranges::find
#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

// RFC 7519 Section 5.1: "a recipient using the media type value MUST treat it
// as if `application/` were prepended to any `typ` value not containing a `/`".
// Removing an explicit `application/` prefix, when nothing else in the value
// contains a slash, lets the compact `at+jwt` form and the full
// `application/at+jwt` form compare equal
auto strip_application_prefix(const std::string_view value)
    -> std::string_view {
  constexpr std::string_view prefix{"application/"};
  if (value.size() > prefix.size() &&
      sourcemeta::core::equals_ignore_case(value.substr(0, prefix.size()),
                                           prefix) &&
      value.find('/', prefix.size()) == std::string_view::npos) {
    return value.substr(prefix.size());
  }

  return value;
}

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
                const std::chrono::seconds clock_skew,
                const std::optional<std::string_view> expected_subject,
                const std::optional<std::string_view> expected_type)
    -> std::optional<JWTVerificationError> {
  // The algorithm allow-list is enforced before any key is touched, per step 3
  // of the Sourcemeta One validation algorithm
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
  if (expected_type.has_value()) {
    const auto type{token.type()};
    if (!type.has_value() ||
        !equals_ignore_case(strip_application_prefix(type.value()),
                            strip_application_prefix(expected_type.value()))) {
      return JWTVerificationError::Type;
    }
  }

  const auto claim_error{jwt_check_claims(token, expected_issuer,
                                          expected_audience, now, clock_skew,
                                          expected_subject)};
  if (claim_error.has_value()) {
    return to_verification_error(claim_error.value());
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
