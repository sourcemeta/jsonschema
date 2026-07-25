#include <sourcemeta/core/oidc_hash.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto oidc_token_hash(const std::string_view token, const JWSAlgorithm algorithm)
    -> std::optional<std::string> {
  // The correct digest for EdDSA depends on the signing curve, SHA-512 for
  // Ed25519 but SHAKE256 for Ed448, which the algorithm alone does not convey.
  // Selecting SHA-512 for every EdDSA value would silently produce a wrong
  // binding for Ed448, so it is rejected rather than guessed
  if (algorithm == JWSAlgorithm::EdDSA) {
    return std::nullopt;
  }

  // OpenID Connect Core 1.0 Section 3.1.3.6: "hash the octets of the ASCII
  // representation ... with the hash algorithm used ... take the left-most half
  // of the hash and base64url-encode it". The digest is selected by an explicit
  // table rather than by slicing the algorithm name
  switch (jws_algorithm_digest_bits(algorithm)) {
    case 256: {
      const auto digest{sha256_digest(token)};
      return base64url_encode(std::span<const std::uint8_t>{digest.data(), 16});
    }
    case 384: {
      const auto digest{sha384_digest(token)};
      return base64url_encode(std::span<const std::uint8_t>{digest.data(), 24});
    }
    case 512: {
      const auto digest{sha512_digest(token)};
      return base64url_encode(std::span<const std::uint8_t>{digest.data(), 32});
    }
    default:
      // A defensive fallback for a future algorithm whose digest size is not
      // one of the three the currently defined algorithms use
      return std::nullopt;
  }
}

auto oidc_verify_token_hash(const std::string_view token,
                            const JWSAlgorithm algorithm,
                            const std::string_view claim) -> bool {
  const auto expected{oidc_token_hash(token, algorithm)};
  return expected.has_value() && secure_equals(expected.value(), claim);
}

} // namespace sourcemeta::core
