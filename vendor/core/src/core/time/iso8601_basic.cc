#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <charconv>    // std::from_chars
#include <chrono>      // std::chrono::system_clock
#include <cstddef>     // std::size_t
#include <ctime>       // std::tm
#include <iomanip>     // std::put_time
#include <locale>      // std::locale
#include <optional>    // std::optional, std::nullopt
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
constexpr auto FORMAT_ISO8601_BASIC{"%Y%m%dT%H%M%SZ"};

auto all_digits(const std::string_view value, const std::size_t offset,
                const std::size_t length) -> bool {
  for (std::size_t index = offset; index < offset + length; ++index) {
    // A locale-independent ASCII check, matching what from_chars accepts
    if (value[index] < '0' || value[index] > '9') {
      return false;
    }
  }

  return true;
}

auto parse_field(const std::string_view value, const std::size_t offset,
                 const std::size_t length) -> int {
  int result = 0;
  std::from_chars(value.data() + offset, value.data() + offset + length,
                  result);
  return result;
}
} // namespace

namespace sourcemeta::core {

auto to_iso8601_basic(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::put_time(&parts, FORMAT_ISO8601_BASIC);
  return stream.str();
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
  if (!all_digits(value, 0, 8) || !all_digits(value, 9, 6)) {
    return std::nullopt;
  }

  std::tm parts = {};
  parts.tm_year = parse_field(value, 0, 4) - 1900;
  parts.tm_mon = parse_field(value, 4, 2) - 1;
  parts.tm_mday = parse_field(value, 6, 2);
  parts.tm_hour = parse_field(value, 9, 2);
  parts.tm_min = parse_field(value, 11, 2);
  parts.tm_sec = parse_field(value, 13, 2);
  parts.tm_isdst = 0;
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }
  return broken_down_time_to_time_point(parts);
}

} // namespace sourcemeta::core
