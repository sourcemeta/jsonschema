#include <sourcemeta/core/crypto_rsa_oaep.h>

#include "crypto_apple.h"

#include <CoreFoundation/CoreFoundation.h> // CF*, kCF*
#include <Security/Security.h>             // Sec*, kSec*

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

namespace {
auto make_data(const std::string_view value) -> CFDataRef {
  return CFDataCreate(kCFAllocatorDefault,
                      reinterpret_cast<const std::uint8_t *>(value.data()),
                      static_cast<CFIndex>(value.size()));
}

auto to_oaep_algorithm(const RSAOAEPHash hash) -> SecKeyAlgorithm {
  switch (hash) {
    case RSAOAEPHash::SHA1:
      return kSecKeyAlgorithmRSAEncryptionOAEPSHA1;
    case RSAOAEPHash::SHA256:
      return kSecKeyAlgorithmRSAEncryptionOAEPSHA256;
  }

  std::unreachable();
}
} // namespace

auto rsa_oaep_encrypt(const PublicKey &key, const RSAOAEPHash hash,
                      const std::string_view plaintext)
    -> std::optional<std::string> {
  const auto *const internal{key.internal()};
  if (internal == nullptr || internal->kind != PublicKey::Type::RSA) {
    return std::nullopt;
  }

  auto plaintext_data{make_data(plaintext)};
  if (plaintext_data == nullptr) {
    return std::nullopt;
  }

  auto ciphertext{SecKeyCreateEncryptedData(
      internal->key, to_oaep_algorithm(hash), plaintext_data, nullptr)};
  CFRelease(plaintext_data);
  if (ciphertext == nullptr) {
    return std::nullopt;
  }

  std::string result{
      reinterpret_cast<const char *>(CFDataGetBytePtr(ciphertext)),
      static_cast<std::size_t>(CFDataGetLength(ciphertext))};
  CFRelease(ciphertext);
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

  auto ciphertext_data{make_data(ciphertext)};
  if (ciphertext_data == nullptr) {
    return std::nullopt;
  }

  auto plaintext{SecKeyCreateDecryptedData(
      internal->key, to_oaep_algorithm(hash), ciphertext_data, nullptr)};
  CFRelease(ciphertext_data);
  if (plaintext == nullptr) {
    return std::nullopt;
  }

  std::string result{
      reinterpret_cast<const char *>(CFDataGetBytePtr(plaintext)),
      static_cast<std::size_t>(CFDataGetLength(plaintext))};
  CFRelease(plaintext);
  return result;
}

} // namespace sourcemeta::core
