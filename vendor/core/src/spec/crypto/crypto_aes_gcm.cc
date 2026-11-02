#include <sourcemeta/core/crypto_aes_gcm.h>

#include "crypto_aes.h"
#include "crypto_random.h"

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <exception>   // std::exception
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {
// A 96-bit initialization vector and a 128-bit tag (NIST SP 800-38D)
constexpr std::size_t IV_BYTES{12};
constexpr std::size_t TAG_BYTES{16};
// A 256-bit key, the only size the self-contained seal accepts
constexpr std::size_t SEAL_KEY_BYTES{32};
// An upper bound the backends can all process without narrowing their lengths
constexpr std::size_t MAX_INPUT_BYTES{
    static_cast<std::size_t>(std::numeric_limits<int>::max())};

auto is_valid_key_size(const std::size_t size) -> bool {
  return size == 16 || size == 24 || size == 32;
}
} // namespace

auto aes_gcm_encrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view associated_data,
                     const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext> {
  if (!is_valid_key_size(key.size()) || iv.size() != IV_BYTES ||
      plaintext.size() > MAX_INPUT_BYTES ||
      associated_data.size() > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  return aes_gcm_seal(key, iv, associated_data, plaintext);
}

auto aes_gcm_decrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view associated_data,
                     const std::string_view ciphertext,
                     const std::string_view tag) -> std::optional<std::string> {
  if (!is_valid_key_size(key.size()) || iv.size() != IV_BYTES ||
      tag.size() != TAG_BYTES || ciphertext.size() > MAX_INPUT_BYTES ||
      associated_data.size() > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  return aes_gcm_open(key, iv, associated_data, ciphertext, tag);
}

auto aes_256_gcm_seal(const std::string_view key,
                      const std::string_view plaintext)
    -> std::optional<std::string> {
  if (key.size() != SEAL_KEY_BYTES) {
    return std::nullopt;
  }

  std::string nonce(IV_BYTES, '\x00');
  try {
    fill_random_bytes(std::span<std::uint8_t>{
        reinterpret_cast<std::uint8_t *>(nonce.data()), IV_BYTES});
  } catch (const std::exception &) {
    return std::nullopt;
  }

  const auto result{aes_gcm_encrypt(key, nonce, "", plaintext)};
  if (!result.has_value()) {
    return std::nullopt;
  }

  // The sealed message is the nonce followed by the ciphertext and its tag
  std::string sealed{std::move(nonce)};
  sealed.append(result.value().data);
  return sealed;
}

auto aes_256_gcm_unseal(const std::string_view key,
                        const std::string_view sealed)
    -> std::optional<std::string> {
  if (key.size() != SEAL_KEY_BYTES || sealed.size() < IV_BYTES + TAG_BYTES) {
    return std::nullopt;
  }

  const auto nonce{sealed.substr(0, IV_BYTES)};
  const auto ciphertext{
      sealed.substr(IV_BYTES, sealed.size() - IV_BYTES - TAG_BYTES)};
  const auto tag{sealed.substr(sealed.size() - TAG_BYTES)};
  return aes_gcm_decrypt(key, nonce, "", ciphertext, tag);
}

} // namespace sourcemeta::core
