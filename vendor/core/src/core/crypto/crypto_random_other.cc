#include "crypto_random.h"

#include <sys/random.h> // getentropy

#include <algorithm> // std::min
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <fstream>   // std::ifstream
#include <ios>       // std::ios::binary
#include <span>      // std::span
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

namespace {

// Read from the kernel random device, used only on the rare platforms where
// getentropy is present but the kernel call is unavailable
auto fill_from_random_device(const std::span<std::uint8_t> bytes) -> void {
  std::ifstream device{"/dev/urandom", std::ios::binary};
  if (!device) {
    throw std::runtime_error("Could not open the system random device");
  }

  // A single read can return fewer bytes than requested when interrupted by a
  // signal, so the buffer is filled across as many reads as it takes, failing
  // only when a read makes no progress or the device faults
  std::size_t offset{0};
  while (offset < bytes.size()) {
    device.read(reinterpret_cast<char *>(bytes.data() + offset),
                static_cast<std::streamsize>(bytes.size() - offset));
    const auto count{device.gcount()};
    if (device.bad() || count <= 0) {
      throw std::runtime_error("Could not read from the system random device");
    }

    offset += static_cast<std::size_t>(count);
    device.clear();
  }
}

} // namespace

auto fill_random_bytes(std::span<std::uint8_t> bytes) -> void {
  // getentropy draws from the kernel cryptographic generator and fails closed,
  // never returning low-quality bytes, but fills at most 256 bytes per call
  constexpr std::size_t maximum_per_call{256};
  std::size_t offset{0};
  while (offset < bytes.size()) {
    const auto chunk{std::min(bytes.size() - offset, maximum_per_call)};
    if (getentropy(bytes.data() + offset, chunk) != 0) {
      fill_from_random_device(bytes.subspan(offset));
      return;
    }

    offset += chunk;
  }
}

} // namespace sourcemeta::core
