#include "crypto_random.h"

#include <Security/SecBase.h>   // errSecSuccess
#include <Security/SecRandom.h> // SecRandomCopyBytes, kSecRandomDefault

#include <array>     // std::array
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto fill_random_bytes(std::array<unsigned char, 16> &bytes) -> void {
  if (SecRandomCopyBytes(kSecRandomDefault, bytes.size(), bytes.data()) !=
      errSecSuccess) {
    throw std::runtime_error(
        "Could not generate random bytes with the Security framework");
  }
}

} // namespace sourcemeta::core
