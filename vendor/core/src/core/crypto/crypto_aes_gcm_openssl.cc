#include "crypto_aes.h"

#include <openssl/evp.h> // EVP_*

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
constexpr std::size_t TAG_BYTES{16};
} // namespace

namespace sourcemeta::core {

auto aes_256_gcm_encrypt(const std::string_view key,
                         const std::string_view nonce,
                         const std::string_view plaintext)
    -> std::optional<std::string> {
  auto *context{EVP_CIPHER_CTX_new()};
  if (context == nullptr) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  std::string ciphertext(plaintext.size(), '\x00');
  std::string tag(TAG_BYTES, '\x00');
  int length{0};
  int final_length{0};
  if (EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr,
                         nullptr) == 1 &&
      EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN,
                          static_cast<int>(nonce.size()), nullptr) == 1 &&
      EVP_EncryptInit_ex(
          context, nullptr, nullptr,
          reinterpret_cast<const unsigned char *>(key.data()),
          reinterpret_cast<const unsigned char *>(nonce.data())) == 1 &&
      (plaintext.empty() ||
       EVP_EncryptUpdate(
           context, reinterpret_cast<unsigned char *>(ciphertext.data()),
           &length, reinterpret_cast<const unsigned char *>(plaintext.data()),
           static_cast<int>(plaintext.size())) == 1) &&
      EVP_EncryptFinal_ex(context,
                          reinterpret_cast<unsigned char *>(ciphertext.data()) +
                              length,
                          &final_length) == 1 &&
      EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG,
                          static_cast<int>(TAG_BYTES),
                          reinterpret_cast<unsigned char *>(tag.data())) == 1) {
    ciphertext.append(tag);
    result = std::move(ciphertext);
  }

  EVP_CIPHER_CTX_free(context);
  return result;
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
  auto *context{EVP_CIPHER_CTX_new()};
  if (context == nullptr) {
    return std::nullopt;
  }

  std::optional<std::string> result;
  std::string plaintext(message.size(), '\x00');
  int length{0};
  int final_length{0};
  if (EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), nullptr, nullptr,
                         nullptr) == 1 &&
      EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN,
                          static_cast<int>(nonce.size()), nullptr) == 1 &&
      EVP_DecryptInit_ex(
          context, nullptr, nullptr,
          reinterpret_cast<const unsigned char *>(key.data()),
          reinterpret_cast<const unsigned char *>(nonce.data())) == 1 &&
      (message.empty() ||
       EVP_DecryptUpdate(
           context, reinterpret_cast<unsigned char *>(plaintext.data()),
           &length, reinterpret_cast<const unsigned char *>(message.data()),
           static_cast<int>(message.size())) == 1) &&
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
