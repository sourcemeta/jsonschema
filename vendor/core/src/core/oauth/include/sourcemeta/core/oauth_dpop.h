#ifndef SOURCEMETA_CORE_OAUTH_DPOP_H_
#define SOURCEMETA_CORE_OAUTH_DPOP_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwk_private.h>

#include <sourcemeta/core/json.h>

#include <array>       // std::array
#include <chrono>      // std::chrono::seconds, std::chrono::system_clock
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <map>         // std::map
#include <mutex>       // std::mutex
#include <optional>    // std::optional
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core {

/// @ingroup oauth
/// The token type of a DPoP-bound access token (RFC 9449 Section 5), the value
/// a bound token response carries and a client confirms before use.
inline constexpr std::string_view OAUTH_TOKEN_TYPE_DPOP{"DPoP"};

/// @ingroup oauth
/// A client-side minter of DPoP proof JSON Web Tokens (RFC 9449 Section 4.2)
/// that holds one proof-of-possession key and the most recent nonce each server
/// has issued. Nonces are tracked per server because a nonce is only accepted
/// by the server that issued it (RFC 9449 Section 9), so a caller keys the
/// authorization server by its issuer and each resource server by its origin.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <sourcemeta/core/jose.h>
/// #include <chrono>
/// #include <string>
/// #include <cassert>
///
/// auto key{sourcemeta::core::JWKPrivate::from_pem(pem)};
/// assert(key.has_value());
/// sourcemeta::core::OAuthDPoPProofer proofer{
///     std::move(key.value()), sourcemeta::core::JWSAlgorithm::ES256};
/// std::string header;
/// assert(proofer.proof("https://server.example.com", "POST",
///                      "https://server.example.com/token", "",
///                      std::chrono::system_clock::now(), header));
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthDPoPProofer {
public:
  /// Construct a proofer bound to a proof-of-possession key and the asymmetric
  /// signature algorithm to sign proofs with (RFC 9449 Section 4.2).
  OAuthDPoPProofer(JWKPrivate key, const JWSAlgorithm algorithm);

  /// A proofer owns a private key and a mutex, so it is neither copied nor
  /// moved.
  OAuthDPoPProofer(const OAuthDPoPProofer &) = delete;
  auto operator=(const OAuthDPoPProofer &) -> OAuthDPoPProofer & = delete;
  OAuthDPoPProofer(OAuthDPoPProofer &&) = delete;
  auto operator=(OAuthDPoPProofer &&) -> OAuthDPoPProofer & = delete;
  ~OAuthDPoPProofer() = default;

  /// Append a DPoP proof for a request to the sink, returning whether it could
  /// be built and signed. The method and target URI are covered by the proof,
  /// the access token binds the proof through its hash when it is presented to
  /// a protected resource and is omitted otherwise (RFC 9449 Section 7), and
  /// the most recent nonce the server issued is included when one is known. The
  /// creation time is taken from the given clock reading (RFC 9449
  /// Section 4.2).
  [[nodiscard]] auto
  proof(const std::string_view server, const std::string_view method,
        const std::string_view url, const std::string_view access_token,
        const std::chrono::system_clock::time_point now, std::string &sink)
      -> bool;

  /// Record the most recent nonce a server issued through its response header,
  /// to be included in later proofs to that server, ignoring a malformed nonce
  /// rather than echoing it (RFC 9449 Section 8).
  auto observe(const std::string_view server, const std::string_view nonce)
      -> void;

  /// The JSON Web Key thumbprint of the proof-of-possession key (RFC 9449
  /// Section 6.1), the value a client sends to bind an authorization code to
  /// the key (RFC 9449 Section 10), or no value when the public part cannot be
  /// recovered.
  [[nodiscard]] auto thumbprint() const -> std::optional<std::string>;

private:
  JWKPrivate key_;
  JWSAlgorithm algorithm_;
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  mutable std::mutex mutex_;
  std::map<std::string, std::string, std::less<>> nonces_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup oauth
/// Build the confirmation value that binds an access token to a DPoP key
/// (RFC 9449 Section 6.1), for a server to assign under the `cnf` claim of a
/// JSON Web Token access token or a top-level `cnf` of a token introspection
/// response (RFC 9449 Section 6.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto confirmation{sourcemeta::core::oauth_dpop_confirmation(
///     "0ZcOCORZNYy-DWpqq30jZyJGHTN0d2HglBV3uiguA4I")};
/// assert(confirmation.at("jkt").to_string() ==
///        "0ZcOCORZNYy-DWpqq30jZyJGHTN0d2HglBV3uiguA4I");
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_dpop_confirmation(const std::string_view thumbprint) -> JSON;

/// @ingroup oauth
/// The JSON Web Key thumbprint of the key a DPoP proof is signed with (RFC 9449
/// Section 6.1), the value a server matches against an authorization code
/// binding (RFC 9449 Section 10) or binds an issued token to, or no value when
/// the proof or its embedded key cannot be read. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto thumbprint{sourcemeta::core::oauth_dpop_proof_thumbprint(proof)};
/// assert(!thumbprint.has_value() || !thumbprint.value().empty());
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_dpop_proof_thumbprint(const std::string_view proof)
    -> std::optional<std::string>;

/// @ingroup oauth
/// The reason a DPoP proof failed verification (RFC 9449 Section 4.3), one per
/// check performed, with a missing nonce kept distinct from a mismatched one
/// because a server resupplies a nonce for the former (RFC 9449 Section 9).
enum class OAuthDPoPError : std::uint8_t {
  /// More or fewer than one DPoP header field was present (check 1).
  ProofCount,
  /// The header value was not a single well-formed JSON Web Token (check 2).
  Malformed,
  /// A required header parameter or claim was absent (check 3).
  MissingClaim,
  /// The token type header parameter was not `dpop+jwt` (check 4).
  UnexpectedType,
  /// The algorithm was absent, symmetric, or outside the accepted set
  /// (check 5).
  UnsupportedAlgorithm,
  /// The embedded key carried private material (check 7).
  PrivateKey,
  /// The signature did not verify with the embedded key (check 6).
  Signature,
  /// The method claim did not match the request method (check 8).
  MethodMismatch,
  /// The target claim did not match the request target (check 9).
  TargetMismatch,
  /// A nonce was required but absent, the downgrade the server must reject
  /// (check 10, RFC 9449 Section 11.3).
  MissingNonce,
  /// The nonce claim did not match the nonce the server issued (check 10).
  NonceMismatch,
  /// The creation time was outside the acceptable window (check 11).
  Expired,
  /// The access token hash was absent or did not match the presented token
  /// (check 12).
  AccessTokenMismatch,
  /// The proof key did not match the key the access token is bound to, or a
  /// token was presented with no binding to confirm it against (check 12).
  KeyMismatch
};

/// @ingroup oauth
/// The inputs to DPoP proof verification a caller supplies beyond the request
/// (RFC 9449 Section 4.3). Every field has a safe default, so a token endpoint
/// verifying a proof with no bound token leaves the token and thumbprint unset,
/// and a protected resource sets both.
struct OAuthDPoPVerifyOptions {
  /// The number of DPoP header fields the request carried, checked to be
  /// exactly one (RFC 9449 Section 4.3 check 1).
  std::size_t proof_count{1};
  /// The asymmetric algorithms accepted per local policy, every asymmetric
  /// algorithm when empty (RFC 9449 Section 4.3 check 5).
  std::span<const JWSAlgorithm> allowed_algorithms{};
  /// How far in the past a creation time may be (RFC 9449 Section 11.1).
  std::chrono::seconds past_window{std::chrono::seconds{300}};
  /// How far in the future a creation time may be, to tolerate clock offset
  /// (RFC 9449 Section 11.1).
  std::chrono::seconds future_window{std::chrono::seconds{5}};
  /// The nonce the server issued to the client, requiring a matching nonce
  /// claim when set (RFC 9449 Section 9).
  std::optional<std::string_view> expected_nonce{std::nullopt};
  /// The access token presented alongside the proof, requiring a matching hash
  /// claim when set, and requiring the bound thumbprint to also be set so the
  /// key binding is confirmed rather than skipped (RFC 9449 Section 7).
  std::optional<std::string_view> access_token{std::nullopt};
  /// The thumbprint the access token is bound to, requiring a matching proof
  /// key when set (RFC 9449 Section 4.3 check 12).
  std::optional<std::string_view> bound_thumbprint{std::nullopt};
};

/// @ingroup oauth
/// Verify a DPoP proof against the request it accompanies (RFC 9449
/// Section 4.3), returning the first failing check or no value when every check
/// passes. The checks are run in a fixed order though the specification permits
/// any order, and replay is a separate concern the caller enforces after this
/// passes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <chrono>
/// #include <cassert>
///
/// sourcemeta::core::OAuthDPoPVerifyOptions options;
/// const auto error{sourcemeta::core::oauth_dpop_verify(
///     proof, "POST", "https://server.example.com/token",
///     std::chrono::system_clock::now(), options)};
/// assert(!error.has_value() ||
///        error.value() == sourcemeta::core::OAuthDPoPError::Expired);
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto oauth_dpop_verify(
    const std::string_view proof, const std::string_view method,
    const std::string_view url, const std::chrono::system_clock::time_point now,
    const OAuthDPoPVerifyOptions &options) -> std::optional<OAuthDPoPError>;

/// @ingroup oauth
/// An in-memory store of the proof identifiers seen within their acceptance
/// window, to reject a replayed DPoP proof at one target (RFC 9449
/// Section 11.1). A store instance is safe to share across threads, it holds a
/// hash of each identifier rather than the identifier itself, and it is bounded
/// to a fixed capacity so an attacker minting distinct proofs cannot exhaust
/// memory. When full it fails closed, rejecting a new identifier rather than
/// evicting a live entry whose replay guard is still needed, so a flood costs
/// availability rather than replay protection. Give each entry a window that
/// covers the whole acceptance window a proof enjoys, the sum of the past and
/// future tolerances, so an entry never expires while its proof is still
/// accepted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <chrono>
/// #include <cassert>
///
/// sourcemeta::core::OAuthDPoPReplayStore store;
/// const auto now{std::chrono::system_clock::now()};
/// assert(store.check_and_insert("id", "https://server.example.com/token", now,
///                               std::chrono::seconds{300}));
/// assert(!store.check_and_insert("id", "https://server.example.com/token",
/// now,
///                                std::chrono::seconds{300}));
/// ```
class SOURCEMETA_CORE_OAUTH_EXPORT OAuthDPoPReplayStore {
public:
  /// The default maximum number of live entries a store retains.
  static constexpr std::size_t DEFAULT_CAPACITY{131072};

  /// Construct an empty store with the given maximum number of live entries,
  /// beyond which a new identifier is rejected until a slot frees on expiry.
  explicit OAuthDPoPReplayStore(
      const std::size_t capacity = DEFAULT_CAPACITY) noexcept
      : capacity_{capacity == 0 ? DEFAULT_CAPACITY : capacity} {}

  /// A store owns a mutex, so it is neither copied nor moved.
  OAuthDPoPReplayStore(const OAuthDPoPReplayStore &) = delete;
  auto operator=(const OAuthDPoPReplayStore &)
      -> OAuthDPoPReplayStore & = delete;
  OAuthDPoPReplayStore(OAuthDPoPReplayStore &&) = delete;
  auto operator=(OAuthDPoPReplayStore &&) -> OAuthDPoPReplayStore & = delete;
  ~OAuthDPoPReplayStore() = default;

  /// Record a proof identifier at a target and report whether it is new,
  /// returning false for one already seen within its window, which is a replay
  /// (RFC 9449 Section 11.1). Entries whose window has elapsed by the given
  /// time are pruned, and a new identifier is refused once the store is full of
  /// live entries, so the return also stands for a store that cannot admit it.
  /// The target is canonicalized as an HTTP target URI when `normalize_target`
  /// is set, so an equivalent but differently spelled DPoP target still
  /// collides (RFC 9449 Section 4.3 check 9), and keyed verbatim otherwise, for
  /// a target compared without normalization such as an assertion audience.
  [[nodiscard]] auto
  check_and_insert(const std::string_view identifier,
                   const std::string_view target,
                   const std::chrono::system_clock::time_point now,
                   const std::chrono::seconds window,
                   const bool normalize_target = true) -> bool;

  /// The number of entries whose window has not elapsed by the given time,
  /// counted without modifying the store, since expired entries are pruned when
  /// the next one is recorded.
  [[nodiscard]] auto size(const std::chrono::system_clock::time_point now) const
      -> std::size_t;

private:
  struct Entry {
    std::array<std::uint8_t, 32> digest;
    std::chrono::system_clock::time_point expiry;
  };

  std::size_t capacity_;
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  mutable std::mutex mutex_;
  std::vector<Entry> entries_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup oauth
/// Whether a value is a well-formed DPoP nonce, a non-empty string of the nonce
/// character set (RFC 9449 Section 8.1, RFC 6749 Appendix A), so a client
/// validates a received nonce before echoing it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_is_valid_dpop_nonce("eyJ7S_zG.eyJH0-Z.HX4w"));
/// assert(!sourcemeta::core::oauth_is_valid_dpop_nonce(""));
/// ```
[[nodiscard]] SOURCEMETA_CORE_OAUTH_EXPORT auto
oauth_is_valid_dpop_nonce(const std::string_view value) noexcept -> bool;

} // namespace sourcemeta::core

#endif
