#include "crypto_aes.h"
#include "crypto_aes_block.h"

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

// A from-scratch AES in Cipher Block Chaining mode (NIST SP 800-38A) for the
// reference backend, over the shared AES block cipher. This is not
// constant-time, which is acceptable only because this backend is the
// non-production fallback

namespace sourcemeta::core {
namespace {
constexpr std::size_t BLOCK_BYTES{16};

auto load_block(const std::string_view data, const std::size_t offset)
    -> AesBlock {
  AesBlock block{};
  for (std::size_t index = 0; index < BLOCK_BYTES; ++index) {
    block[index] = static_cast<std::uint8_t>(data[offset + index]);
  }

  return block;
}
} // namespace

auto aes_cbc_encrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view plaintext)
    -> std::optional<std::string> {
  assert(plaintext.size() % BLOCK_BYTES == 0);
  const auto schedule{aes_expand_key(key)};
  std::string ciphertext(plaintext.size(), '\x00');
  auto chain{load_block(iv, 0)};
  for (std::size_t offset = 0; offset < plaintext.size();
       offset += BLOCK_BYTES) {
    auto block{load_block(plaintext, offset)};
    for (std::size_t index = 0; index < BLOCK_BYTES; ++index) {
      block[index] ^= chain[index];
    }

    chain = aes_encrypt_block(schedule, block);
    for (std::size_t index = 0; index < BLOCK_BYTES; ++index) {
      ciphertext[offset + index] = static_cast<char>(chain[index]);
    }
  }

  return ciphertext;
}

auto aes_cbc_decrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view ciphertext)
    -> std::optional<std::string> {
  assert(ciphertext.size() % BLOCK_BYTES == 0);
  const auto schedule{aes_expand_key(key)};
  std::string plaintext(ciphertext.size(), '\x00');
  auto chain{load_block(iv, 0)};
  for (std::size_t offset = 0; offset < ciphertext.size();
       offset += BLOCK_BYTES) {
    const auto block{load_block(ciphertext, offset)};
    const auto decrypted{aes_decrypt_block(schedule, block)};
    for (std::size_t index = 0; index < BLOCK_BYTES; ++index) {
      plaintext[offset + index] =
          static_cast<char>(decrypted[index] ^ chain[index]);
    }

    chain = block;
  }

  return plaintext;
}

} // namespace sourcemeta::core
