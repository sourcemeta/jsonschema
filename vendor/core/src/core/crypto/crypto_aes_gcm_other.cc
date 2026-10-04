#include "crypto_aes.h"
#include "crypto_aes_block.h"
#include "crypto_ghash.h"

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

// A from-scratch AES in Galois/Counter Mode (NIST SP 800-38D) for the reference
// backend, over the shared AES block cipher. This is not constant-time, which
// is acceptable only because this backend is the non-production fallback

namespace sourcemeta::core {
namespace {

struct Context {
  AesKeySchedule schedule;
  AesBlock hash_key;
  AesBlock counter_zero;
  AesBlock tag_mask;
};

auto make_context(const std::string_view key, const std::string_view iv)
    -> Context {
  assert(iv.size() == 12);
  Context context{.schedule = aes_expand_key(key),
                  .hash_key = {},
                  .counter_zero = {},
                  .tag_mask = {}};
  context.hash_key = aes_encrypt_block(context.schedule, AesBlock{});
  for (std::size_t index = 0; index < 12; ++index) {
    context.counter_zero[index] = static_cast<std::uint8_t>(iv[index]);
  }

  // The 96-bit initialization vector yields the pre-counter block iv ||
  // 0x00000001
  context.counter_zero[15] = 1;
  context.tag_mask = aes_encrypt_block(context.schedule, context.counter_zero);
  return context;
}

auto increment_counter(AesBlock &counter) -> void {
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
  AesBlock counter{context.counter_zero};
  for (std::size_t offset = 0; offset < input.size(); offset += 16) {
    increment_counter(counter);
    const auto keystream{aes_encrypt_block(context.schedule, counter)};
    for (std::size_t index = 0; index < 16 && offset + index < input.size();
         ++index) {
      output[offset + index] = static_cast<char>(
          static_cast<std::uint8_t>(input[offset + index]) ^ keystream[index]);
    }
  }

  return output;
}

// The authentication tag over the associated data and the ciphertext (NIST SP
// 800-38D Section 7.1)
auto compute_tag(const Context &context, const std::string_view associated_data,
                 const std::string_view ciphertext) -> AesBlock {
  auto accumulator{ghash(context.hash_key, associated_data, AesBlock{})};
  accumulator = ghash(context.hash_key, ciphertext, accumulator);

  // The final GHASH block is the bit lengths of the associated data and the
  // ciphertext, each as a 64-bit big-endian integer
  AesBlock lengths{};
  const auto associated_bits{
      static_cast<std::uint64_t>(associated_data.size()) * 8};
  const auto ciphertext_bits{static_cast<std::uint64_t>(ciphertext.size()) * 8};
  for (std::size_t index = 0; index < 8; ++index) {
    lengths[7 - index] =
        static_cast<std::uint8_t>((associated_bits >> (8 * index)) & 0xffu);
    lengths[15 - index] =
        static_cast<std::uint8_t>((ciphertext_bits >> (8 * index)) & 0xffu);
  }

  for (std::size_t index = 0; index < 16; ++index) {
    accumulator[index] ^= lengths[index];
  }

  accumulator = gf_multiply(accumulator, context.hash_key);
  AesBlock tag{};
  for (std::size_t index = 0; index < 16; ++index) {
    tag[index] = accumulator[index] ^ context.tag_mask[index];
  }

  return tag;
}

} // namespace

auto aes_gcm_seal(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext> {
  const auto context{make_context(key, iv)};
  // The tag is computed over the ciphertext, then appended to it
  AESGCMCiphertext result{.data = counter_mode(context, plaintext)};
  const auto tag{compute_tag(context, associated_data, result.data)};
  result.data.append(reinterpret_cast<const char *>(tag.data()), tag.size());
  return result;
}

auto aes_gcm_open(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view ciphertext, const std::string_view tag)
    -> std::optional<std::string> {
  const auto context{make_context(key, iv)};
  const auto expected{compute_tag(context, associated_data, ciphertext)};

  std::uint8_t difference{0};
  for (std::size_t index = 0; index < 16; ++index) {
    difference = static_cast<std::uint8_t>(
        difference | (expected[index] ^ static_cast<std::uint8_t>(tag[index])));
  }

  if (difference != 0) {
    return std::nullopt;
  }

  return counter_mode(context, ciphertext);
}

} // namespace sourcemeta::core
