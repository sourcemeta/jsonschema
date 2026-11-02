#include "crypto_aes.h"

#include <openssl/evp.h> // EVP_*

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
constexpr std::size_t TAG_BYTES{16};

auto cipher_for_key(const std::size_t key_size) -> const EVP_CIPHER * {
  switch (key_size) {
    case 16:
      return EVP_aes_128_gcm();
    case 24:
      return EVP_aes_192_gcm();
    case 32:
      return EVP_aes_256_gcm();
    default:
      return nullptr;
  }
}
} // namespace

namespace sourcemeta::core {

auto aes_gcm_seal(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view plaintext)
    -> std::optional<AESGCMCiphertext> {
  const auto *const cipher{cipher_for_key(key.size())};
  auto *context{EVP_CIPHER_CTX_new()};
  if (cipher == nullptr || context == nullptr) {
    EVP_CIPHER_CTX_free(context);
    return std::nullopt;
  }

  std::optional<AESGCMCiphertext> result;
  // A single buffer holds the ciphertext followed by its 16-byte tag
  std::string data(plaintext.size() + TAG_BYTES, '\x00');
  auto *const output{reinterpret_cast<unsigned char *>(data.data())};
  int length{0};
  int final_length{0};
  int associated_length{0};
  if (EVP_EncryptInit_ex(context, cipher, nullptr, nullptr, nullptr) == 1 &&
      EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN,
                          static_cast<int>(iv.size()), nullptr) == 1 &&
      EVP_EncryptInit_ex(context, nullptr, nullptr,
                         reinterpret_cast<const unsigned char *>(key.data()),
                         reinterpret_cast<const unsigned char *>(iv.data())) ==
          1 &&
      (associated_data.empty() ||
       EVP_EncryptUpdate(
           context, nullptr, &associated_length,
           reinterpret_cast<const unsigned char *>(associated_data.data()),
           static_cast<int>(associated_data.size())) == 1) &&
      (plaintext.empty() ||
       EVP_EncryptUpdate(
           context, output, &length,
           reinterpret_cast<const unsigned char *>(plaintext.data()),
           static_cast<int>(plaintext.size())) == 1) &&
      EVP_EncryptFinal_ex(context, output + length, &final_length) == 1 &&
      EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG,
                          static_cast<int>(TAG_BYTES),
                          output + plaintext.size()) == 1) {
    result = AESGCMCiphertext{.data = std::move(data)};
  }

  EVP_CIPHER_CTX_free(context);
  return result;
}

auto aes_gcm_open(const std::string_view key, const std::string_view iv,
                  const std::string_view associated_data,
                  const std::string_view ciphertext, const std::string_view tag)
    -> std::optional<std::string> {
  const auto *const cipher{cipher_for_key(key.size())};
  auto *context{EVP_CIPHER_CTX_new()};
  if (cipher == nullptr || context == nullptr) {
    EVP_CIPHER_CTX_free(context);
    return std::nullopt;
  }

  std::optional<std::string> result;
  std::string plaintext(ciphertext.size(), '\x00');
  int length{0};
  int final_length{0};
  int associated_length{0};
  if (EVP_DecryptInit_ex(context, cipher, nullptr, nullptr, nullptr) == 1 &&
      EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN,
                          static_cast<int>(iv.size()), nullptr) == 1 &&
      EVP_DecryptInit_ex(context, nullptr, nullptr,
                         reinterpret_cast<const unsigned char *>(key.data()),
                         reinterpret_cast<const unsigned char *>(iv.data())) ==
          1 &&
      (associated_data.empty() ||
       EVP_DecryptUpdate(
           context, nullptr, &associated_length,
           reinterpret_cast<const unsigned char *>(associated_data.data()),
           static_cast<int>(associated_data.size())) == 1) &&
      (ciphertext.empty() ||
       EVP_DecryptUpdate(
           context, reinterpret_cast<unsigned char *>(plaintext.data()),
           &length, reinterpret_cast<const unsigned char *>(ciphertext.data()),
           static_cast<int>(ciphertext.size())) == 1) &&
      EVP_CIPHER_CTX_ctrl(
          context, EVP_CTRL_GCM_SET_TAG, static_cast<int>(TAG_BYTES),
          const_cast<unsigned char *>(
              reinterpret_cast<const unsigned char *>(tag.data()))) == 1 &&
      // The final step verifies the tag and reports failure for a mismatch
      EVP_DecryptFinal_ex(
          context, reinterpret_cast<unsigned char *>(plaintext.data()) + length,
          &final_length) == 1) {
    result = std::move(plaintext);
  }

  EVP_CIPHER_CTX_free(context);
  return result;
}

} // namespace sourcemeta::core
