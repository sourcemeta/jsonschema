#include "crypto_random.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <array>     // std::array
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto fill_random_bytes(std::array<unsigned char, 16> &bytes) -> void {
  if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, bytes.data(),
                                      static_cast<ULONG>(bytes.size()),
                                      BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
    throw std::runtime_error("Could not generate random bytes with CNG");
  }
}

} // namespace sourcemeta::core
