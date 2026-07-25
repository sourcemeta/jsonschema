#ifndef SOURCEMETA_CORE_CRYPTO_AES_BLOCK_H_
#define SOURCEMETA_CORE_CRYPTO_AES_BLOCK_H_

// The AES block cipher (FIPS 197) for the reference backend, shared by the
// modes of operation built on top of it. This is not constant-time, which is
// acceptable only because this backend is the non-production fallback. Both the
// forward and the inverse cipher over 128, 192, and 256-bit keys are provided,
// sharing the single key schedule

#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

// The Rijndael substitution box (FIPS 197 Figure 7)
inline constexpr std::array<std::uint8_t, 256> aes_substitution{
    {0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
     0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
     0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
     0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
     0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
     0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
     0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
     0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
     0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
     0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
     0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
     0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
     0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
     0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
     0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
     0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
     0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
     0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
     0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
     0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
     0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
     0xb0, 0x54, 0xbb, 0x16}};

// The inverse of the Rijndael substitution box (FIPS 197 Figure 14)
inline constexpr std::array<std::uint8_t, 256> aes_inverse_substitution{
    {0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
     0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
     0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
     0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
     0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
     0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
     0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50,
     0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
     0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
     0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
     0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41,
     0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
     0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
     0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
     0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
     0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
     0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
     0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
     0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
     0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
     0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
     0x55, 0x21, 0x0c, 0x7d}};

using AesBlock = std::array<std::uint8_t, 16>;
// The expanded key material fits the largest schedule, the 256-bit key over 14
// rounds (FIPS 197 Section 5.2)
using AesRoundKeys = std::array<std::uint8_t, 240>;

// A key schedule together with its round count, since AES-128, AES-192, and
// AES-256 run 10, 12, and 14 rounds respectively (FIPS 197 Section 5.1)
struct AesKeySchedule {
  AesRoundKeys round_keys;
  std::size_t rounds;
};

// Multiply by two in GF(2^8) with the AES reduction polynomial (FIPS 197
// Section 4.2)
inline auto aes_xtime(const std::uint8_t value) -> std::uint8_t {
  return static_cast<std::uint8_t>((static_cast<unsigned>(value) << 1u) ^
                                   (((value & 0x80u) != 0) ? 0x1bu : 0x00u));
}

inline auto aes_field_multiply(const std::uint8_t left,
                               const std::uint8_t right) -> std::uint8_t {
  std::uint8_t product{0};
  std::uint8_t factor{left};
  for (std::uint8_t bit{right}; bit != 0; bit >>= 1u) {
    if ((bit & 1u) != 0) {
      product ^= factor;
    }

    factor = aes_xtime(factor);
  }

  return product;
}

// AES key expansion (FIPS 197 Section 5.2) over a 128, 192, or 256-bit key
inline auto aes_expand_key(const std::string_view key) -> AesKeySchedule {
  assert(key.size() == 16 || key.size() == 24 || key.size() == 32);
  // The number of 32-bit words in the key, four, six, or eight (FIPS 197 Table
  // 4), and the round count it yields
  const auto key_words{key.size() / 4};
  const auto rounds{key_words + 6};
  AesKeySchedule schedule{.round_keys = {}, .rounds = rounds};
  auto &round_keys{schedule.round_keys};
  for (std::size_t index = 0; index < key.size(); ++index) {
    round_keys[index] = static_cast<std::uint8_t>(key[index]);
  }

  const auto expanded_bytes{16 * (rounds + 1)};
  std::uint8_t round_constant{0x01};
  for (std::size_t index = key.size(); index < expanded_bytes; index += 4) {
    std::array<std::uint8_t, 4> word{
        {round_keys[index - 4], round_keys[index - 3], round_keys[index - 2],
         round_keys[index - 1]}};
    const auto position{index / 4};
    if (position % key_words == 0) {
      const auto first{word[0]};
      word[0] = aes_substitution[word[1]] ^ round_constant;
      word[1] = aes_substitution[word[2]];
      word[2] = aes_substitution[word[3]];
      word[3] = aes_substitution[first];
      round_constant = aes_xtime(round_constant);
    } else if (key_words > 6 && position % key_words == 4) {
      // The extra substitution mid-schedule applies only to the 256-bit key
      for (auto &byte : word) {
        byte = aes_substitution[byte];
      }
    }

    for (std::size_t offset = 0; offset < 4; ++offset) {
      round_keys[index + offset] =
          round_keys[index - key.size() + offset] ^ word[offset];
    }
  }

  return schedule;
}

inline auto aes_encrypt_block(const AesKeySchedule &schedule, AesBlock state)
    -> AesBlock {
  const auto &round_keys{schedule.round_keys};
  const auto add_round_key{[&round_keys, &state](const std::size_t round) {
    for (std::size_t index = 0; index < 16; ++index) {
      state[index] ^= round_keys[(round * 16) + index];
    }
  }};

  add_round_key(0);
  for (std::size_t round = 1; round <= schedule.rounds; ++round) {
    for (auto &byte : state) {
      byte = aes_substitution[byte];
    }

    // ShiftRows over the column-major state (FIPS 197 Section 5.1.2)
    const AesBlock shifted{{state[0], state[5], state[10], state[15], state[4],
                            state[9], state[14], state[3], state[8], state[13],
                            state[2], state[7], state[12], state[1], state[6],
                            state[11]}};
    state = shifted;

    if (round != schedule.rounds) {
      // MixColumns (FIPS 197 Section 5.1.3)
      for (std::size_t column = 0; column < 4; ++column) {
        const auto base{column * 4};
        const auto first{state[base]};
        const auto second{state[base + 1]};
        const auto third{state[base + 2]};
        const auto fourth{state[base + 3]};
        state[base] = aes_field_multiply(first, 2) ^
                      aes_field_multiply(second, 3) ^ third ^ fourth;
        state[base + 1] = first ^ aes_field_multiply(second, 2) ^
                          aes_field_multiply(third, 3) ^ fourth;
        state[base + 2] = first ^ second ^ aes_field_multiply(third, 2) ^
                          aes_field_multiply(fourth, 3);
        state[base + 3] = aes_field_multiply(first, 3) ^ second ^ third ^
                          aes_field_multiply(fourth, 2);
      }
    }

    add_round_key(round);
  }

  return state;
}

inline auto aes_decrypt_block(const AesKeySchedule &schedule, AesBlock state)
    -> AesBlock {
  const auto &round_keys{schedule.round_keys};
  const auto add_round_key{[&round_keys, &state](const std::size_t round) {
    for (std::size_t index = 0; index < 16; ++index) {
      state[index] ^= round_keys[(round * 16) + index];
    }
  }};

  add_round_key(schedule.rounds);
  for (std::size_t round = schedule.rounds; round-- > 0;) {
    // InvShiftRows over the column-major state (FIPS 197 Section 5.3.1)
    const AesBlock shifted{{state[0], state[13], state[10], state[7], state[4],
                            state[1], state[14], state[11], state[8], state[5],
                            state[2], state[15], state[12], state[9], state[6],
                            state[3]}};
    state = shifted;

    for (auto &byte : state) {
      byte = aes_inverse_substitution[byte];
    }

    add_round_key(round);

    if (round != 0) {
      // InvMixColumns (FIPS 197 Section 5.3.3)
      for (std::size_t column = 0; column < 4; ++column) {
        const auto base{column * 4};
        const auto first{state[base]};
        const auto second{state[base + 1]};
        const auto third{state[base + 2]};
        const auto fourth{state[base + 3]};
        state[base] =
            aes_field_multiply(first, 0x0e) ^ aes_field_multiply(second, 0x0b) ^
            aes_field_multiply(third, 0x0d) ^ aes_field_multiply(fourth, 0x09);
        state[base + 1] =
            aes_field_multiply(first, 0x09) ^ aes_field_multiply(second, 0x0e) ^
            aes_field_multiply(third, 0x0b) ^ aes_field_multiply(fourth, 0x0d);
        state[base + 2] =
            aes_field_multiply(first, 0x0d) ^ aes_field_multiply(second, 0x09) ^
            aes_field_multiply(third, 0x0e) ^ aes_field_multiply(fourth, 0x0b);
        state[base + 3] =
            aes_field_multiply(first, 0x0b) ^ aes_field_multiply(second, 0x0d) ^
            aes_field_multiply(third, 0x09) ^ aes_field_multiply(fourth, 0x0e);
      }
    }
  }

  return state;
}

} // namespace sourcemeta::core

#endif
