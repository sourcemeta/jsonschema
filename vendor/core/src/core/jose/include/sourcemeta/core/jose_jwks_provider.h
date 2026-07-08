#ifndef SOURCEMETA_CORE_JOSE_JWKS_PROVIDER_H_
#define SOURCEMETA_CORE_JOSE_JWKS_PROVIDER_H_

#ifndef SOURCEMETA_CORE_JOSE_EXPORT
#include <sourcemeta/core/jose_export.h>
#endif

#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwks.h>
#include <sourcemeta/core/jose_jwt.h>
#include <sourcemeta/core/jose_verify.h>

#include <chrono>      // std::chrono
#include <functional>  // std::function
#include <memory>      // std::shared_ptr
#include <mutex>       // std::mutex
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// A stateful, thread-safe resolver that owns a key set URL and verifies tokens
/// against it, fetching and refreshing the keys internally. It adds caching, a
/// freshness-aware refresh, a guarded refetch on rotation, and serving of the
/// previously held keys through a transient outage. The current time is read
/// internally from an injectable clock, so a caller never deals with it and
/// tests can drive expiry, rotation, and the refetch cooldown
/// deterministically.
class SOURCEMETA_CORE_JOSE_EXPORT JWKSProvider {
public:
  /// The outcome of one key set retrieval. The freshness hint is kept separate
  /// from the body because only the caller that made the request can observe
  /// the caching response header. An absent hint means no lifetime was
  /// advertised, which is distinct from an advertised lifetime of zero.
  struct FetchResult {
    /// The raw key set bytes.
    std::string body;
    /// The advertised freshness lifetime, absent when none was given.
    std::optional<std::chrono::seconds> max_age;
  };

  /// A pluggable transport that turns a URL into raw key set bytes plus an
  /// optional freshness hint. Returns no value on a failed retrieval, such as a
  /// transport error, an unsuccessful response, or an oversized body. Injecting
  /// the transport keeps this module free of any networking dependency and
  /// makes the provider substitutable for testing.
  using Fetcher =
      std::function<std::optional<FetchResult>(std::string_view url)>;

  /// A source of the current time. It defaults to the system clock and exists
  /// as an injection point so that expiry, rotation, and the refetch cooldown
  /// can be driven deterministically under test.
  using Clock = std::function<std::chrono::system_clock::time_point()>;

  /// Tunables for the caching and verification policy.
  struct Options {
    /// The lifetime to assume when the transport advertises none.
    std::chrono::seconds fallback_ttl{std::chrono::hours{1}};
    /// The shortest lifetime honored, clamping smaller advertised values up.
    std::chrono::seconds minimum_ttl{std::chrono::minutes{5}};
    /// The longest lifetime honored, clamping larger advertised values down.
    std::chrono::seconds maximum_ttl{std::chrono::hours{24}};
    /// The wait before another refetch is allowed after an unknown key
    /// identifier.
    std::chrono::seconds unknown_kid_cooldown{std::chrono::minutes{5}};
    /// The tolerance applied to time-based claims.
    std::chrono::seconds clock_skew{0};
  };

  /// Construct a provider for a concrete key set URL with an injected
  /// transport, using the default policy and the system clock.
  JWKSProvider(std::string jwks_uri, Fetcher fetcher);
  /// Construct a provider for a concrete key set URL with an injected
  /// transport, overriding the caching and verification policy.
  JWKSProvider(std::string jwks_uri, Fetcher fetcher, Options options);
  /// Construct a provider for a concrete key set URL with an injected
  /// transport, overriding the policy and the clock.
  JWKSProvider(std::string jwks_uri, Fetcher fetcher, Options options,
               Clock clock);

  /// The provider owns a lock guarding its cache, so it is neither copyable
  /// nor movable. Store it behind a pointer or construct it in place.
  JWKSProvider(const JWKSProvider &) = delete;
  auto operator=(const JWKSProvider &) -> JWKSProvider & = delete;
  JWKSProvider(JWKSProvider &&) = delete;
  auto operator=(JWKSProvider &&) -> JWKSProvider & = delete;

  /// Verify a token against the provider's key set, fetching or refreshing the
  /// keys as needed. Returns no value when the token is fully valid, otherwise
  /// the first failing step. The current time and the clock skew tolerance are
  /// the provider's own concern, so a caller supplies only what identifies the
  /// token: an optional expected subject and an optional expected type for the
  /// access token profile.
  [[nodiscard]] auto
  verify(const JWT &token,
         const std::span<const JWSAlgorithm> allowed_algorithms,
         const std::string_view expected_issuer,
         const std::string_view expected_audience,
         const std::optional<std::string_view> expected_subject = std::nullopt,
         const std::optional<std::string_view> expected_type = std::nullopt)
      -> std::optional<JWTVerificationError>;

private:
  auto fetch_and_install_locked(std::chrono::system_clock::time_point now)
      -> bool;
  auto refresh_locked(std::chrono::system_clock::time_point now)
      -> std::shared_ptr<const JWKS>;
  auto refetch_for_unknown_kid_locked(std::chrono::system_clock::time_point now)
      -> std::shared_ptr<const JWKS>;

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  const std::string jwks_uri_;
  const Fetcher fetcher_;
  const Options options_;
  const Clock clock_;
  std::mutex mutex_;
  std::shared_ptr<const JWKS> keys_;
  std::chrono::system_clock::time_point next_refresh_{};
  std::chrono::system_clock::time_point unknown_kid_cooldown_until_{};
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
