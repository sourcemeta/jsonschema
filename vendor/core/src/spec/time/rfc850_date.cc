#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <chrono>      // std::chrono::system_clock
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int32_t, std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

auto current_utc_year() noexcept -> std::int32_t {
  const auto now{std::chrono::system_clock::now()};
  return sourcemeta::core::time_point_to_broken_down(now).year;
}

} // namespace

namespace sourcemeta::core {

auto to_rfc850_date(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::string result;
  result.reserve(33);
  result.append(LONG_DAY_NAMES[static_cast<std::size_t>(parts.weekday)]);
  result.append(", ");
  append_two_digits(result, parts.day);
  result.push_back('-');
  result.append(MONTH_NAMES[static_cast<std::size_t>(parts.month - 1)]);
  result.push_back('-');
  // RFC 9110 §5.6.7 carries the year as its last two digits. The remainder can
  // be negative for a year before year zero, which this format has no spelling
  // for, so it is lifted into the range of the field rather than being left to
  // carry a sign into it
  append_two_digits(
      result, static_cast<std::uint8_t>(((parts.year % 100) + 100) % 100));
  result.push_back(' ');
  append_two_digits(result, parts.hour);
  result.push_back(':');
  append_two_digits(result, parts.minute);
  result.push_back(':');
  append_two_digits(result, parts.second);
  result.append(" GMT");
  return result;
}

auto from_rfc850_date(const std::string_view value)
    -> std::optional<std::chrono::system_clock::time_point> {
  const auto comma_position{value.find(',')};
  if (comma_position == std::string_view::npos) {
    return std::nullopt;
  }
  if (!is_long_day_name(value.substr(0, comma_position))) {
    return std::nullopt;
  }
  const auto rest{value.substr(comma_position + 1)};
  if (rest.size() != 23 || rest[0] != ' ' || rest[3] != '-' || rest[7] != '-' ||
      rest[10] != ' ' || rest[13] != ':' || rest[16] != ':' ||
      rest.substr(19) != " GMT") {
    return std::nullopt;
  }
  if (!is_digit(rest.substr(1, 2)) || !is_digit(rest.substr(8, 2)) ||
      !is_digit(rest.substr(11, 2)) || !is_digit(rest.substr(14, 2)) ||
      !is_digit(rest.substr(17, 2))) {
    return std::nullopt;
  }
  const auto month{month_from_name(rest.substr(4, 3))};
  if (!month.has_value()) {
    return std::nullopt;
  }

  // RFC 9110 §5.6.7: a timestamp that appears to be more than fifty years in
  // the future stands for the most recent past year ending in the same two
  // digits
  const auto current_year{current_utc_year()};
  const std::int32_t current_century{(current_year / 100) * 100};
  std::int32_t full_year{current_century + read_two_digits(rest, 8)};
  if (full_year > current_year + 50) {
    full_year -= 100;
  }

  BrokenDownTime parts{};
  parts.year = full_year;
  parts.month = month.value();
  parts.day = read_two_digits(rest, 1);
  parts.hour = read_two_digits(rest, 11);
  parts.minute = read_two_digits(rest, 14);
  parts.second = read_two_digits(rest, 17);
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }

  return broken_down_time_to_time_point(parts);
}

} // namespace sourcemeta::core
