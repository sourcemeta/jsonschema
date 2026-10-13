#include "crypto_random.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // ULONG

#include <bcrypt.h> // BCrypt*, BCRYPT_*

#include <algorithm> // std::min
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <limits>    // std::numeric_limits
#include <span>      // std::span
#include <stdexcept> // std::runtime_error
#else
// glibc and the BSDs declare the entropy call in the random header while
// musl only declares it in the standard POSIX header, so both are needed
#include <sys/random.h> // getentropy
#include <unistd.h>     // getentropy

#include <algorithm> // std::min
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <span>      // std::span
#include <stdexcept> // std::runtime_error
#endif

namespace sourcemeta::core {

auto fill_random_bytes(std::span<std::uint8_t> bytes) -> void {
#if defined(_WIN32)
  // The kernel cryptographic generator is only reachable through CNG, as the
  // standard library offers no source that is guaranteed to be one.
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
#else
  // getentropy draws from the kernel cryptographic generator and fails closed,
  // never returning low-quality bytes, but fills at most 256 bytes per call.
  //
  // There is no second source behind it. The call is reached through the
  // kernel one that every system this builds on carries, and a refusal from
  // it is a refusal by whoever set the policy that produced it rather than a
  // gap to route around: reading the random device instead would walk past a
  // sandbox that turned the call away on purpose. So a failure here is final,
  // which is how the generator on the other side of this file answers too
  constexpr std::size_t maximum_per_call{256};
  std::size_t offset{0};
  while (offset < bytes.size()) {
    const auto chunk{std::min(bytes.size() - offset, maximum_per_call)};
    if (getentropy(bytes.data() + offset, chunk) != 0) {
      throw std::runtime_error(
          "Could not generate random bytes with the system entropy call");
    }

    offset += chunk;
  }
#endif
}

} // namespace sourcemeta::core
