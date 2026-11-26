#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <chrono>      // std::chrono::system_clock
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto to_iso8601_basic(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::string result;
  result.reserve(16);
  append_four_digit_year(result, parts.year);
  append_two_digits(result, parts.month);
  append_two_digits(result, parts.day);
  result.push_back('T');
  append_two_digits(result, parts.hour);
  append_two_digits(result, parts.minute);
  append_two_digits(result, parts.second);
  result.push_back('Z');
  return result;
}

auto from_iso8601_basic(const std::string_view value) noexcept
    -> std::optional<std::chrono::system_clock::time_point> {
  // Basic format is fixed width with a literal date-time separator and zone
  // designator (ISO 8601-1 §5.4.2.1), so the shape is validated by position
  if (value.size() != 16) {
    return std::nullopt;
  }
  if (value[8] != 'T' || value[15] != 'Z') {
    return std::nullopt;
  }
  if (!is_digit(value.substr(0, 8)) || !is_digit(value.substr(9, 6))) {
    return std::nullopt;
  }

  BrokenDownTime parts{};
  parts.year = read_four_digits(value, 0);
  parts.month = read_two_digits(value, 4);
  parts.day = read_two_digits(value, 6);
  parts.hour = read_two_digits(value, 9);
  parts.minute = read_two_digits(value, 11);
  parts.second = read_two_digits(value, 13);
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }

  return broken_down_time_to_time_point(parts);
}

} // namespace sourcemeta::core
