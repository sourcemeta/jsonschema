#include "crypto_aes.h"

#include <openssl/evp.h> // EVP_*

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
auto cipher_for_key(const std::size_t key_size) -> const EVP_CIPHER * {
  switch (key_size) {
    case 16:
      return EVP_aes_128_cbc();
    case 24:
      return EVP_aes_192_cbc();
    case 32:
      return EVP_aes_256_cbc();
    default:
      return nullptr;
  }
}
} // namespace

namespace sourcemeta::core {

auto aes_cbc_encrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view plaintext)
    -> std::optional<std::string> {
  const auto *const cipher{cipher_for_key(key.size())};
  auto *context{EVP_CIPHER_CTX_new()};
  if (cipher == nullptr || context == nullptr) {
    EVP_CIPHER_CTX_free(context);
    return std::nullopt;
  }

  std::optional<std::string> result;
  std::string ciphertext(plaintext.size(), '\x00');
  auto *const output{reinterpret_cast<unsigned char *>(ciphertext.data())};
  int length{0};
  int final_length{0};
  if (EVP_EncryptInit_ex(context, cipher, nullptr,
                         reinterpret_cast<const unsigned char *>(key.data()),
                         reinterpret_cast<const unsigned char *>(iv.data())) ==
          1 &&
      // The shared caller applies the padding, so the cipher runs over whole
      // blocks with its own padding disabled
      EVP_CIPHER_CTX_set_padding(context, 0) == 1 &&
      EVP_EncryptUpdate(
          context, output, &length,
          reinterpret_cast<const unsigned char *>(plaintext.data()),
          static_cast<int>(plaintext.size())) == 1 &&
      EVP_EncryptFinal_ex(context, output + length, &final_length) == 1) {
    result = std::move(ciphertext);
  }

  EVP_CIPHER_CTX_free(context);
  return result;
}

auto aes_cbc_decrypt(const std::string_view key, const std::string_view iv,
                     const std::string_view ciphertext)
    -> std::optional<std::string> {
  const auto *const cipher{cipher_for_key(key.size())};
  auto *context{EVP_CIPHER_CTX_new()};
  if (cipher == nullptr || context == nullptr) {
    EVP_CIPHER_CTX_free(context);
    return std::nullopt;
  }

  std::optional<std::string> result;
  std::string plaintext(ciphertext.size(), '\x00');
  auto *const output{reinterpret_cast<unsigned char *>(plaintext.data())};
  int length{0};
  int final_length{0};
  if (EVP_DecryptInit_ex(context, cipher, nullptr,
                         reinterpret_cast<const unsigned char *>(key.data()),
                         reinterpret_cast<const unsigned char *>(iv.data())) ==
          1 &&
      EVP_CIPHER_CTX_set_padding(context, 0) == 1 &&
      EVP_DecryptUpdate(
          context, output, &length,
          reinterpret_cast<const unsigned char *>(ciphertext.data()),
          static_cast<int>(ciphertext.size())) == 1 &&
      EVP_DecryptFinal_ex(context, output + length, &final_length) == 1) {
    result = std::move(plaintext);
  }

  EVP_CIPHER_CTX_free(context);
  return result;
}

} // namespace sourcemeta::core
