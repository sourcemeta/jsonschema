#ifndef SOURCEMETA_CORE_CRYPTO_APPLE_H_
#define SOURCEMETA_CORE_CRYPTO_APPLE_H_

// The Apple key internals and the Security-framework export helper shared
// across the Apple backends, so the parsed key layout and the external
// representation handling stay single-sourced

#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/crypto_verify.h>

#include <CoreFoundation/CoreFoundation.h> // CF*, kCF*
#include <Security/Security.h>             // Sec*, kSec*

#include <cstddef>  // std::size_t
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string

namespace sourcemeta::core {

// The parsed key keeps the platform key object alive for reuse. The Edwards
// curves have no Security framework primitive, so they keep the raw seed or
// point and operate through CryptoKit or the reference implementation
struct PublicKey::Internal {
  PublicKey::Type kind;
  SecKeyRef key;
  std::string modulus;
  std::size_t field_bytes;
  std::string edwards_point;
  EdwardsCurve edwards_curve;
};

struct PrivateKey::Internal {
  PrivateKey::Type kind;
  SecKeyRef key;
  std::size_t field_bytes;
  std::string edwards_seed;
  EdwardsCurve edwards_curve;
  bool rsa_pss_restricted{false};
};

// Copy the platform's external representation of a key, the PKCS#1 structure
// for RSA and the X9.63 uncompressed point for elliptic curve keys
inline auto copy_external_representation(SecKeyRef key)
    -> std::optional<std::string> {
  CFErrorRef error{nullptr};
  auto data{SecKeyCopyExternalRepresentation(key, &error)};
  if (data == nullptr) {
    if (error != nullptr) {
      CFRelease(error);
    }

    return std::nullopt;
  }

  std::string result{reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                     static_cast<std::size_t>(CFDataGetLength(data))};
  CFRelease(data);
  return result;
}

} // namespace sourcemeta::core

#endif
