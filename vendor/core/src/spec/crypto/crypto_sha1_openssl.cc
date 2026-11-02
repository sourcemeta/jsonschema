#include <sourcemeta/core/crypto_error.h>
#include <sourcemeta/core/crypto_sha1.h>
#include <sourcemeta/core/text.h>

#include <openssl/evp.h> // EVP_*

#include <array> // std::array

namespace sourcemeta::core {

auto sha1(const std::string_view input) -> std::string {
  auto *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    throw CryptoError("Could not allocate OpenSSL digest context");
  }

  if (EVP_DigestInit_ex(context, EVP_sha1(), nullptr) != 1 ||
      EVP_DigestUpdate(context, input.data(), input.size()) != 1) {
    EVP_MD_CTX_free(context);
    throw CryptoError("Could not compute SHA-1 digest");
  }

  std::array<unsigned char, 20> digest{};
  unsigned int length = 0;
  if (EVP_DigestFinal_ex(context, digest.data(), &length) != 1) {
    EVP_MD_CTX_free(context);
    throw CryptoError("Could not finalize SHA-1 digest");
  }

  EVP_MD_CTX_free(context);

  return bytes_to_hex(
      {reinterpret_cast<const char *>(digest.data()), digest.size()});
}

} // namespace sourcemeta::core
