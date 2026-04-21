#include <sourcemeta/core/time.h>

#include <array> // std::array

namespace sourcemeta::core {

static constexpr auto is_digit(const char character) -> bool {
  return character >= '0' && character <= '9';
}

static constexpr auto is_leap_year(const unsigned int year) -> bool {
  return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

static constexpr auto max_day_in_month(const unsigned int month,
                                       const unsigned int year)
    -> unsigned int {
  constexpr std::array<unsigned int, 13> days{
      {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
  if (month == 2 && is_leap_year(year)) {
    return 29;
  }
  return days[month];
}

auto is_rfc3339_datetime(const std::string_view value) -> bool {
  const auto size{value.size()};

  // Minimum valid date-time: "YYYY-MM-DDTHH:MM:SSZ" = 20 characters
  if (size < 20) {
    return false;
  }

  std::string_view::size_type position{0};

  // --- full-date: date-fullyear "-" date-month "-" date-mday ---

  // date-fullyear = 4DIGIT
  if (!is_digit(value[0]) || !is_digit(value[1]) || !is_digit(value[2]) ||
      !is_digit(value[3])) {
    return false;
  }
  const auto year{static_cast<unsigned int>(value[0] - '0') * 1000 +
                  static_cast<unsigned int>(value[1] - '0') * 100 +
                  static_cast<unsigned int>(value[2] - '0') * 10 +
                  static_cast<unsigned int>(value[3] - '0')};
  position = 4;

  // "-"
  if (value[position] != '-') {
    return false;
  }
  position += 1;

  // date-month = 2DIGIT ; 01-12
  if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
    return false;
  }
  const auto month{static_cast<unsigned int>(value[position] - '0') * 10 +
                   static_cast<unsigned int>(value[position + 1] - '0')};
  if (month < 1 || month > 12) {
    return false;
  }
  position += 2;

  // "-"
  if (value[position] != '-') {
    return false;
  }
  position += 1;

  // date-mday = 2DIGIT ; 01-28/29/30/31 based on month/year
  if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
    return false;
  }
  const auto day{static_cast<unsigned int>(value[position] - '0') * 10 +
                 static_cast<unsigned int>(value[position + 1] - '0')};
  position += 2;

  // --- "T" or "t" separator ---
  if (value[position] != 'T' && value[position] != 't') {
    return false;
  }
  position += 1;

  // --- partial-time: time-hour ":" time-minute ":" time-second ---

  // time-hour = 2DIGIT ; 00-23
  if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
    return false;
  }
  const auto hour{static_cast<unsigned int>(value[position] - '0') * 10 +
                  static_cast<unsigned int>(value[position + 1] - '0')};
  if (hour > 23) {
    return false;
  }
  position += 2;

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
    // No time offset present — invalid
    return false;
  }

  if (value[position] == 'Z' || value[position] == 'z') {
    position += 1;
  } else if (value[position] == '+' || value[position] == '-') {
    position += 1;

    // time-numoffset = ("+" / "-") time-hour ":" time-minute
    if (position + 5 > size) {
      return false;
    }

    // Offset time-hour = 2DIGIT ; 00-23
    if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
      return false;
    }
    const auto offset_hour{
        static_cast<unsigned int>(value[position] - '0') * 10 +
        static_cast<unsigned int>(value[position + 1] - '0')};
    if (offset_hour > 23) {
      return false;
    }
    position += 2;

    // ":" — REQUIRED (colonless offsets like +0530 are invalid per §5.6)
    if (value[position] != ':') {
      return false;
    }
    position += 1;

    // Offset time-minute = 2DIGIT ; 00-59
    if (!is_digit(value[position]) || !is_digit(value[position + 1])) {
      return false;
    }
    const auto offset_minute{
        static_cast<unsigned int>(value[position] - '0') * 10 +
        static_cast<unsigned int>(value[position + 1] - '0')};
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

  // --- Validate date-mday against month/year (§5.7) ---
  if (day < 1 || day > max_day_in_month(month, year)) {
    return false;
  }

  return true;
}

} // namespace sourcemeta::core
