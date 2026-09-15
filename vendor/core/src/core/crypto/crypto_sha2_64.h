#ifndef SOURCEMETA_CORE_CRYPTO_SHA2_64_H_
#define SOURCEMETA_CORE_CRYPTO_SHA2_64_H_

// Shared FIPS 180-4 core for the hash functions built on 64-bit words,
// used only by the fallback implementations

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <cstring>     // std::memcpy
#include <string_view> // std::string_view

namespace sourcemeta::core {

// FIPS 180-4 Section 4.1.3 logical functions. Each right rotation is written
// out as a shift pair rather than through a helper, so that an unoptimized
// build does not pay a function call per rotation
inline constexpr auto sha2_64_big_sigma_0(std::uint64_t value) noexcept
    -> std::uint64_t {
  return ((value >> 28u) | (value << 36u)) ^ ((value >> 34u) | (value << 30u)) ^
         ((value >> 39u) | (value << 25u));
}

inline constexpr auto sha2_64_big_sigma_1(std::uint64_t value) noexcept
    -> std::uint64_t {
  return ((value >> 14u) | (value << 50u)) ^ ((value >> 18u) | (value << 46u)) ^
         ((value >> 41u) | (value << 23u));
}

inline constexpr auto sha2_64_small_sigma_0(std::uint64_t value) noexcept
    -> std::uint64_t {
  return ((value >> 1u) | (value << 63u)) ^ ((value >> 8u) | (value << 56u)) ^
         (value >> 7u);
}

inline constexpr auto sha2_64_small_sigma_1(std::uint64_t value) noexcept
    -> std::uint64_t {
  return ((value >> 19u) | (value << 45u)) ^ ((value >> 61u) | (value << 3u)) ^
         (value >> 6u);
}

// Equivalent to (x & y) ^ (~x & z) but avoids a bitwise NOT
inline constexpr auto sha2_64_choice(std::uint64_t x, std::uint64_t y,
                                     std::uint64_t z) noexcept
    -> std::uint64_t {
  return z ^ (x & (y ^ z));
}

inline constexpr auto sha2_64_majority(std::uint64_t x, std::uint64_t y,
                                       std::uint64_t z) noexcept
    -> std::uint64_t {
  return (x & y) ^ (x & z) ^ (y & z);
}

inline auto sha2_64_process_block(const std::uint8_t *block,
                                  std::array<std::uint64_t, 8> &state) noexcept
    -> void {
  // First 64 bits of the fractional parts of the cube roots
  // of the first 80 prime numbers (FIPS 180-4 Section 4.2.3)
  static constexpr std::array<std::uint64_t, 80> round_constants = {
      {0x428a2f98d728ae22U, 0x7137449123ef65cdU, 0xb5c0fbcfec4d3b2fU,
       0xe9b5dba58189dbbcU, 0x3956c25bf348b538U, 0x59f111f1b605d019U,
       0x923f82a4af194f9bU, 0xab1c5ed5da6d8118U, 0xd807aa98a3030242U,
       0x12835b0145706fbeU, 0x243185be4ee4b28cU, 0x550c7dc3d5ffb4e2U,
       0x72be5d74f27b896fU, 0x80deb1fe3b1696b1U, 0x9bdc06a725c71235U,
       0xc19bf174cf692694U, 0xe49b69c19ef14ad2U, 0xefbe4786384f25e3U,
       0x0fc19dc68b8cd5b5U, 0x240ca1cc77ac9c65U, 0x2de92c6f592b0275U,
       0x4a7484aa6ea6e483U, 0x5cb0a9dcbd41fbd4U, 0x76f988da831153b5U,
       0x983e5152ee66dfabU, 0xa831c66d2db43210U, 0xb00327c898fb213fU,
       0xbf597fc7beef0ee4U, 0xc6e00bf33da88fc2U, 0xd5a79147930aa725U,
       0x06ca6351e003826fU, 0x142929670a0e6e70U, 0x27b70a8546d22ffcU,
       0x2e1b21385c26c926U, 0x4d2c6dfc5ac42aedU, 0x53380d139d95b3dfU,
       0x650a73548baf63deU, 0x766a0abb3c77b2a8U, 0x81c2c92e47edaee6U,
       0x92722c851482353bU, 0xa2bfe8a14cf10364U, 0xa81a664bbc423001U,
       0xc24b8b70d0f89791U, 0xc76c51a30654be30U, 0xd192e819d6ef5218U,
       0xd69906245565a910U, 0xf40e35855771202aU, 0x106aa07032bbd1b8U,
       0x19a4c116b8d2d0c8U, 0x1e376c085141ab53U, 0x2748774cdf8eeb99U,
       0x34b0bcb5e19b48a8U, 0x391c0cb3c5c95a63U, 0x4ed8aa4ae3418acbU,
       0x5b9cca4f7763e373U, 0x682e6ff3d6b2b8a3U, 0x748f82ee5defb2fcU,
       0x78a5636f43172f60U, 0x84c87814a1f0ab72U, 0x8cc702081a6439ecU,
       0x90befffa23631e28U, 0xa4506cebde82bde9U, 0xbef9a3f7b2c67915U,
       0xc67178f2e372532bU, 0xca273eceea26619cU, 0xd186b8c721c0c207U,
       0xeada7dd6cde0eb1eU, 0xf57d4f7fee6ed178U, 0x06f067aa72176fbaU,
       0x0a637dc5a2c898a6U, 0x113f9804bef90daeU, 0x1b710b35131c471bU,
       0x28db77f523047d84U, 0x32caab7b40c72493U, 0x3c9ebe0a15c9bebcU,
       0x431d67c49c100d4cU, 0x4cc5d4becb3e42b6U, 0x597f299cfc657e2aU,
       0x5fcb6fab3ad6faecU, 0x6c44198c4a475817U}};

  // Decode 16 big-endian 64-bit words from the block
  std::array<std::uint64_t, 80> schedule;
  auto *schedule_data = schedule.data();
  for (std::uint64_t word_index = 0; word_index < 16u; ++word_index) {
    const std::uint64_t byte_index = word_index * 8u;
    schedule_data[word_index] =
        (static_cast<std::uint64_t>(block[byte_index]) << 56u) |
        (static_cast<std::uint64_t>(block[byte_index + 1u]) << 48u) |
        (static_cast<std::uint64_t>(block[byte_index + 2u]) << 40u) |
        (static_cast<std::uint64_t>(block[byte_index + 3u]) << 32u) |
        (static_cast<std::uint64_t>(block[byte_index + 4u]) << 24u) |
        (static_cast<std::uint64_t>(block[byte_index + 5u]) << 16u) |
        (static_cast<std::uint64_t>(block[byte_index + 6u]) << 8u) |
        static_cast<std::uint64_t>(block[byte_index + 7u]);
  }

  // Extend the message schedule (FIPS 180-4 Section 6.4.2 step 1)
  for (std::uint64_t index = 16u; index < 80u; ++index) {
    schedule_data[index] = sha2_64_small_sigma_1(schedule_data[index - 2u]) +
                           schedule_data[index - 7u] +
                           sha2_64_small_sigma_0(schedule_data[index - 15u]) +
                           schedule_data[index - 16u];
  }

  auto working = state;
  auto *working_data = working.data();
  const auto *constants_data = round_constants.data();

  // Compression function (FIPS 180-4 Section 6.4.2 step 3)
  for (std::uint64_t round_index = 0u; round_index < 80u; ++round_index) {
    const auto temporary_1 =
        working_data[7] + sha2_64_big_sigma_1(working_data[4]) +
        sha2_64_choice(working_data[4], working_data[5], working_data[6]) +
        constants_data[round_index] + schedule_data[round_index];
    const auto temporary_2 =
        sha2_64_big_sigma_0(working_data[0]) +
        sha2_64_majority(working_data[0], working_data[1], working_data[2]);

    working_data[7] = working_data[6];
    working_data[6] = working_data[5];
    working_data[5] = working_data[4];
    working_data[4] = working_data[3] + temporary_1;
    working_data[3] = working_data[2];
    working_data[2] = working_data[1];
    working_data[1] = working_data[0];
    working_data[0] = temporary_1 + temporary_2;
  }

  auto *state_data = state.data();
  for (std::uint64_t index = 0u; index < 8u; ++index) {
    state_data[index] += working_data[index];
  }
}

inline auto sha2_64_hash(const std::string_view input,
                         std::array<std::uint64_t, 8> &state) -> void {
  const auto *const input_bytes =
      reinterpret_cast<const std::uint8_t *>(input.data());
  const std::size_t input_length = input.size();

  // Process all full 128-byte blocks directly from the input (streaming)
  std::size_t processed_bytes = 0u;
  while (input_length - processed_bytes >= 128u) {
    sha2_64_process_block(input_bytes + processed_bytes, state);
    processed_bytes += 128u;
  }

  // Prepare the final block(s) (one or two 128-byte blocks)
  std::array<std::uint8_t, 256> final_block{};
  const std::size_t remaining_bytes = input_length - processed_bytes;
  if (remaining_bytes > 0u) {
    std::memcpy(final_block.data(), input_bytes + processed_bytes,
                remaining_bytes);
  }

  // Append the 0x80 byte after the message data
  final_block[remaining_bytes] = 0x80u;

  // Append length in bits as a big-endian 128-bit value at the end of the
  // padding (FIPS 180-4 Section 5.1.2). The bit length of any input is at
  // most 67 bits wide, so the upper word carries the bits that the 8x
  // multiplication would otherwise overflow
  const std::uint64_t message_length_bits_high =
      static_cast<std::uint64_t>(input_length) >> 61u;
  const std::uint64_t message_length_bits_low =
      static_cast<std::uint64_t>(input_length) << 3u;

  if (remaining_bytes < 112u) {
    for (std::uint64_t index = 0u; index < 8u; ++index) {
      final_block[112u + index] = static_cast<std::uint8_t>(
          (message_length_bits_high >> (8u * (7u - index))) & 0xffu);
      final_block[120u + index] = static_cast<std::uint8_t>(
          (message_length_bits_low >> (8u * (7u - index))) & 0xffu);
    }
    sha2_64_process_block(final_block.data(), state);
  } else {
    for (std::uint64_t index = 0u; index < 8u; ++index) {
      final_block[128u + 112u + index] = static_cast<std::uint8_t>(
          (message_length_bits_high >> (8u * (7u - index))) & 0xffu);
      final_block[128u + 120u + index] = static_cast<std::uint8_t>(
          (message_length_bits_low >> (8u * (7u - index))) & 0xffu);
    }

    sha2_64_process_block(final_block.data(), state);
    sha2_64_process_block(final_block.data() + 128u, state);
  }
}

} // namespace sourcemeta::core

#endif
