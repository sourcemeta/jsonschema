#include <sourcemeta/core/time.h>

#include <chrono>   // std::chrono::duration, std::chrono::system_clock
#include <cmath>    // std::isfinite
#include <optional> // std::optional, std::nullopt

namespace sourcemeta::core {

auto from_unix_timestamp(const std::chrono::duration<double> seconds) noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  if (!std::isfinite(seconds.count())) {
    return std::nullopt;
  }

  // Reject timestamps outside the clock's representable window, leaving a one
  // second guard so that the conversion to the clock's native tick cannot
  // overflow at the boundary
  constexpr auto maximum{
      std::chrono::duration_cast<std::chrono::duration<double>>(
          std::chrono::system_clock::duration::max()) -
      std::chrono::duration<double>{1}};
  constexpr auto minimum{
      std::chrono::duration_cast<std::chrono::duration<double>>(
          std::chrono::system_clock::duration::min()) +
      std::chrono::duration<double>{1}};
  if (seconds < minimum || seconds > maximum) {
    return std::nullopt;
  }

  return std::chrono::system_clock::time_point{
      std::chrono::duration_cast<std::chrono::system_clock::duration>(seconds)};
}

auto to_unix_timestamp(
    const std::chrono::system_clock::time_point time) noexcept
    -> std::chrono::duration<double> {
  return std::chrono::duration_cast<std::chrono::duration<double>>(
      time.time_since_epoch());
}

} // namespace sourcemeta::core
