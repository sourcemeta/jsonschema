#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/time.h>

namespace sourcemeta::core {

auto is_rfc3339_partialtime_no_secfrac(const std::string_view value) -> bool {
  // partial-time without [time-secfrac] is exactly "HH:MM:SS" = 8 characters
  if (value.size() != 8) {
    return false;
  }

  // time-hour = 2DIGIT ; 00-23
  if (!is_digit(value[0]) || !is_digit(value[1])) {
    return false;
  }
  const auto hour{static_cast<unsigned int>(value[0] - '0') * 10 +
                  static_cast<unsigned int>(value[1] - '0')};
  if (hour > 23) {
    return false;
  }

  if (value[2] != ':') {
    return false;
  }

  // time-minute = 2DIGIT ; 00-59
  if (!is_digit(value[3]) || !is_digit(value[4])) {
    return false;
  }
  const auto minute{static_cast<unsigned int>(value[3] - '0') * 10 +
                    static_cast<unsigned int>(value[4] - '0')};
  if (minute > 59) {
    return false;
  }

  if (value[5] != ':') {
    return false;
  }

  // time-second = 2DIGIT ; 00-60 (60 = leap second per §5.7)
  if (!is_digit(value[6]) || !is_digit(value[7])) {
    return false;
  }
  const auto second{static_cast<unsigned int>(value[6] - '0') * 10 +
                    static_cast<unsigned int>(value[7] - '0')};
  if (second > 60) {
    return false;
  }

  // Leap second per §5.7: only legal at 23:59 UTC. partial-time carries no
  // offset, so we treat the value as UTC and require 23:59:60 exactly
  if (second == 60 && (hour != 23 || minute != 59)) {
    return false;
  }

  return true;
}

} // namespace sourcemeta::core
