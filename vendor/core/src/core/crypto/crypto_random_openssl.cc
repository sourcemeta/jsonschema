#include "crypto_random.h"

#include <openssl/rand.h> // RAND_bytes

#include <algorithm> // std::min
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <limits>    // std::numeric_limits
#include <span>      // std::span
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

auto fill_random_bytes(std::span<std::uint8_t> bytes) -> void {
  // RAND_bytes takes a signed int length, so fill in chunks to avoid narrowing
  // a larger span into a truncated or negative length
  while (!bytes.empty()) {
    const auto chunk{static_cast<int>(
        std::min<std::size_t>(bytes.size(), std::numeric_limits<int>::max()))};
    if (RAND_bytes(bytes.data(), chunk) != 1) {
      throw std::runtime_error("Could not generate random bytes with OpenSSL");
    }

    bytes = bytes.subspan(static_cast<std::size_t>(chunk));
  }
}

} // namespace sourcemeta::core
