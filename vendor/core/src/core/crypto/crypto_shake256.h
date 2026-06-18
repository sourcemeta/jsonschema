#ifndef SOURCEMETA_CORE_CRYPTO_SHAKE256_H_
#define SOURCEMETA_CORE_CRYPTO_SHAKE256_H_

// The SHAKE256 extendable-output function (FIPS 202), built on the Keccak-f
// [1600] permutation, for the reference Ed448 verification backend

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

inline constexpr std::array<std::uint64_t, 24> keccak_round_constants{
    {0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
     0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
     0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
     0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
     0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
     0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
     0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
     0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL}};

// The rotation offsets and destination lanes of the combined rho and pi steps,
// walking the lanes starting from lane 1
inline constexpr std::array<unsigned, 24> keccak_rho_offsets{
    {1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
     27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44}};

inline constexpr std::array<unsigned, 24> keccak_pi_lanes{
    {10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
     15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1}};

inline constexpr auto keccak_rotate_left(const std::uint64_t value,
                                         const unsigned offset) noexcept
    -> std::uint64_t {
  return (value << offset) | (value >> (64u - offset));
}

inline auto keccak_permute(std::array<std::uint64_t, 25> &state) noexcept
    -> void {
  for (std::size_t round = 0; round < 24; ++round) {
    // Theta
    std::array<std::uint64_t, 5> column_parity{};
    for (std::size_t column = 0; column < 5; ++column) {
      column_parity[column] = state[column] ^ state[column + 5] ^
                              state[column + 10] ^ state[column + 15] ^
                              state[column + 20];
    }

    for (std::size_t column = 0; column < 5; ++column) {
      const auto delta{column_parity[(column + 4) % 5] ^
                       keccak_rotate_left(column_parity[(column + 1) % 5], 1)};
      for (std::size_t row = 0; row < 25; row += 5) {
        state[row + column] ^= delta;
      }
    }

    // Rho and pi
    auto current{state[1]};
    for (std::size_t index = 0; index < 24; ++index) {
      const auto lane{keccak_pi_lanes[index]};
      const auto moved{state[lane]};
      state[lane] = keccak_rotate_left(current, keccak_rho_offsets[index]);
      current = moved;
    }

    // Chi
    for (std::size_t row = 0; row < 25; row += 5) {
      std::array<std::uint64_t, 5> plane{};
      for (std::size_t column = 0; column < 5; ++column) {
        plane[column] = state[row + column];
      }

      for (std::size_t column = 0; column < 5; ++column) {
        state[row + column] = plane[column] ^ (~plane[(column + 1) % 5] &
                                               plane[(column + 2) % 5]);
      }
    }

    // Iota
    state[0] ^= keccak_round_constants[round];
  }
}

// Hash a string with SHAKE256, returning the requested number of output bytes
inline auto shake256(const std::string_view input,
                     const std::size_t output_length) -> std::string {
  // The bitrate is 1600 - 2 * 256 = 1088 bits, that is 136 octets
  constexpr std::size_t rate{136};
  std::array<std::uint64_t, 25> state{};

  std::size_t pointer{0};
  for (const auto character : input) {
    state[pointer / 8] ^=
        static_cast<std::uint64_t>(static_cast<std::uint8_t>(character))
        << (8 * (pointer % 8));
    pointer += 1;
    if (pointer == rate) {
      keccak_permute(state);
      pointer = 0;
    }
  }

  // The SHAKE domain separation suffix is the bits 1111, padded to the rate
  // with the pad10*1 rule
  state[pointer / 8] ^= static_cast<std::uint64_t>(0x1f) << (8 * (pointer % 8));
  state[(rate - 1) / 8] ^= static_cast<std::uint64_t>(0x80)
                           << (8 * ((rate - 1) % 8));
  keccak_permute(state);

  std::string output;
  output.reserve(output_length);
  std::size_t squeeze_pointer{0};
  while (output.size() < output_length) {
    output.push_back(static_cast<char>(
        (state[squeeze_pointer / 8] >> (8 * (squeeze_pointer % 8))) & 0xffu));
    squeeze_pointer += 1;
    if (squeeze_pointer == rate) {
      keccak_permute(state);
      squeeze_pointer = 0;
    }
  }

  return output;
}

} // namespace sourcemeta::core

#endif
