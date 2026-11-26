#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <chrono>      // std::chrono::system_clock
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto to_asctime(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::string result;
  result.reserve(24);
  result.append(DAY_NAMES[static_cast<std::size_t>(parts.weekday)]);
  result.push_back(' ');
  result.append(MONTH_NAMES[static_cast<std::size_t>(parts.month - 1)]);
  result.push_back(' ');
  append_space_padded_day(result, parts.day);
  result.push_back(' ');
  append_two_digits(result, parts.hour);
  result.push_back(':');
  append_two_digits(result, parts.minute);
  result.push_back(':');
  append_two_digits(result, parts.second);
  result.push_back(' ');
  append_four_digit_year(result, parts.year);
  return result;
}

auto from_asctime(const std::string_view value)
    -> std::optional<std::chrono::system_clock::time_point> {
  if (value.size() != 24) {
    return std::nullopt;
  }
  if (value[3] != ' ' || value[7] != ' ' || value[10] != ' ' ||
      value[13] != ':' || value[16] != ':' || value[19] != ' ') {
    return std::nullopt;
  }
  if ((value[8] != ' ' && !is_digit(value[8])) || !is_digit(value[9]) ||
      !is_digit(value.substr(11, 2)) || !is_digit(value.substr(14, 2)) ||
      !is_digit(value.substr(17, 2)) || !is_digit(value.substr(20, 4))) {
    return std::nullopt;
  }
  if (!is_day_name(value.substr(0, 3))) {
    return std::nullopt;
  }
  const auto month{month_from_name(value.substr(4, 3))};
  if (!month.has_value()) {
    return std::nullopt;
  }

  BrokenDownTime parts{};
  parts.month = month.value();
  // RFC 9110 §5.6.7: date3 = month SP ( 2DIGIT / ( SP 1DIGIT ) ), so a day
  // below the tenth is written against a space rather than a zero
  if (value[8] == ' ') {
    parts.day = static_cast<std::uint8_t>(value[9] - '0');
  } else {
    parts.day = read_two_digits(value, 8);
  }
  parts.hour = read_two_digits(value, 11);
  parts.minute = read_two_digits(value, 14);
  parts.second = read_two_digits(value, 17);
  parts.year = read_four_digits(value, 20);
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }

  return broken_down_time_to_time_point(parts);
}

} // namespace sourcemeta::core
