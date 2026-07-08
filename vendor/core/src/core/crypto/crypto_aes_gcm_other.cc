#include "crypto_aes.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

// A from-scratch AES-256 in Galois/Counter Mode (FIPS 197 and NIST SP 800-38D)
// for the reference backend. This is not constant-time, which is acceptable
// only because this backend is the non-production fallback

namespace sourcemeta::core {
namespace {

// The Rijndael substitution box (FIPS 197 Figure 7)
constexpr std::array<std::uint8_t, 256> SUBSTITUTION{
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

using Block = std::array<std::uint8_t, 16>;
using RoundKeys = std::array<std::uint8_t, 240>;

// Multiply by two in GF(2^8) with the AES reduction polynomial (FIPS 197
// Section 4.2)
auto xtime(const std::uint8_t value) -> std::uint8_t {
  return static_cast<std::uint8_t>((static_cast<unsigned>(value) << 1u) ^
                                   (((value & 0x80u) != 0) ? 0x1bu : 0x00u));
}

auto multiply(const std::uint8_t left, const std::uint8_t right)
    -> std::uint8_t {
  std::uint8_t product{0};
  std::uint8_t factor{left};
  for (std::uint8_t bit{right}; bit != 0; bit >>= 1u) {
    if ((bit & 1u) != 0) {
      product ^= factor;
    }

    factor = xtime(factor);
  }

  return product;
}

// AES-256 key expansion (FIPS 197 Section 5.2, with the 256-bit key schedule)
auto expand_key(const std::string_view key) -> RoundKeys {
  RoundKeys round_keys{};
  for (std::size_t index = 0; index < 32; ++index) {
    round_keys[index] = static_cast<std::uint8_t>(key[index]);
  }

  std::uint8_t round_constant{0x01};
  for (std::size_t index = 32; index < round_keys.size(); index += 4) {
    std::array<std::uint8_t, 4> word{
        {round_keys[index - 4], round_keys[index - 3], round_keys[index - 2],
         round_keys[index - 1]}};
    const auto position{index / 4};
    if (position % 8 == 0) {
      const auto first{word[0]};
      word[0] = SUBSTITUTION[word[1]] ^ round_constant;
      word[1] = SUBSTITUTION[word[2]];
      word[2] = SUBSTITUTION[word[3]];
      word[3] = SUBSTITUTION[first];
      round_constant = xtime(round_constant);
    } else if (position % 8 == 4) {
      for (auto &byte : word) {
        byte = SUBSTITUTION[byte];
      }
    }

    for (std::size_t offset = 0; offset < 4; ++offset) {
      round_keys[index + offset] =
          round_keys[index - 32 + offset] ^ word[offset];
    }
  }

  return round_keys;
}

auto encrypt_block(const RoundKeys &round_keys, Block state) -> Block {
  const auto add_round_key{[&round_keys, &state](const std::size_t round) {
    for (std::size_t index = 0; index < 16; ++index) {
      state[index] ^= round_keys[(round * 16) + index];
    }
  }};

  add_round_key(0);
  for (std::size_t round = 1; round <= 14; ++round) {
    for (auto &byte : state) {
      byte = SUBSTITUTION[byte];
    }

    // ShiftRows over the column-major state (FIPS 197 Section 5.1.2)
    const Block shifted{{state[0], state[5], state[10], state[15], state[4],
                         state[9], state[14], state[3], state[8], state[13],
                         state[2], state[7], state[12], state[1], state[6],
                         state[11]}};
    state = shifted;

    if (round != 14) {
      // MixColumns (FIPS 197 Section 5.1.3)
      for (std::size_t column = 0; column < 4; ++column) {
        const auto base{column * 4};
        const auto first{state[base]};
        const auto second{state[base + 1]};
        const auto third{state[base + 2]};
        const auto fourth{state[base + 3]};
        state[base] = multiply(first, 2) ^ multiply(second, 3) ^ third ^ fourth;
        state[base + 1] =
            first ^ multiply(second, 2) ^ multiply(third, 3) ^ fourth;
        state[base + 2] =
            first ^ second ^ multiply(third, 2) ^ multiply(fourth, 3);
        state[base + 3] =
            multiply(first, 3) ^ second ^ third ^ multiply(fourth, 2);
      }
    }

    add_round_key(round);
  }

  return state;
}

// Multiply two blocks in GF(2^128) with the GCM reduction polynomial (NIST SP
// 800-38D Section 6.3)
auto gf_multiply(const Block &left, const Block &right) -> Block {
  Block product{};
  Block value{left};
  for (std::size_t bit = 0; bit < 128; ++bit) {
    if (((right[bit / 8] >> (7 - (bit % 8))) & 1u) != 0) {
      for (std::size_t index = 0; index < 16; ++index) {
        product[index] ^= value[index];
      }
    }

    const auto carry_out{static_cast<std::uint8_t>(value[15] & 1u)};
    std::uint8_t carry_in{0};
    for (auto &byte : value) {
      const auto next_carry{static_cast<std::uint8_t>(byte & 1u)};
      byte = static_cast<std::uint8_t>((byte >> 1u) | (carry_in << 7u));
      carry_in = next_carry;
    }

    if (carry_out != 0) {
      value[0] ^= 0xe1u;
    }
  }

  return product;
}

// GHASH the data padded to whole blocks (NIST SP 800-38D Section 6.4)
auto ghash(const Block &key, const std::string_view data, Block accumulator)
    -> Block {
  for (std::size_t offset = 0; offset < data.size(); offset += 16) {
    for (std::size_t index = 0; index < 16 && offset + index < data.size();
         ++index) {
      accumulator[index] ^= static_cast<std::uint8_t>(data[offset + index]);
    }

    accumulator = gf_multiply(accumulator, key);
  }

  return accumulator;
}

struct Context {
  RoundKeys round_keys;
  Block hash_key;
  Block counter_zero;
  Block tag_mask;
};

auto make_context(const std::string_view key, const std::string_view nonce)
    -> Context {
  Context context{.round_keys = expand_key(key),
                  .hash_key = {},
                  .counter_zero = {},
                  .tag_mask = {}};
  context.hash_key = encrypt_block(context.round_keys, Block{});
  for (std::size_t index = 0; index < 12; ++index) {
    context.counter_zero[index] = static_cast<std::uint8_t>(nonce[index]);
  }

  // The 96-bit nonce yields the pre-counter block nonce || 0x00000001
  context.counter_zero[15] = 1;
  context.tag_mask = encrypt_block(context.round_keys, context.counter_zero);
  return context;
}

auto increment_counter(Block &counter) -> void {
  for (std::size_t index = 16; index-- > 12;) {
    if (++counter[index] != 0) {
      break;
    }
  }
}

// Counter-mode keystream applied to the input, symmetric for both directions
auto counter_mode(const Context &context, const std::string_view input)
    -> std::string {
  std::string output(input.size(), '\x00');
  Block counter{context.counter_zero};
  for (std::size_t offset = 0; offset < input.size(); offset += 16) {
    increment_counter(counter);
    const auto keystream{encrypt_block(context.round_keys, counter)};
    for (std::size_t index = 0; index < 16 && offset + index < input.size();
         ++index) {
      output[offset + index] = static_cast<char>(
          static_cast<std::uint8_t>(input[offset + index]) ^ keystream[index]);
    }
  }

  return output;
}

// The authentication tag over an empty associated data and the ciphertext
auto compute_tag(const Context &context, const std::string_view ciphertext)
    -> Block {
  auto accumulator{ghash(context.hash_key, ciphertext, Block{})};
  Block lengths{};
  const auto ciphertext_bits{static_cast<std::uint64_t>(ciphertext.size()) * 8};
  for (std::size_t index = 0; index < 8; ++index) {
    lengths[15 - index] =
        static_cast<std::uint8_t>((ciphertext_bits >> (8 * index)) & 0xffu);
  }

  for (std::size_t index = 0; index < 16; ++index) {
    accumulator[index] ^= lengths[index];
  }

  accumulator = gf_multiply(accumulator, context.hash_key);
  Block tag{};
  for (std::size_t index = 0; index < 16; ++index) {
    tag[index] = accumulator[index] ^ context.tag_mask[index];
  }

  return tag;
}

} // namespace

auto aes_256_gcm_encrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view plaintext)
    -> std::optional<std::string> {
  const auto context{make_context(key, nonce)};
  std::string ciphertext{counter_mode(context, plaintext)};
  const auto tag{compute_tag(context, ciphertext)};
  ciphertext.append(reinterpret_cast<const char *>(tag.data()), tag.size());
  return ciphertext;
}

auto aes_256_gcm_decrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view ciphertext)
    -> std::optional<std::string> {
  if (ciphertext.size() < 16) {
    return std::nullopt;
  }

  const auto message{ciphertext.substr(0, ciphertext.size() - 16)};
  const auto received_tag{ciphertext.substr(ciphertext.size() - 16)};
  const auto context{make_context(key, nonce)};
  const auto tag{compute_tag(context, message)};

  std::uint8_t difference{0};
  for (std::size_t index = 0; index < 16; ++index) {
    difference = static_cast<std::uint8_t>(
        difference |
        (tag[index] ^ static_cast<std::uint8_t>(received_tag[index])));
  }

  if (difference != 0) {
    return std::nullopt;
  }

  return counter_mode(context, message);
}

} // namespace sourcemeta::core
