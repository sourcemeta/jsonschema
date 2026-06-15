#include "crypto_random.h"

#include <array> // std::array
#include <random> // std::random_device, std::mt19937, std::uniform_int_distribution

namespace sourcemeta::core {

auto fill_random_bytes(std::array<unsigned char, 16> &bytes) -> void {
  // Not a cryptographically secure generator. This fallback only exists to
  // keep the module buildable on platforms without a system provider
  thread_local std::random_device device;
  thread_local std::mt19937 generator{device()};
  std::uniform_int_distribution<unsigned int> distribution{0, 255};
  for (auto &byte : bytes) {
    byte = static_cast<unsigned char>(distribution(generator));
  }
}

} // namespace sourcemeta::core
