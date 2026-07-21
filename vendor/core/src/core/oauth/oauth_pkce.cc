#include <sourcemeta/core/oauth_pkce.h>

#include <sourcemeta/core/crypto.h>

#include "oauth_syntax.h"

#include <algorithm>   // std::ranges::copy
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto oauth_pkce_method_code(const OAuthPKCEMethod method) noexcept
    -> std::string_view {
  switch (method) {
    case OAuthPKCEMethod::S256:
      return "S256";
    case OAuthPKCEMethod::Plain:
      return "plain";
  }

  std::unreachable();
}

auto to_oauth_pkce_method(const std::string_view value) noexcept
    -> std::optional<OAuthPKCEMethod> {
  if (value == "S256") {
    return OAuthPKCEMethod::S256;
  } else if (value == "plain") {
    return OAuthPKCEMethod::Plain;
  } else {
    return std::nullopt;
  }
}

auto oauth_pkce_verifier() -> std::array<char, 43> {
  // The verifier is the base64url of 32 random octets, so both the entropy and
  // the encoded form are secret. The scope guard wipes the entropy on every
  // exit, including an exception while encoding. The caller owns the returned
  // value and is responsible for wiping it in turn
  std::string entropy(32, '\x00');
  const SecureStringScope entropy_scope{entropy};
  random_bytes(
      std::span<std::uint8_t>{reinterpret_cast<std::uint8_t *>(entropy.data()),
                              entropy.size()}); // NOLINT
  SecureString encoded;
  base64url_encode(entropy, encoded);

  std::array<char, 43> result{};
  std::ranges::copy(std::string_view{encoded}, result.begin());
  return result;
}

auto oauth_pkce_challenge(const std::string_view verifier)
    -> std::array<char, 43> {
  const auto digest{sha256_digest(verifier)};
  const auto encoded{base64url_encode(
      std::string_view{reinterpret_cast<const char *>(digest.data()), // NOLINT
                       digest.size()})};

  std::array<char, 43> result{};
  std::ranges::copy(encoded, result.begin());
  return result;
}

auto oauth_pkce_verify(const std::string_view verifier,
                       const std::string_view challenge,
                       const OAuthPKCEMethod method, const OAuthProfile profile)
    -> OAuthPKCEOutcome {
  if (challenge.empty()) {
    // RFC 9700 Section 2.1.1: a presented verifier with no bound challenge is
    // rejected, since it signals a possible authorization code injection
    return verifier.empty() ? OAuthPKCEOutcome::NotUsed
                            : OAuthPKCEOutcome::MissingChallenge;
  }

  // OAuth 2.1 Section 4.1.3: iff the authorization request carried a challenge,
  // the token request must carry a verifier
  if (verifier.empty()) {
    return OAuthPKCEOutcome::MissingVerifier;
  }

  if (!oauth_is_pkce_verifier(verifier)) {
    return OAuthPKCEOutcome::MalformedVerifier;
  }

  switch (method) {
    case OAuthPKCEMethod::Plain:
      // RFC 9700 Section 2.1.1 hardening: the plain method is rejected under
      // the strict profile
      if (profile == OAuthProfile::Strict) {
        return OAuthPKCEOutcome::MethodNotAllowed;
      }

      if (!oauth_is_pkce_challenge(challenge)) {
        return OAuthPKCEOutcome::MalformedChallenge;
      }

      // RFC 7636 Section 4.6: the plain method compares the two values directly
      return secure_equals(verifier, challenge) ? OAuthPKCEOutcome::Match
                                                : OAuthPKCEOutcome::Mismatch;
    case OAuthPKCEMethod::S256: {
      // RFC 7636 Section 4.2: an S256 challenge is the base64url of a SHA-256
      // digest, so it is always exactly 43 characters
      if (challenge.size() != 43 || !oauth_is_pkce_challenge(challenge)) {
        return OAuthPKCEOutcome::MalformedChallenge;
      }

      const auto computed{oauth_pkce_challenge(verifier)};
      return secure_equals(std::string_view{computed.data(), computed.size()},
                           challenge)
                 ? OAuthPKCEOutcome::Match
                 : OAuthPKCEOutcome::Mismatch;
    }
  }

  std::unreachable();
}

} // namespace sourcemeta::core
