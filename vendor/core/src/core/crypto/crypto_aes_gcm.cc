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
// A 256-bit key, a 96-bit nonce, and a 128-bit tag (NIST SP 800-38D)
constexpr std::size_t KEY_BYTES{32};
constexpr std::size_t NONCE_BYTES{12};
constexpr std::size_t TAG_BYTES{16};
// An upper bound the backends can all process without narrowing their lengths
constexpr std::size_t MAX_INPUT_BYTES{
    static_cast<std::size_t>(std::numeric_limits<int>::max())};
} // namespace

auto aes_256_gcm_seal(const std::string_view key,
                      const std::string_view plaintext)
    -> std::optional<std::string> {
  if (key.size() != KEY_BYTES || plaintext.size() > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  std::string nonce(NONCE_BYTES, '\x00');
  try {
    fill_random_bytes(std::span<std::uint8_t>{
        reinterpret_cast<std::uint8_t *>(nonce.data()), NONCE_BYTES});
  } catch (const std::exception &) {
    return std::nullopt;
  }

  const auto ciphertext{aes_256_gcm_encrypt(key, nonce, plaintext)};
  if (!ciphertext.has_value()) {
    return std::nullopt;
  }

  // The sealed message is the nonce followed by the ciphertext and its tag
  std::string sealed{std::move(nonce)};
  sealed.append(ciphertext.value());
  return sealed;
}

auto aes_256_gcm_unseal(const std::string_view key,
                        const std::string_view sealed)
    -> std::optional<std::string> {
  if (key.size() != KEY_BYTES || sealed.size() < NONCE_BYTES + TAG_BYTES ||
      sealed.size() - NONCE_BYTES - TAG_BYTES > MAX_INPUT_BYTES) {
    return std::nullopt;
  }

  const auto nonce{sealed.substr(0, NONCE_BYTES)};
  const auto ciphertext{sealed.substr(NONCE_BYTES)};
  return aes_256_gcm_decrypt(key, nonce, ciphertext);
}

} // namespace sourcemeta::core
