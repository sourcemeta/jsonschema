#include <sourcemeta/core/crypto_sha512.h>

#include <openssl/evp.h> // EVP_*

#include <array>     // std::array
#include <cstdint>   // std::uint8_t
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto sha512_digest(const std::string_view input)
    -> std::array<std::uint8_t, 64> {
  auto *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    throw std::runtime_error("Could not allocate OpenSSL digest context");
  }

  if (EVP_DigestInit_ex(context, EVP_sha512(), nullptr) != 1 ||
      EVP_DigestUpdate(context, input.data(), input.size()) != 1) {
    EVP_MD_CTX_free(context);
    throw std::runtime_error("Could not compute SHA-512 digest");
  }

  std::array<std::uint8_t, 64> digest{};
  unsigned int length = 0;
  if (EVP_DigestFinal_ex(context, digest.data(), &length) != 1) {
    EVP_MD_CTX_free(context);
    throw std::runtime_error("Could not finalize SHA-512 digest");
  }

  EVP_MD_CTX_free(context);
  return digest;
}

} // namespace sourcemeta::core
