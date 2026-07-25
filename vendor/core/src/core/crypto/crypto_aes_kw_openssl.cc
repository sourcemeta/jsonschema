#include "crypto_aes.h"

#include <openssl/evp.h> // EVP_*

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
constexpr std::size_t SEMIBLOCK_BYTES{8};

auto wrap_cipher_for_key(const std::size_t key_size) -> const EVP_CIPHER * {
  switch (key_size) {
    case 16:
      return EVP_aes_128_wrap();
    case 24:
      return EVP_aes_192_wrap();
    case 32:
      return EVP_aes_256_wrap();
    default:
      return nullptr;
  }
}
} // namespace

namespace sourcemeta::core {

auto aes_wrap(const std::string_view key_encryption_key,
              const std::string_view key) -> std::optional<std::string> {
  const auto *const cipher{wrap_cipher_for_key(key_encryption_key.size())};
  auto *context{EVP_CIPHER_CTX_new()};
  if (cipher == nullptr || context == nullptr) {
    EVP_CIPHER_CTX_free(context);
    return std::nullopt;
  }

  // The wrap ciphers are disabled unless the context opts in
  EVP_CIPHER_CTX_set_flags(context, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
  std::optional<std::string> result;
  std::string wrapped(key.size() + SEMIBLOCK_BYTES, '\x00');
  auto *const output{reinterpret_cast<unsigned char *>(wrapped.data())};
  int length{0};
  int final_length{0};
  // A null initialization vector selects the RFC 3394 default integrity value
  if (EVP_EncryptInit_ex(
          context, cipher, nullptr,
          reinterpret_cast<const unsigned char *>(key_encryption_key.data()),
          nullptr) == 1 &&
      EVP_EncryptUpdate(context, output, &length,
                        reinterpret_cast<const unsigned char *>(key.data()),
                        static_cast<int>(key.size())) == 1 &&
      EVP_EncryptFinal_ex(context, output + length, &final_length) == 1 &&
      static_cast<std::size_t>(length) +
              static_cast<std::size_t>(final_length) ==
          wrapped.size()) {
    result = std::move(wrapped);
  }

  EVP_CIPHER_CTX_free(context);
  return result;
}

auto aes_unwrap(const std::string_view key_encryption_key,
                const std::string_view wrapped_key)
    -> std::optional<std::string> {
  const auto *const cipher{wrap_cipher_for_key(key_encryption_key.size())};
  auto *context{EVP_CIPHER_CTX_new()};
  if (cipher == nullptr || context == nullptr) {
    EVP_CIPHER_CTX_free(context);
    return std::nullopt;
  }

  EVP_CIPHER_CTX_set_flags(context, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
  std::optional<std::string> result;
  std::string key(wrapped_key.size() - SEMIBLOCK_BYTES, '\x00');
  auto *const output{reinterpret_cast<unsigned char *>(key.data())};
  int length{0};
  int final_length{0};
  // The integrity check runs during decryption, so a failure surfaces here
  if (EVP_DecryptInit_ex(
          context, cipher, nullptr,
          reinterpret_cast<const unsigned char *>(key_encryption_key.data()),
          nullptr) == 1 &&
      EVP_DecryptUpdate(
          context, output, &length,
          reinterpret_cast<const unsigned char *>(wrapped_key.data()),
          static_cast<int>(wrapped_key.size())) == 1 &&
      EVP_DecryptFinal_ex(context, output + length, &final_length) == 1 &&
      static_cast<std::size_t>(length) +
              static_cast<std::size_t>(final_length) ==
          key.size()) {
    result = std::move(key);
  }

  EVP_CIPHER_CTX_free(context);
  return result;
}

} // namespace sourcemeta::core
