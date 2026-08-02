#include <sourcemeta/core/jose_verify.h>

#include <sourcemeta/core/time.h>

#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace {

using Clock = std::chrono::system_clock;

// The skew is applied to the server clock rather than the attacker-controlled
// claim, so a NumericDate near the representable bound cannot overflow the
// comparison (the two forms are otherwise equivalent). The shift itself
// saturates, and the skew is bounded first, so an extreme caller-supplied clock
// or skew stays well-defined too
auto shift_backward(const Clock::time_point now,
                    const std::chrono::seconds skew) -> Clock::time_point {
  return sourcemeta::core::clock_shift_backward(
      now, sourcemeta::core::jwt_bounded_clock_skew(skew));
}

auto shift_forward(const Clock::time_point now, const std::chrono::seconds skew)
    -> Clock::time_point {
  return sourcemeta::core::clock_shift_forward(
      now, sourcemeta::core::jwt_bounded_clock_skew(skew));
}

} // namespace

namespace sourcemeta::core {

auto jwt_check_claims(const JWT &token, const std::string_view expected_issuer,
                      const std::string_view expected_audience,
                      const std::chrono::system_clock::time_point now,
                      const JWTClockSkew clock_skew,
                      const std::optional<std::string_view> expected_subject)
    -> std::optional<JWTClaimError> {
  // The issuer must be present and match the expected value (RFC 7519 Section
  // 4.1.1)
  const auto issuer{token.issuer()};
  if (!issuer.has_value() || issuer.value() != expected_issuer) {
    return JWTClaimError::Issuer;
  }

  // The subject is checked only when the caller pins one, since it is the
  // authenticated principal that many flows accept as any valid identity (RFC
  // 7519 Section 4.1.2)
  if (expected_subject.has_value()) {
    const auto subject{token.subject()};
    if (!subject.has_value() || subject.value() != expected_subject.value()) {
      return JWTClaimError::Subject;
    }
  }

  // The audience must be present and contain the expected value (RFC 7519
  // Section 4.1.3)
  if (!token.has_audience(expected_audience)) {
    return JWTClaimError::Audience;
  }

  // A bearer credential without an expiry is not acceptable for authentication,
  // so the claim is required here even though RFC 7519 makes it optional in
  // general (RFC 9068 Section 2.2)
  const auto expires_at{token.expires_at()};
  if (!expires_at.has_value() ||
      shift_backward(now, clock_skew.expiration) >= expires_at.value()) {
    return JWTClaimError::Expiration;
  }

  // The not-before time, when present, must be a usable NumericDate that is not
  // in the future. A claim that is present but malformed fails closed rather
  // than being ignored (RFC 7519 Section 4.1.5)
  const auto &payload{token.payload()};
  if (payload.defines("nbf")) {
    const auto not_before{token.not_before()};
    if (!not_before.has_value() ||
        shift_forward(now, clock_skew.not_before) < not_before.value()) {
      return JWTClaimError::NotBefore;
    }
  }

  // The issued-at time, when present, must be a usable NumericDate that is not
  // in the future (RFC 7519 Section 4.1.6)
  if (payload.defines("iat")) {
    const auto issued_at{token.issued_at()};
    if (!issued_at.has_value() ||
        shift_forward(now, clock_skew.issued_at) < issued_at.value()) {
      return JWTClaimError::IssuedAt;
    }
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
