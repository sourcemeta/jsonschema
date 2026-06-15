#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"

#include <openssl/bn.h>          // BN_*
#include <openssl/core_names.h>  // OSSL_PKEY_PARAM_*
#include <openssl/evp.h>         // EVP_*
#include <openssl/param_build.h> // OSSL_PARAM_*
#include <openssl/rsa.h> // RSA_PKCS1_PSS_PADDING, RSA_PSS_SALTLEN_DIGEST

#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto to_message_digest(
    const sourcemeta::core::SignatureHashFunction hash) noexcept
    -> const EVP_MD * {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return EVP_sha256();
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return EVP_sha384();
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return EVP_sha512();
  }

  std::unreachable();
}

auto make_rsa_public_key(const std::string_view modulus,
                         const std::string_view exponent) -> EVP_PKEY * {
  EVP_PKEY *result{nullptr};
  auto *modulus_number{
      BN_bin2bn(reinterpret_cast<const unsigned char *>(modulus.data()),
                static_cast<int>(modulus.size()), nullptr)};
  auto *exponent_number{
      BN_bin2bn(reinterpret_cast<const unsigned char *>(exponent.data()),
                static_cast<int>(exponent.size()), nullptr)};
  auto *builder{OSSL_PARAM_BLD_new()};

  if (modulus_number != nullptr && exponent_number != nullptr &&
      builder != nullptr &&
      OSSL_PARAM_BLD_push_BN(builder, OSSL_PKEY_PARAM_RSA_N, modulus_number) ==
          1 &&
      OSSL_PARAM_BLD_push_BN(builder, OSSL_PKEY_PARAM_RSA_E, exponent_number) ==
          1) {
    auto *parameters{OSSL_PARAM_BLD_to_param(builder)};
    if (parameters != nullptr) {
      auto *context{EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr)};
      if (context != nullptr) {
        if (EVP_PKEY_fromdata_init(context) == 1) {
          EVP_PKEY_fromdata(context, &result, EVP_PKEY_PUBLIC_KEY, parameters);
        }

        EVP_PKEY_CTX_free(context);
      }

      OSSL_PARAM_free(parameters);
    }
  }

  OSSL_PARAM_BLD_free(builder);
  BN_free(exponent_number);
  BN_free(modulus_number);
  return result;
}

auto verify_rsa_signature(const sourcemeta::core::SignatureHashFunction hash,
                          const std::string_view modulus,
                          const std::string_view exponent,
                          const std::string_view message,
                          const std::string_view signature,
                          const bool probabilistic) -> bool {
  const auto stripped_modulus{sourcemeta::core::strip_left(modulus, '\x00')};
  const auto stripped_exponent{sourcemeta::core::strip_left(exponent, '\x00')};
  if (stripped_modulus.empty() || stripped_exponent.empty() ||
      stripped_modulus.size() > sourcemeta::core::MAXIMUM_KEY_BYTES ||
      stripped_exponent.size() > sourcemeta::core::MAXIMUM_KEY_BYTES) {
    return false;
  }

  auto *key{make_rsa_public_key(stripped_modulus, stripped_exponent)};
  if (key == nullptr) {
    return false;
  }

  auto result{false};
  auto *context{EVP_MD_CTX_new()};
  if (context != nullptr) {
    EVP_PKEY_CTX *key_context{nullptr};
    auto ready{EVP_DigestVerifyInit(context, &key_context,
                                    to_message_digest(hash), nullptr,
                                    key) == 1};
    if (ready && probabilistic) {
      // Requesting the digest-length salt that RFC 7518 Section 3.5
      // requires makes verification reject signatures carrying any other
      // salt length
      ready = EVP_PKEY_CTX_set_rsa_padding(key_context,
                                           RSA_PKCS1_PSS_PADDING) == 1 &&
              EVP_PKEY_CTX_set_rsa_pss_saltlen(key_context,
                                               RSA_PSS_SALTLEN_DIGEST) == 1;
    }

    if (ready) {
      result = EVP_DigestVerify(
                   context,
                   reinterpret_cast<const unsigned char *>(signature.data()),
                   signature.size(),
                   reinterpret_cast<const unsigned char *>(message.data()),
                   message.size()) == 1;
    }

    EVP_MD_CTX_free(context);
  }

  EVP_PKEY_free(key);
  return result;
}

} // namespace

namespace sourcemeta::core {

auto rsassa_pkcs1_v15_verify(const SignatureHashFunction hash,
                             const std::string_view modulus,
                             const std::string_view exponent,
                             const std::string_view message,
                             const std::string_view signature) -> bool {
  return verify_rsa_signature(hash, modulus, exponent, message, signature,
                              false);
}

auto rsassa_pss_verify(const SignatureHashFunction hash,
                       const std::string_view modulus,
                       const std::string_view exponent,
                       const std::string_view message,
                       const std::string_view signature) -> bool {
  return verify_rsa_signature(hash, modulus, exponent, message, signature,
                              true);
}

} // namespace sourcemeta::core
