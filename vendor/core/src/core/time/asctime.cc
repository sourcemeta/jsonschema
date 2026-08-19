#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <cctype>      // std::isdigit
#include <chrono>      // std::chrono::system_clock
#include <ctime>       // std::tm
#include <iomanip>     // std::put_time, std::get_time
#include <locale>      // std::locale
#include <optional>    // std::optional, std::nullopt
#include <sstream>     // std::ostringstream, std::istringstream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
// The year is rendered separately so it is always four digits (RFC 9110
// §5.6.7: "year = 4DIGIT"), which "%Y" does not guarantee for a year below 1000
constexpr auto FORMAT_ASCTIME_OUTPUT_BEFORE_YEAR{"%a %b %e %H:%M:%S "};
constexpr auto FORMAT_ASCTIME_NORMALISED_INPUT{"%a %b %d %H:%M:%S %Y"};
} // namespace

namespace sourcemeta::core {

auto to_asctime(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::put_time(&parts, FORMAT_ASCTIME_OUTPUT_BEFORE_YEAR)
         << format_four_digit_year(parts.tm_year + 1900);
  return stream.str();
}

auto from_asctime(const std::string_view value) noexcept
    -> std::optional<std::chrono::system_clock::time_point> try {
  if (value.size() != 24) {
    return std::nullopt;
  }
  if (value[3] != ' ' || value[7] != ' ' || value[10] != ' ' ||
      value[13] != ':' || value[16] != ':' || value[19] != ' ') {
    return std::nullopt;
  }
  if ((value[8] != ' ' &&
       (std::isdigit(static_cast<unsigned char>(value[8])) == 0)) ||
      (std::isdigit(static_cast<unsigned char>(value[9])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[11])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[12])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[14])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[15])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[17])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[18])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[20])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[21])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[22])) == 0) ||
      (std::isdigit(static_cast<unsigned char>(value[23])) == 0)) {
    return std::nullopt;
  }
  std::string normalised{value};
  if (normalised[8] == ' ') {
    normalised[8] = '0';
  }
  std::istringstream stream{normalised};
  stream.imbue(std::locale::classic());
  std::tm parts = {};
  stream >> std::get_time(&parts, FORMAT_ASCTIME_NORMALISED_INPUT);
  if (stream.fail()) {
    return std::nullopt;
  }
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }
  // RFC 9110 §5.6.7: HTTP-date is case sensitive
  if (!is_case_sensitive_day_abbreviation(value.substr(0, 3), parts.tm_wday) ||
      !is_case_sensitive_month_abbreviation(value.substr(4, 3), parts.tm_mon)) {
    return std::nullopt;
  }
  return broken_down_time_to_time_point(parts);
} catch (...) {
  return std::nullopt;
}

} // namespace sourcemeta::core
