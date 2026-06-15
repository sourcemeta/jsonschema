#include <sourcemeta/core/crypto_verify.h>
#include <sourcemeta/core/text.h>

#include "crypto_helpers.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG, LPCWSTR

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <bit>         // std::countl_zero
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <cstring>     // std::memcpy
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto to_cng_algorithm(
    const sourcemeta::core::SignatureHashFunction hash) noexcept -> LPCWSTR {
  switch (hash) {
    case sourcemeta::core::SignatureHashFunction::SHA256:
      return BCRYPT_SHA256_ALGORITHM;
    case sourcemeta::core::SignatureHashFunction::SHA384:
      return BCRYPT_SHA384_ALGORITHM;
    case sourcemeta::core::SignatureHashFunction::SHA512:
      return BCRYPT_SHA512_ALGORITHM;
  }

  std::unreachable();
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

  const auto modulus_bit_length{
      (stripped_modulus.size() * 8u) -
      static_cast<std::size_t>(std::countl_zero(
          static_cast<std::uint8_t>(stripped_modulus.front())))};

  BCRYPT_RSAKEY_BLOB header{};
  header.Magic = BCRYPT_RSAPUBLIC_MAGIC;
  header.BitLength = static_cast<ULONG>(modulus_bit_length);
  header.cbPublicExp = static_cast<ULONG>(stripped_exponent.size());
  header.cbModulus = static_cast<ULONG>(stripped_modulus.size());
  header.cbPrime1 = 0;
  header.cbPrime2 = 0;

  std::string blob;
  blob.resize(sizeof(header));
  std::memcpy(blob.data(), &header, sizeof(header));
  blob.append(stripped_exponent);
  blob.append(stripped_modulus);

  BCRYPT_ALG_HANDLE algorithm{nullptr};
  if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(
          &algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0))) {
    return false;
  }

  BCRYPT_KEY_HANDLE key{nullptr};
  if (!BCRYPT_SUCCESS(
          BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPUBLIC_BLOB, &key,
                              reinterpret_cast<unsigned char *>(blob.data()),
                              static_cast<ULONG>(blob.size()), 0))) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return false;
  }

  const auto digest{sourcemeta::core::digest_message(hash, message)};

  // The signature parameter is not const-qualified but is input only
  auto result{false};
  if (probabilistic) {
    // The digest-length salt is what RFC 7518 Section 3.5 requires
    BCRYPT_PSS_PADDING_INFO padding{};
    padding.pszAlgId = to_cng_algorithm(hash);
    padding.cbSalt = static_cast<ULONG>(digest.size());
    result = BCRYPT_SUCCESS(BCryptVerifySignature(
        key, &padding,
        reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data())),
        static_cast<ULONG>(digest.size()),
        reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
        static_cast<ULONG>(signature.size()), BCRYPT_PAD_PSS));
  } else {
    BCRYPT_PKCS1_PADDING_INFO padding{};
    padding.pszAlgId = to_cng_algorithm(hash);
    result = BCRYPT_SUCCESS(BCryptVerifySignature(
        key, &padding,
        reinterpret_cast<unsigned char *>(const_cast<char *>(digest.data())),
        static_cast<ULONG>(digest.size()),
        reinterpret_cast<unsigned char *>(const_cast<char *>(signature.data())),
        static_cast<ULONG>(signature.size()), BCRYPT_PAD_PKCS1));
  }

  BCryptDestroyKey(key);
  BCryptCloseAlgorithmProvider(algorithm, 0);
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
