#include <sourcemeta/core/crypto_ecdh.h>

#include "crypto_apple.h"

#include <CoreFoundation/CoreFoundation.h> // CF*, kCF*
#include <Security/Security.h>             // Sec*, kSec*

#include <cstddef>  // std::size_t
#include <optional> // std::optional, std::nullopt
#include <string>   // std::string

namespace sourcemeta::core {

auto ecdh_derive(const PrivateKey &private_key, const PublicKey &public_key)
    -> std::optional<std::string> {
  const auto *const private_internal{private_key.internal()};
  const auto *const public_internal{public_key.internal()};
  if (private_internal == nullptr || public_internal == nullptr ||
      private_internal->kind != PrivateKey::Type::EllipticCurve ||
      public_internal->kind != PublicKey::Type::EllipticCurve ||
      private_internal->field_bytes != public_internal->field_bytes) {
    return std::nullopt;
  }

  auto parameters{CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr, 0,
                                     &kCFTypeDictionaryKeyCallBacks,
                                     &kCFTypeDictionaryValueCallBacks)};
  if (parameters == nullptr) {
    return std::nullopt;
  }

  // The standard exchange returns the agreed point x coordinate, and the peer
  // key was validated on the curve when it was parsed
  auto shared{SecKeyCopyKeyExchangeResult(
      private_internal->key, kSecKeyAlgorithmECDHKeyExchangeStandard,
      public_internal->key, parameters, nullptr)};
  CFRelease(parameters);
  if (shared == nullptr) {
    return std::nullopt;
  }

  const auto length{static_cast<std::size_t>(CFDataGetLength(shared))};
  if (length > private_internal->field_bytes) {
    CFRelease(shared);
    return std::nullopt;
  }

  // Left-pad to the fixed-length x coordinate the contract promises, since a
  // leading zero could otherwise shorten the platform output
  std::string result(private_internal->field_bytes - length, '\x00');
  result.append(reinterpret_cast<const char *>(CFDataGetBytePtr(shared)),
                length);
  CFRelease(shared);
  return result;
}

} // namespace sourcemeta::core
