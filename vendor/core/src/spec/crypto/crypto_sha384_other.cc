#include <sourcemeta/core/crypto_sha384.h>

#include "crypto_sha2_64.h"

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t, std::uint64_t

namespace sourcemeta::core {

auto sha384_digest(const std::string_view input)
    -> std::array<std::uint8_t, 48> {
  // Initial hash values: first 64 bits of the fractional parts of the
  // square roots of the 9th through 16th primes (FIPS 180-4 Section 5.3.4)
  std::array<std::uint64_t, 8> state{
      {0xcbbb9d5dc1059ed8U, 0x629a292a367cd507U, 0x9159015a3070dd17U,
       0x152fecd8f70e5939U, 0x67332667ffc00b31U, 0x8eb44a8768581511U,
       0xdb0c2e0d64f98fa7U, 0x47b5481dbefa4fa4U}};

  sha2_64_hash(input, state);

  // The digest is the leftmost 384 bits of the final state
  // (FIPS 180-4 Section 6.5)
  std::array<std::uint8_t, 48> digest{};
  for (std::size_t word_index = 0u; word_index < 6u; ++word_index) {
    for (std::size_t byte_index = 0u; byte_index < 8u; ++byte_index) {
      digest[(word_index * 8u) + byte_index] = static_cast<std::uint8_t>(
          (state[word_index] >> (8u * (7u - byte_index))) & 0xffu);
    }
  }

  return digest;
}

} // namespace sourcemeta::core
