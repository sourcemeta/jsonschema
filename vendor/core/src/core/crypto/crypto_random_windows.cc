#include "crypto_random.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h> // ULONG

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <algorithm> // std::min
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <limits>    // std::numeric_limits
#include <span>      // std::span
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto fill_random_bytes(std::span<std::uint8_t> bytes) -> void {
  // BCryptGenRandom takes a ULONG length, so fill in chunks to avoid narrowing
  // a larger span into a wrapped length that would leave a suffix unfilled
  while (!bytes.empty()) {
    const auto chunk{static_cast<ULONG>(std::min<std::size_t>(
        bytes.size(), std::numeric_limits<ULONG>::max()))};
    if (!BCRYPT_SUCCESS(BCryptGenRandom(nullptr, bytes.data(), chunk,
                                        BCRYPT_USE_SYSTEM_PREFERRED_RNG))) {
      throw std::runtime_error("Could not generate random bytes with CNG");
    }

    bytes = bytes.subspan(chunk);
  }
}

} // namespace sourcemeta::core
