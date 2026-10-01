#include <sourcemeta/core/oidc_hash.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// The left-most half of a digest, base64url encoded, which is what the binding
// of a token to an identity token is made of
template <std::size_t Size>
auto encode_left_half(const std::array<std::uint8_t, Size> &digest)
    -> std::string {
  return sourcemeta::core::base64url_encode(
      std::span<const std::uint8_t>{digest.data(), Size / 2});
}

} // namespace

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
    case 256:
      return encode_left_half(sha256_digest(token));
    case 384:
      return encode_left_half(sha384_digest(token));
    case 512:
      return encode_left_half(sha512_digest(token));
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
