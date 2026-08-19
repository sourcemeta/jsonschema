#include <sourcemeta/core/time.h>

#include <algorithm> // std::clamp
#include <chrono> // std::chrono::system_clock, std::chrono::seconds, std::chrono::duration_cast
#include <ratio> // std::ratio, std::ratio_less_equal_v

namespace {

using Clock = std::chrono::system_clock;

// Expressing the clock's widest span in seconds is a division only while a tick
// is no coarser than a second, which holds for every standard library this
// builds against. A coarser tick would turn it into a multiplication that
// overflows, so the assumption is stated here rather than left to be discovered
static_assert(std::ratio_less_equal_v<Clock::period, std::ratio<1>>);

// The span reduced to ticks the clock can hold, truncated toward zero so the
// conversion back can never exceed the representable range. A negative span
// becomes none, since a span that runs backwards has no meaning here and
// shifting the wrong way would be worse than ignoring it
auto bounded_ticks(const std::chrono::seconds span) noexcept
    -> Clock::duration {
  constexpr auto LIMIT{
      std::chrono::duration_cast<std::chrono::seconds>(Clock::duration::max())};
  return std::chrono::duration_cast<Clock::duration>(
      std::clamp(span, std::chrono::seconds::zero(), LIMIT));
}

} // namespace

namespace sourcemeta::core {

auto clock_shift_backward(const std::chrono::system_clock::time_point time,
                          const std::chrono::seconds span) noexcept
    -> std::chrono::system_clock::time_point {
  const auto ticks{bounded_ticks(span)};
  if (time.time_since_epoch() < Clock::duration::min() + ticks) {
    return Clock::time_point{Clock::duration::min()};
  }

  return time - ticks;
}

auto clock_shift_forward(const std::chrono::system_clock::time_point time,
                         const std::chrono::seconds span) noexcept
    -> std::chrono::system_clock::time_point {
  const auto ticks{bounded_ticks(span)};
  if (time.time_since_epoch() > Clock::duration::max() - ticks) {
    return Clock::time_point{Clock::duration::max()};
  }

  return time + ticks;
}

} // namespace sourcemeta::core
