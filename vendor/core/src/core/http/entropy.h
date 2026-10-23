#ifndef SOURCEMETA_CORE_HTTP_ENTROPY_H_
#define SOURCEMETA_CORE_HTTP_ENTROPY_H_

#include <sourcemeta/core/crypto.h>

#include <algorithm> // std::ranges::all_of
#include <cstdint>   // std::uint8_t
#include <span>      // std::span
#include <stdexcept> // std::runtime_error

namespace sourcemeta::core {

// An identifier of all zeroes is invalid, so a draw that lands on one cannot
// be written out. Taking the draw again covers that, and giving up after a
// few attempts reports a provider that only ever returns nothing rather than
// trusting it or spinning on it
// https://www.w3.org/TR/trace-context/#trace-id
inline auto http_draw_identifier(const std::span<std::uint8_t> bytes) -> void {
  for (auto attempt{0}; attempt < 8; ++attempt) {
    random_bytes(bytes);
    if (!std::ranges::all_of(
            bytes, [](const std::uint8_t byte) { return byte == 0; })) {
      return;
    }
  }

  throw std::runtime_error{
      "Could not draw a non-zero trace context identifier"};
}

} // namespace sourcemeta::core

#endif
