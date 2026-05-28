#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/time.h>

namespace sourcemeta::core {

auto is_rfc3339_fulltime(const std::string_view value) -> bool {
  const auto size{value.size()};

  // Minimum valid full-time: "HH:MM:SSZ" = 9 characters
  if (size < 9) {
    return false;
  }

  std::string_view::size_type position{0};

  // --- partial-time: time-hour ":" time-minute ":" time-second ---

  // time-hour = 2DIGIT ; 00-23
  if (!is_digit(value[0]) || !is_digit(value[1])) {
    return false;
  }
  const auto hour{static_cast<unsigned int>(value[0] - '0') * 10 +
                  static_cast<unsigned int>(value[1] - '0')};
  if (hour > 23) {
    return false;
  }
  position = 2;

  // ":"
  if (value[position] != ':') {
    return false;
  }
  position += 1;

  // time-minute = 2DIGIT ; 00-59
  if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
    return false;
  }
  const auto minute{static_cast<unsigned int>(value[position] - '0') * 10 +
                    static_cast<unsigned int>(value[position + 1] - '0')};
  if (minute > 59) {
    return false;
  }
  position += 2;

  // ":"
  if (value[position] != ':') {
    return false;
  }
  position += 1;

  // time-second = 2DIGIT ; 00-60 (60 = leap second per §5.7)
  if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
    return false;
  }
  const auto second{static_cast<unsigned int>(value[position] - '0') * 10 +
                    static_cast<unsigned int>(value[position + 1] - '0')};
  if (second > 60) {
    return false;
  }
  position += 2;

  // --- [time-secfrac] = "." 1*DIGIT ---
  if (position < size && value[position] == '.') {
    position += 1;
    if (position >= size || !is_digit(value[position])) {
      // "." must be followed by at least 1 digit
      return false;
    }
    while (position < size && is_digit(value[position])) {
      position += 1;
    }
  }

  // --- time-offset = "Z" / time-numoffset ---
  if (position >= size) {
    return false;
  }

  char offset_sign{'+'};
  unsigned int offset_hour{0};
  unsigned int offset_minute{0};

  if (value[position] == 'Z' || value[position] == 'z') {
    position += 1;
  } else if (value[position] == '+' || value[position] == '-') {
    offset_sign = value[position];
    position += 1;

    // time-numoffset = ("+" / "-") time-hour ":" time-minute
    if (position + 5 > size) {
      return false;
    }

    // Offset time-hour = 2DIGIT ; 00-23
    if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
      return false;
    }
    offset_hour = static_cast<unsigned int>(value[position] - '0') * 10 +
                  static_cast<unsigned int>(value[position + 1] - '0');
    if (offset_hour > 23) {
      return false;
    }
    position += 2;

    // ":" — REQUIRED (colonless offsets are invalid per §5.6)
    if (value[position] != ':') {
      return false;
    }
    position += 1;

    // Offset time-minute = 2DIGIT ; 00-59
    if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
      return false;
    }
    offset_minute = static_cast<unsigned int>(value[position] - '0') * 10 +
                    static_cast<unsigned int>(value[position + 1] - '0');
    if (offset_minute > 59) {
      return false;
    }
    position += 2;
  } else {
    // Not Z, not +/-, invalid character for time-offset
    return false;
  }

  // String must be fully consumed — no trailing characters
  if (position != size) {
    return false;
  }

  // --- Leap second time-side validation (§5.7) ---
  // For full-time we cannot verify the June-30 / December-31 date constraint
  // (we have no date context), but we can verify that, after applying the
  // offset, the UTC time-of-day is exactly 23:59 — the only moment a leap
  // second may legally appear
  if (second == 60) {
    const auto local_minute_of_day{hour * 60 + minute};
    const auto offset_total_minutes{offset_hour * 60 + offset_minute};

    unsigned int utc_minute_of_day{0};
    if (offset_sign == '+') {
      if (local_minute_of_day >= offset_total_minutes) {
        utc_minute_of_day = local_minute_of_day - offset_total_minutes;
      } else {
        utc_minute_of_day = local_minute_of_day + 1440 - offset_total_minutes;
      }
    } else {
      utc_minute_of_day = (local_minute_of_day + offset_total_minutes) % 1440;
    }

    if (utc_minute_of_day != 23 * 60 + 59) {
      return false;
    }
  }

  return true;
}

} // namespace sourcemeta::core
