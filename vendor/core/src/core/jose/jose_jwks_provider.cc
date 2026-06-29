#include <sourcemeta/core/jose_jwks_provider.h>

#include <sourcemeta/core/json.h> // sourcemeta::core::try_parse_json

#include <algorithm> // std::max, std::min
#include <chrono>    // std::chrono
#include <memory>    // std::make_shared, std::shared_ptr
#include <mutex>     // std::scoped_lock
#include <optional>  // std::optional, std::nullopt
#include <utility>   // std::move

namespace {

auto system_now() -> std::chrono::system_clock::time_point {
  return std::chrono::system_clock::now();
}

auto clamp_ttl(const std::optional<std::chrono::seconds> max_age,
               const sourcemeta::core::JWKSProvider::Options &options)
    -> std::chrono::seconds {
  // An advertised lifetime (including a real zero) is held within the
  // configured band. The absence of one falls back to the default lifetime. The
  // lower and upper limits are applied as separate comparisons so that a
  // configuration whose band is inverted, with the minimum above the maximum,
  // stays well defined rather than undefined, with the minimum taking
  // precedence
  if (max_age.has_value()) {
    return std::max(options.minimum_ttl,
                    std::min(max_age.value(), options.maximum_ttl));
  }

  return options.fallback_ttl;
}

} // namespace

namespace sourcemeta::core {

JWKSProvider::JWKSProvider(std::string jwks_uri, Fetcher fetcher)
    : JWKSProvider{std::move(jwks_uri), std::move(fetcher), Options{},
                   system_now} {}

JWKSProvider::JWKSProvider(std::string jwks_uri, Fetcher fetcher,
                           Options options)
    : JWKSProvider{std::move(jwks_uri), std::move(fetcher), options,
                   system_now} {}

JWKSProvider::JWKSProvider(std::string jwks_uri, Fetcher fetcher,
                           Options options, Clock clock)
    : jwks_uri_{std::move(jwks_uri)}, fetcher_{std::move(fetcher)},
      options_{options},
      // An empty clock falls back to the system clock so that a default
      // constructed one cannot make verification throw
      clock_{clock ? std::move(clock) : Clock{system_now}} {}

auto JWKSProvider::fetch_and_install_locked(
    const std::chrono::system_clock::time_point now) -> bool {
  std::optional<FetchResult> fetched;
  try {
    fetched = this->fetcher_(this->jwks_uri_);
  } catch (...) {
    // A fetcher signals failure by returning no value, but a throwing transport
    // is treated as just another failed retrieval, so a misbehaving fetcher can
    // never escape verification
    return false;
  }

  if (!fetched.has_value()) {
    return false;
  }

  auto document{try_parse_json(fetched.value().body)};
  if (!document.has_value()) {
    return false;
  }

  auto parsed{JWKS::from(std::move(document).value())};
  if (!parsed.has_value()) {
    return false;
  }

  this->keys_ = std::make_shared<const JWKS>(std::move(parsed).value());
  this->next_refresh_ =
      now + clamp_ttl(fetched.value().max_age, this->options_);
  return true;
}

auto JWKSProvider::refresh_locked(
    const std::chrono::system_clock::time_point now)
    -> std::shared_ptr<const JWKS> {
  if (this->keys_ == nullptr) {
    // Cold start: keep attempting on every call until a set is obtained, so the
    // provider recovers the instant the issuer becomes reachable
    this->fetch_and_install_locked(now);
  } else if (now >= this->next_refresh_) {
    if (!this->fetch_and_install_locked(now)) {
      // A failed refresh of a set we already hold serves the stale keys and
      // backs off, so a persistently unreachable issuer is not refetched on
      // every request. Only the ability to validate a freshly rotated key is
      // delayed, and that is separately bounded by the cooldown
      this->next_refresh_ = now + this->options_.minimum_ttl;
    }
  }

  return this->keys_;
}

auto JWKSProvider::refetch_for_unknown_kid_locked(
    const std::chrono::system_clock::time_point now)
    -> std::shared_ptr<const JWKS> {
  if (now < this->unknown_kid_cooldown_until_) {
    return nullptr;
  }

  // The cooldown is armed before the attempt so that tokens carrying random key
  // identifiers cannot drive a fetch on every request
  this->unknown_kid_cooldown_until_ = now + this->options_.unknown_kid_cooldown;
  if (this->fetch_and_install_locked(now)) {
    return this->keys_;
  }

  return nullptr;
}

auto JWKSProvider::verify(
    const JWT &token, const std::span<const JWSAlgorithm> allowed_algorithms,
    const std::string_view expected_issuer,
    const std::string_view expected_audience,
    const std::optional<std::string_view> expected_subject,
    const std::optional<std::string_view> expected_type)
    -> std::optional<JWTVerificationError> {
  const auto now{this->clock_()};

  std::shared_ptr<const JWKS> snapshot;
  {
    const std::scoped_lock lock{this->mutex_};
    snapshot = this->refresh_locked(now);
  }

  if (snapshot == nullptr) {
    return JWTVerificationError::UnknownKey;
  }

  const auto error{jwt_verify(
      token, *snapshot, allowed_algorithms, expected_issuer, expected_audience,
      now, this->options_.clock_skew, expected_subject, expected_type)};
  if (!error.has_value() || error.value() != JWTVerificationError::UnknownKey) {
    return error;
  }

  // No key in the current set could verify the token, whether because the key
  // it names is absent, because it names none and nothing matched, or because
  // only incompatible keys are present. That is the signal the set may be stale
  // after a rotation, so the provider performs one guarded, cooldown-bounded
  // refetch and retries once. A failure that is instead a present but
  // non-verifying key is never retried, since that is an attack or a corrupt
  // token, not a rotation
  std::shared_ptr<const JWKS> refreshed;
  {
    const std::scoped_lock lock{this->mutex_};
    refreshed = this->refetch_for_unknown_kid_locked(now);
  }

  if (refreshed == nullptr || refreshed == snapshot) {
    return error;
  }

  return jwt_verify(token, *refreshed, allowed_algorithms, expected_issuer,
                    expected_audience, now, this->options_.clock_skew,
                    expected_subject, expected_type);
}

} // namespace sourcemeta::core
