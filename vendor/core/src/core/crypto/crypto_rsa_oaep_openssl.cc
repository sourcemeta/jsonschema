#include <sourcemeta/core/crypto_rsa_oaep.h>

#include "crypto_openssl.h"

#include <openssl/evp.h> // EVP_*
#include <openssl/rsa.h> // RSA_PKCS1_OAEP_PADDING

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

namespace sourcemeta::core {

namespace {
auto to_message_digest(const RSAOAEPHash hash) -> const EVP_MD * {
  switch (hash) {
    case RSAOAEPHash::SHA1:
      return EVP_sha1();
    case RSAOAEPHash::SHA256:
      return EVP_sha256();
  }

  std::unreachable();
}

auto configure(EVP_PKEY_CTX *context, const RSAOAEPHash hash) -> bool {
  return EVP_PKEY_CTX_set_rsa_padding(context, RSA_PKCS1_OAEP_PADDING) == 1 &&
         EVP_PKEY_CTX_set_rsa_oaep_md(context, to_message_digest(hash)) == 1 &&
         EVP_PKEY_CTX_set_rsa_mgf1_md(context, to_message_digest(hash)) == 1;
}
} // namespace

auto rsa_oaep_encrypt(const PublicKey &key, const RSAOAEPHash hash,
                      const std::string_view plaintext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA) {
    return std::nullopt;
  }

  auto *context{EVP_PKEY_CTX_new(internal->key, nullptr)};
  if (context == nullptr) {
    return std::nullopt;
  }

  const auto *const input{
      reinterpret_cast<const unsigned char *>(plaintext.data())};
  std::optional<std::string> result;
  std::size_t length{0};
  if (EVP_PKEY_encrypt_init(context) == 1 && configure(context, hash) &&
      EVP_PKEY_encrypt(context, nullptr, &length, input, plaintext.size()) ==
          1) {
    std::string ciphertext(length, '\x00');
    if (EVP_PKEY_encrypt(context,
                         reinterpret_cast<unsigned char *>(ciphertext.data()),
                         &length, input, plaintext.size()) == 1) {
      ciphertext.resize(length);
      result = std::move(ciphertext);
    }
  }

  EVP_PKEY_CTX_free(context);
  return result;
}

auto rsa_oaep_decrypt(const PrivateKey &key, const RSAOAEPHash hash,
                      const std::string_view ciphertext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PrivateKey::Type::RSA ||
      internal->rsa_pss_restricted) {
    return std::nullopt;
  }

  // OpenSSL reads the ciphertext as a raw integer, so a shorter input with a
  // dropped leading zero would decrypt as the same value. Require the exact
  // modulus size so that a malformed ciphertext is rejected
  if (ciphertext.size() !=
      static_cast<std::size_t>(EVP_PKEY_get_size(internal->key))) {
    return std::nullopt;
  }

  auto *context{EVP_PKEY_CTX_new(internal->key, nullptr)};
  if (context == nullptr) {
    return std::nullopt;
  }

  const auto *const input{
      reinterpret_cast<const unsigned char *>(ciphertext.data())};
  std::optional<std::string> result;
  std::size_t length{0};
  if (EVP_PKEY_decrypt_init(context) == 1 && configure(context, hash) &&
      EVP_PKEY_decrypt(context, nullptr, &length, input, ciphertext.size()) ==
          1) {
    std::string plaintext(length, '\x00');
    // A failed OAEP check surfaces as a non-positive return here
    if (EVP_PKEY_decrypt(context,
                         reinterpret_cast<unsigned char *>(plaintext.data()),
                         &length, input, ciphertext.size()) == 1) {
      plaintext.resize(length);
      result = std::move(plaintext);
    }
  }

  EVP_PKEY_CTX_free(context);
  return result;
}

} // namespace sourcemeta::core
