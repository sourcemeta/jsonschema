#include "crypto_random.h"

#include <sourcemeta/core/crypto_error.h>

#include <Security/SecBase.h>   // errSecSuccess
#include <Security/SecRandom.h> // SecRandomCopyBytes, kSecRandomDefault

#include <cstdint> // std::uint8_t
#include <span>    // std::span

namespace sourcemeta::core {

auto fill_random_bytes(std::span<std::uint8_t> bytes) -> void {
  if (SecRandomCopyBytes(kSecRandomDefault, bytes.size(), bytes.data()) !=
      errSecSuccess) {
    throw CryptoError(
        "Could not generate random bytes with the Security framework");
  }
}

} // namespace sourcemeta::core
