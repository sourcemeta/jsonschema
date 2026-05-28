#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/time.h>

#include <cstdint> // std::uint8_t, std::uint16_t

namespace sourcemeta::core {

auto is_rfc3339_fulldate(const std::string_view value) -> bool {
  // full-date = date-fullyear "-" date-month "-" date-mday
  // Exactly 10 characters: "YYYY-MM-DD"
  if (value.size() != 10) {
    return false;
  }

  // date-fullyear = 4DIGIT
  if (!is_digit(value[0]) || !is_digit(value[1]) || !is_digit(value[2]) ||
      !is_digit(value[3])) {
    return false;
  }
  const auto year{static_cast<std::uint16_t>(
      (value[0] - '0') * 1000 + (value[1] - '0') * 100 + (value[2] - '0') * 10 +
      (value[3] - '0'))};

  if (value[4] != '-') {
    return false;
  }

  // date-month = 2DIGIT ; 01-12
  if (!is_digit(value[5]) || !is_digit(value[6])) {
    return false;
  }
  const auto month{
      static_cast<std::uint8_t>((value[5] - '0') * 10 + (value[6] - '0'))};
  if (month < 1 || month > 12) {
    return false;
  }

  if (value[7] != '-') {
    return false;
  }

  // date-mday = 2DIGIT ; 01-28/29/30/31 based on month/year
  if (!is_digit(value[8]) || !is_digit(value[9])) {
    return false;
  }
  const auto day{
      static_cast<std::uint8_t>((value[8] - '0') * 10 + (value[9] - '0'))};
  if (day < 1 || day > max_day_in_month(month, year)) {
    return false;
  }

  return true;
}

} // namespace sourcemeta::core
