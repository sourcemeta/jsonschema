#include "crypto_aes.h"

#include "crypto_aes_apple.h"

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
constexpr std::size_t TAG_BYTES{16};
} // namespace

namespace sourcemeta::core {

auto aes_256_gcm_encrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view plaintext)
    -> std::optional<std::string> {
  std::string output(plaintext.size() + TAG_BYTES, '\x00');
  if (!sourcemeta_core_aes_256_gcm_seal_cryptokit(
          reinterpret_cast<const unsigned char *>(key.data()), key.size(),
          reinterpret_cast<const unsigned char *>(nonce.data()), nonce.size(),
          reinterpret_cast<const unsigned char *>(plaintext.data()),
          plaintext.size(), reinterpret_cast<unsigned char *>(output.data()))) {
    return std::nullopt;
  }

  return output;
}

auto aes_256_gcm_decrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view ciphertext)
    -> std::optional<std::string> {
  if (ciphertext.size() < TAG_BYTES) {
    return std::nullopt;
  }

  const auto message{ciphertext.substr(0, ciphertext.size() - TAG_BYTES)};
  const auto tag{ciphertext.substr(ciphertext.size() - TAG_BYTES)};
  std::string output(message.size(), '\x00');
  if (!sourcemeta_core_aes_256_gcm_open_cryptokit(
          reinterpret_cast<const unsigned char *>(key.data()), key.size(),
          reinterpret_cast<const unsigned char *>(nonce.data()), nonce.size(),
          reinterpret_cast<const unsigned char *>(message.data()),
          message.size(), reinterpret_cast<const unsigned char *>(tag.data()),
          tag.size(), reinterpret_cast<unsigned char *>(output.data()))) {
    return std::nullopt;
  }

  return output;
}

} // namespace sourcemeta::core
