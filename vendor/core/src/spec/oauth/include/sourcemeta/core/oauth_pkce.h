#ifndef SOURCEMETA_CORE_OAUTH_PKCE_H_
#define SOURCEMETA_CORE_OAUTH_PKCE_H_

#ifndef SOURCEMETA_CORE_OAUTH_EXPORT
#include <sourcemeta/core/oauth_export.h>
#endif

#include <sourcemeta/core/oauth_profile.h>

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup oauth
/// The PKCE code challenge transformation method (RFC 7636 Section 4.2).
enum class OAuthPKCEMethod : std::uint8_t {
  /// The SHA-256 transformation, the only method the strict profile permits
  /// (RFC 7636 Section 4.2).
  S256,
  /// The identity transformation, permitted only under the compatible profile
  /// (RFC 7636 Section 4.2).
  Plain
};

/// @ingroup oauth
/// The result of verifying a PKCE code verifier against a stored code
/// challenge. Only `Match` and `NotUsed` let the exchange proceed. The
/// remaining outcomes each name a distinct pairing failure the token endpoint
/// rejects (RFC 7636 Section 4.6, RFC 9700 Section 2.1.1, OAuth 2.1
/// Section 4.1.3).
enum class OAuthPKCEOutcome : std::uint8_t {
  /// The verifier corresponds to the challenge.
  Match,
  /// Neither a challenge nor a verifier is present, so PKCE was not used. The
  /// caller decides separately whether its profile required it.
  NotUsed,
  /// A challenge is stored but no verifier was presented (OAuth 2.1
  /// Section 4.1.3).
  MissingVerifier,
  /// A verifier was presented but no challenge is stored, which signals a
  /// possible authorization code injection (RFC 9700 Section 2.1.1).
  MissingChallenge,
  /// Both are present, well formed, and permitted, but the verifier does not
  /// correspond to the challenge (RFC 7636 Section 4.6).
  Mismatch,
  /// The stored method is `plain` under the strict profile (RFC 9700
  /// Section 2.1.1).
  MethodNotAllowed,
  /// The presented verifier is not a valid code verifier (RFC 7636
  /// Section 4.1).
  MalformedVerifier,
  /// The stored challenge violates the code challenge syntax, either the wrong
  /// length for its method or a character outside the allowed set (RFC 7636
  /// Section 4.2). A syntactically valid challenge that simply does not
  /// correspond to the verifier is a `Mismatch`, not this.
  MalformedChallenge
};

/// @ingroup oauth
/// The wire value for a PKCE challenge method (RFC 7636 Section 4.3). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::oauth_pkce_method_code(
///            sourcemeta::core::OAuthPKCEMethod::S256) == "S256");
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_pkce_method_code(const OAuthPKCEMethod method) noexcept
    -> std::string_view;

/// @ingroup oauth
/// Map a PKCE challenge method wire value to its type, returning no value for
/// an unrecognized method (RFC 7636 Section 4.3). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_oauth_pkce_method("S256").has_value());
/// assert(!sourcemeta::core::to_oauth_pkce_method("plaintext").has_value());
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto to_oauth_pkce_method(const std::string_view value) noexcept
    -> std::optional<OAuthPKCEMethod>;

/// @ingroup oauth
/// Mint a new PKCE code verifier, the base64url encoding of 32
/// cryptographically random octets (RFC 7636 Section 4.1). The 43 bytes are not
/// null terminated, so pass them onward as a view of `data()` and `size()`
/// rather than as a C string. The result is secret material, so wipe it once
/// the exchange completes. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string_view>
///
/// const auto verifier{sourcemeta::core::oauth_pkce_verifier()};
/// const std::string_view view{verifier.data(), verifier.size()};
/// assert(view.size() == 43);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_pkce_verifier() -> std::array<char, 43>;

/// @ingroup oauth
/// Derive the S256 code challenge for a code verifier, the base64url encoding
/// of its SHA-256 digest (RFC 7636 Section 4.2). For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
/// #include <string_view>
///
/// const auto challenge{sourcemeta::core::oauth_pkce_challenge(
///     "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk")};
/// assert((std::string_view{challenge.data(), challenge.size()} ==
///         "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_pkce_challenge(const std::string_view verifier)
    -> std::array<char, 43>;

/// @ingroup oauth
/// Verify a presented code verifier against a stored code challenge and its
/// method, under the given profile, returning the pairing outcome. An empty
/// verifier or challenge means the value is absent. The final verifier against
/// challenge comparison is constant time over their bytes once their lengths
/// match. For example:
///
/// ```cpp
/// #include <sourcemeta/core/oauth.h>
/// #include <cassert>
///
/// const auto outcome{sourcemeta::core::oauth_pkce_verify(
///     "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk",
///     "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM",
///     sourcemeta::core::OAuthPKCEMethod::S256,
///     sourcemeta::core::OAuthProfile::Strict)};
/// assert(outcome == sourcemeta::core::OAuthPKCEOutcome::Match);
/// ```
SOURCEMETA_CORE_OAUTH_EXPORT
auto oauth_pkce_verify(const std::string_view verifier,
                       const std::string_view challenge,
                       const OAuthPKCEMethod method, const OAuthProfile profile)
    -> OAuthPKCEOutcome;

} // namespace sourcemeta::core

#endif
