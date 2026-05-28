#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/time.h>

#include <cstdint> // std::uint8_t, std::uint16_t

namespace sourcemeta::core {

auto is_rfc3339_datetime(const std::string_view value) -> bool {
  const auto size{value.size()};

  // Minimum valid date-time: "YYYY-MM-DDTHH:MM:SSZ" = 20 characters
  if (size < 20) {
    return false;
  }

  // --- full-date: date-fullyear "-" date-month "-" date-mday ---
  if (!is_rfc3339_fulldate(value.substr(0, 10))) {
    return false;
  }

  // --- "T" or "t" separator ---
  if (value[10] != 'T' && value[10] != 't') {
    return false;
  }

  // --- full-time = partial-time time-offset ---
  if (!is_rfc3339_fulltime(value.substr(11))) {
    return false;
  }

  // --- Date-aware leap second validation (§5.7) ---
  // is_rfc3339_fulltime already verified that, if second == 60, the UTC
  // time-of-day after offset is 23:59. We additionally need to confirm that
  // the UTC date (after any day-rollback from the offset) is June 30 or
  // December 31. If second != 60, no further check is needed
  const auto second{static_cast<unsigned int>(value[17] - '0') * 10 +
                    static_cast<unsigned int>(value[18] - '0')};
  if (second != 60) {
    return true;
  }

  const auto year{static_cast<std::uint16_t>(
      (value[0] - '0') * 1000 + (value[1] - '0') * 100 + (value[2] - '0') * 10 +
      (value[3] - '0'))};
  const auto month{
      static_cast<std::uint8_t>((value[5] - '0') * 10 + (value[6] - '0'))};
  const auto day{
      static_cast<std::uint8_t>((value[8] - '0') * 10 + (value[9] - '0'))};
  const auto hour{static_cast<unsigned int>(value[11] - '0') * 10 +
                  static_cast<unsigned int>(value[12] - '0')};
  const auto minute{static_cast<unsigned int>(value[14] - '0') * 10 +
                    static_cast<unsigned int>(value[15] - '0')};

  // Locate the offset. After is_rfc3339_fulltime validation, the input ends
  // in either "Z"/"z" (1 char) or "[+-]HH:MM" (6 chars). The intervening
  // secfrac is variable-length, but the offset is always at the very end
  char offset_sign{'+'};
  unsigned int offset_hour{0};
  unsigned int offset_minute{0};
  if (value.back() != 'Z' && value.back() != 'z') {
    const auto offset_start{size - 6};
    offset_sign = value[offset_start];
    offset_hour =
        static_cast<unsigned int>(value[offset_start + 1] - '0') * 10 +
        static_cast<unsigned int>(value[offset_start + 2] - '0');
    offset_minute =
        static_cast<unsigned int>(value[offset_start + 4] - '0') * 10 +
        static_cast<unsigned int>(value[offset_start + 5] - '0');
  }

  // Determine whether the leap-second moment falls on the previous UTC day.
  // Only a "+" offset can shift the UTC date backward; a "-" offset cannot
  // shift it forward while UTC time stays at 23:59, since
  // max(local) + max(offset) = 1439 + 1439 = 2878 < 1440 + 1439
  const auto local_minute_of_day{hour * 60 + minute};
  const auto offset_total_minutes{offset_hour * 60 + offset_minute};
  const bool previous_utc_day{offset_sign == '+' &&
                              local_minute_of_day < offset_total_minutes};

  std::uint8_t utc_month{month};
  std::uint8_t utc_day{day};
  if (previous_utc_day) {
    if (utc_day > 1) {
      utc_day -= 1;
    } else if (utc_month > 1) {
      utc_month -= 1;
      utc_day = max_day_in_month(utc_month, year);
    } else {
      // Going back from year 0000 January 1 would yield year -1
      if (year == 0) {
        return false;
      }
      utc_month = 12;
      utc_day = 31;
    }
  }

  return (utc_month == 6 && utc_day == 30) ||
         (utc_month == 12 && utc_day == 31);
}

} // namespace sourcemeta::core
