#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/crypto_aes_cbc_hmac.h>

#include "crypto_aes.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

namespace {
// A 128-bit initialization vector and the AES block size (NIST SP 800-38A)
constexpr std::size_t IV_BYTES{16};
constexpr std::size_t BLOCK_BYTES{16};
// An upper bound the backends can all process without narrowing their lengths
constexpr std::size_t MAX_INPUT_BYTES{
    static_cast<std::size_t>(std::numeric_limits<int>::max())};

// A 256, 384, or 512-bit key selects A128CBC-HS256, A192CBC-HS384, or
// A256CBC-HS512, split into equal halves for the MAC and the cipher (RFC 7518
// Section 5.2.2.1)
auto is_valid_key_size(const std::size_t size) -> bool {
  return size == 32 || size == 48 || size == 64;
}

// The associated data length in bits as a 64-bit big-endian integer, the final
// input to the MAC (RFC 7518 Section 5.2.2.1)
auto associated_data_length_block(const std::size_t associated_data_size)
    -> std::string {
  std::string block(8, '\x00');
  const auto bits{static_cast<std::uint64_t>(associated_data_size) * 8};
  for (std::size_t index = 0; index < 8; ++index) {
    block[7 - index] = static_cast<char>((bits >> (8 * index)) & 0xffu);
  }

  return block;
}

// The HMAC over the associated data, the initialization vector, the ciphertext,
// and the length block, truncated to the tag length that the variant defines
auto authentication_tag(const std::size_t tag_length,
                        const std::string_view mac_key,
                        const std::string_view iv,
                        const std::string_view associated_data,
                        const std::string_view ciphertext) -> std::string {
  const auto length_block{associated_data_length_block(associated_data.size())};
  // The MAC input is the associated data, the initialization vector, the
  // ciphertext, and the length block concatenated (RFC 7518 Section 5.2.2.1)
  std::string message;
  message.reserve(associated_data.size() + iv.size() + ciphertext.size() +
                  length_block.size());
  message.append(associated_data);
  message.append(iv);
  message.append(ciphertext);
  message.append(length_block);
  switch (tag_length) {
    case 16: {
      const auto digest{hmac_sha256_digest(mac_key, message)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         tag_length};
    }
    case 24: {
      const auto digest{hmac_sha384_digest(mac_key, message)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         tag_length};
    }
    case 32: {
      const auto digest{hmac_sha512_digest(mac_key, message)};
      return std::string{reinterpret_cast<const char *>(digest.data()),
                         tag_length};
    }
    default:
      std::unreachable();
  }
}
} // namespace

auto aes_cbc_hmac_encrypt(const std::string_view key, const std::string_view iv,
                          const std::string_view associated_data,
                          const std::string_view plaintext)
    -> std::optional<AESCBCHMACCiphertext> {
  // The plaintext bound leaves a whole block of headroom, since the padding
  // grows the buffer by up to that much and the backends narrow the padded
  // length to an int
  if (!is_valid_key_size(key.size()) || iv.size() != IV_BYTES ||
      plaintext.size() > MAX_INPUT_BYTES - BLOCK_BYTES ||
      associated_data.size() > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  const auto half{key.size() / 2};
  const auto mac_key{key.substr(0, half)};
  const auto encryption_key{key.substr(half)};

  // PKCS#7 padding always adds between one and a whole block of bytes, each
  // equal to the number added (RFC 5652 Section 6.3)
  const auto padding{BLOCK_BYTES - (plaintext.size() % BLOCK_BYTES)};
  std::string padded{plaintext};
  padded.append(padding, static_cast<char>(padding));

  auto ciphertext{aes_cbc_encrypt(encryption_key, iv, padded)};
  if (!ciphertext.has_value()) {
    return std::nullopt;
  }

  const auto tag{authentication_tag(half, mac_key, iv, associated_data,
                                    ciphertext.value())};
  AESCBCHMACCiphertext result{.data = std::move(ciphertext).value(),
                              .tag_length = half};
  result.data.append(tag);
  return result;
}

auto aes_cbc_hmac_decrypt(const std::string_view key, const std::string_view iv,
                          const std::string_view associated_data,
                          const std::string_view ciphertext,
                          const std::string_view tag)
    -> std::optional<std::string> {
  const auto half{key.size() / 2};
  if (!is_valid_key_size(key.size()) || iv.size() != IV_BYTES ||
      tag.size() != half || ciphertext.empty() ||
      ciphertext.size() % BLOCK_BYTES != 0 ||
      ciphertext.size() > MAX_INPUT_BYTES ||
      associated_data.size() > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  const auto mac_key{key.substr(0, half)};
  const auto encryption_key{key.substr(half)};

  // Verify the tag before, and independent of, decrypting so a padding oracle
  // is never reachable (encrypt-then-MAC)
  const auto expected{
      authentication_tag(half, mac_key, iv, associated_data, ciphertext)};
  if (!secure_equals(expected, tag)) {
    return std::nullopt;
  }

  auto padded{aes_cbc_decrypt(encryption_key, iv, ciphertext)};
  if (!padded.has_value()) {
    return std::nullopt;
  }

  // Strip and validate the PKCS#7 padding, safe to do in variable time here
  // since the authentication already succeeded
  const auto &plaintext{padded.value()};
  const auto padding{
      static_cast<std::size_t>(static_cast<unsigned char>(plaintext.back()))};
  if (padding == 0 || padding > BLOCK_BYTES || padding > plaintext.size()) {
    return std::nullopt;
  }

  for (std::size_t index = plaintext.size() - padding; index < plaintext.size();
       ++index) {
    if (static_cast<unsigned char>(plaintext[index]) != padding) {
      return std::nullopt;
    }
  }

  return plaintext.substr(0, plaintext.size() - padding);
}

} // namespace sourcemeta::core
