#ifndef SOURCEMETA_CORE_CRYPTO_APPLE_H_
#define SOURCEMETA_CORE_CRYPTO_APPLE_H_

// Security-framework key export helpers shared by the Apple signing and
// verification backends, so the external representation handling stays
// single-sourced

#include <CoreFoundation/CoreFoundation.h> // CF*, kCF*
#include <Security/Security.h>             // Sec*, kSec*

#include <cstddef>  // std::size_t
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string

namespace sourcemeta::core {

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
