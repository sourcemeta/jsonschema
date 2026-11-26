#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <chrono>      // std::chrono::system_clock
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto to_imf_fixdate(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::string result;
  result.reserve(29);
  result.append(DAY_NAMES[static_cast<std::size_t>(parts.weekday)]);
  result.append(", ");
  append_two_digits(result, parts.day);
  result.push_back(' ');
  result.append(MONTH_NAMES[static_cast<std::size_t>(parts.month - 1)]);
  result.push_back(' ');
  append_four_digit_year(result, parts.year);
  result.push_back(' ');
  append_two_digits(result, parts.hour);
  result.push_back(':');
  append_two_digits(result, parts.minute);
  result.push_back(':');
  append_two_digits(result, parts.second);
  result.append(" GMT");
  return result;
}

auto from_imf_fixdate(const std::string_view value)
    -> std::optional<std::chrono::system_clock::time_point> {
  if (value.size() != 29) {
    return std::nullopt;
  }
  if (value[3] != ',' || value[4] != ' ' || value[7] != ' ' ||
      value[11] != ' ' || value[16] != ' ' || value[19] != ':' ||
      value[22] != ':' || value[25] != ' ' || value.substr(26) != "GMT") {
    return std::nullopt;
  }
  if (!is_digit(value.substr(5, 2)) || !is_digit(value.substr(12, 4)) ||
      !is_digit(value.substr(17, 2)) || !is_digit(value.substr(20, 2)) ||
      !is_digit(value.substr(23, 2))) {
    return std::nullopt;
  }
  if (!is_day_name(value.substr(0, 3))) {
    return std::nullopt;
  }
  const auto month{month_from_name(value.substr(8, 3))};
  if (!month.has_value()) {
    return std::nullopt;
  }

  BrokenDownTime parts{};
  parts.month = month.value();
  parts.day = read_two_digits(value, 5);
  parts.year = read_four_digits(value, 12);
  parts.hour = read_two_digits(value, 17);
  parts.minute = read_two_digits(value, 20);
  parts.second = read_two_digits(value, 23);
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }

  return broken_down_time_to_time_point(parts);
}

} // namespace sourcemeta::core
