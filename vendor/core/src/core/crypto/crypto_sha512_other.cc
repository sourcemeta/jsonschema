#include <sourcemeta/core/crypto_sha512.h>

#include "crypto_sha2_64.h"

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t, std::uint64_t

namespace sourcemeta::core {

auto sha512_digest(const std::string_view input)
    -> std::array<std::uint8_t, 64> {
  // Initial hash values: first 64 bits of the fractional parts of the
  // square roots of the first 8 primes (FIPS 180-4 Section 5.3.5)
  std::array<std::uint64_t, 8> state{
      {0x6a09e667f3bcc908U, 0xbb67ae8584caa73bU, 0x3c6ef372fe94f82bU,
       0xa54ff53a5f1d36f1U, 0x510e527fade682d1U, 0x9b05688c2b3e6c1fU,
       0x1f83d9abfb41bd6bU, 0x5be0cd19137e2179U}};

  sha2_64_hash(input, state);

  std::array<std::uint8_t, 64> digest{};
  for (std::size_t word_index = 0u; word_index < 8u; ++word_index) {
    for (std::size_t byte_index = 0u; byte_index < 8u; ++byte_index) {
      digest[(word_index * 8u) + byte_index] = static_cast<std::uint8_t>(
          (state[word_index] >> (8u * (7u - byte_index))) & 0xffu);
    }
  }

  return digest;
}

} // namespace sourcemeta::core
