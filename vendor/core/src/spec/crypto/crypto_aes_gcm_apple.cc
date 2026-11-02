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

auto aes_gcm_seal(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext> {
  // A single buffer holds the ciphertext followed by its 16-byte tag, which the
  // shim writes into through the two output pointers
  AESGCMCiphertext result{
      .data = std::string(plaintext.size() + TAG_BYTES, '\x00')};
  auto *const output{reinterpret_cast<unsigned char *>(result.data.data())};
  if (!sourcemeta_core_aes_gcm_seal_cryptokit(
          reinterpret_cast<const unsigned char *>(key.data()), key.size(),
          reinterpret_cast<const unsigned char *>(iv.data()), iv.size(),
          reinterpret_cast<const unsigned char *>(plaintext.data()),
          plaintext.size(),
          reinterpret_cast<const unsigned char *>(associated_data.data()),
          associated_data.size(), output, output + plaintext.size())) {
    return std::nullopt;
  }

  return result;
}

auto aes_gcm_open(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view ciphertext, const std::string_view tag)
    -> std::optional<std::string> {
  std::string output(ciphertext.size(), '\x00');
  if (!sourcemeta_core_aes_gcm_open_cryptokit(
          reinterpret_cast<const unsigned char *>(key.data()), key.size(),
          reinterpret_cast<const unsigned char *>(iv.data()), iv.size(),
          reinterpret_cast<const unsigned char *>(ciphertext.data()),
          ciphertext.size(),
          reinterpret_cast<const unsigned char *>(tag.data()), tag.size(),
          reinterpret_cast<const unsigned char *>(associated_data.data()),
          associated_data.size(),
          reinterpret_cast<unsigned char *>(output.data()))) {
    return std::nullopt;
  }

  return output;
}

} // namespace sourcemeta::core
