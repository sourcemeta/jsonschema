#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/time.h>

#include <cstdint> // std::uint8_t

namespace sourcemeta::core {

namespace {

// Finite-state tracker for the duration scan. The state captures both the
// "section" (date before T, time after T) and the last unit parsed, so the
// overloaded 'M' letter (month vs minute) is disambiguated by state alone
enum class DurationState : std::uint8_t {
  Start,
  AfterYear,
  AfterDateMonth,
  AfterDay,
  AfterWeek,
  TimeStart,
  AfterHour,
  AfterTimeMinute,
  AfterSecond,
};

} // namespace

auto is_rfc3339_duration(const std::string_view value) -> bool {
  const auto size{value.size()};

  // Must start with 'P' and have at least one element after
  if (size < 2 || value[0] != 'P') {
    return false;
  }

  DurationState state{DurationState::Start};
  std::string_view::size_type position{1};

  while (position < size) {
    if (value[position] == 'T') {
      // "T" introduces the optional dur-time block. It is allowed from any
      // date-section state (or directly after "P"), and after it at least
      // one time element MUST appear (we re-enter the loop, which requires
      // a digit next)
      if (state != DurationState::Start && state != DurationState::AfterYear &&
          state != DurationState::AfterDateMonth &&
          state != DurationState::AfterDay) {
        return false;
      }
      state = DurationState::TimeStart;
      position += 1;
      continue;
    }

    // Parse 1*DIGIT
    const auto digit_start{position};
    while (position < size && is_digit(value[position])) {
      position += 1;
    }
    if (position == digit_start) {
      // Either no digits parsed, or a non-digit character was encountered
      return false;
    }
    if (position >= size) {
      // Digits without a trailing unit letter
      return false;
    }

    const auto unit{value[position]};
    position += 1;

    switch (unit) {
      case 'Y':
        if (state != DurationState::Start) {
          return false;
        }
        state = DurationState::AfterYear;
        break;
      case 'M':
        // Month before T, minute after T
        if (state == DurationState::Start ||
            state == DurationState::AfterYear) {
          state = DurationState::AfterDateMonth;
        } else if (state == DurationState::TimeStart ||
                   state == DurationState::AfterHour) {
          state = DurationState::AfterTimeMinute;
        } else {
          return false;
        }
        break;
      case 'D':
        if (state != DurationState::Start &&
            state != DurationState::AfterDateMonth) {
          return false;
        }
        state = DurationState::AfterDay;
        break;
      case 'W':
        if (state != DurationState::Start) {
          return false;
        }
        state = DurationState::AfterWeek;
        break;
      case 'H':
        if (state != DurationState::TimeStart) {
          return false;
        }
        state = DurationState::AfterHour;
        break;
      case 'S':
        if (state != DurationState::TimeStart &&
            state != DurationState::AfterTimeMinute) {
          return false;
        }
        state = DurationState::AfterSecond;
        break;
      default:
        return false;
    }
  }

  // Reject if no element was parsed at all ("P") or if a "T" was seen but
  // no time unit followed ("PT", "P1YT")
  return state != DurationState::Start && state != DurationState::TimeStart;
}

} // namespace sourcemeta::core
