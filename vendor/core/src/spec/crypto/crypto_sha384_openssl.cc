#include <sourcemeta/core/crypto_error.h>
#include <sourcemeta/core/crypto_sha384.h>

#include <openssl/evp.h> // EVP_*

#include <array>   // std::array
#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

auto sha384_digest(const std::string_view input)
    -> std::array<std::uint8_t, 48> {
  auto *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    throw CryptoError("Could not allocate OpenSSL digest context");
  }

  if (EVP_DigestInit_ex(context, EVP_sha384(), nullptr) != 1 ||
      EVP_DigestUpdate(context, input.data(), input.size()) != 1) {
    EVP_MD_CTX_free(context);
    throw CryptoError("Could not compute SHA-384 digest");
  }

  std::array<std::uint8_t, 48> digest{};
  unsigned int length = 0;
  if (EVP_DigestFinal_ex(context, digest.data(), &length) != 1) {
    EVP_MD_CTX_free(context);
    throw CryptoError("Could not finalize SHA-384 digest");
  }

  EVP_MD_CTX_free(context);
  return digest;
}

} // namespace sourcemeta::core
