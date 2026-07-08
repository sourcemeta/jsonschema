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
constexpr auto FORMAT_IMF_FIXDATE{"%a, %d %b %Y %H:%M:%S GMT"};
}

namespace sourcemeta::core {

auto to_imf_fixdate(const std::chrono::system_clock::time_point time)
    -> std::string {
  const auto parts{time_point_to_broken_down(time)};
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::put_time(&parts, FORMAT_IMF_FIXDATE);
  return stream.str();
}

auto from_imf_fixdate(const std::string_view value) noexcept
    -> std::optional<std::chrono::system_clock::time_point> try {
  if (value.size() != 29) {
    return std::nullopt;
  }
  if (value[3] != ',' || value[4] != ' ' || value[7] != ' ' ||
      value[11] != ' ' || value[16] != ' ' || value[19] != ':' ||
      value[22] != ':' || value[25] != ' ' || value.substr(26) != "GMT") {
    return std::nullopt;
  }
  if (!std::isdigit(static_cast<unsigned char>(value[5])) ||
      !std::isdigit(static_cast<unsigned char>(value[6])) ||
      !std::isdigit(static_cast<unsigned char>(value[12])) ||
      !std::isdigit(static_cast<unsigned char>(value[13])) ||
      !std::isdigit(static_cast<unsigned char>(value[14])) ||
      !std::isdigit(static_cast<unsigned char>(value[15])) ||
      !std::isdigit(static_cast<unsigned char>(value[17])) ||
      !std::isdigit(static_cast<unsigned char>(value[18])) ||
      !std::isdigit(static_cast<unsigned char>(value[20])) ||
      !std::isdigit(static_cast<unsigned char>(value[21])) ||
      !std::isdigit(static_cast<unsigned char>(value[23])) ||
      !std::isdigit(static_cast<unsigned char>(value[24]))) {
    return std::nullopt;
  }
  std::istringstream stream{std::string{value}};
  stream.imbue(std::locale::classic());
  std::tm parts = {};
  stream >> std::get_time(&parts, FORMAT_IMF_FIXDATE);
  if (stream.fail()) {
    return std::nullopt;
  }
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }
  // RFC 9110 §5.6.7: HTTP-date is case sensitive
  if (!is_case_sensitive_day_abbreviation(value.substr(0, 3), parts.tm_wday) ||
      !is_case_sensitive_month_abbreviation(value.substr(8, 3), parts.tm_mon)) {
    return std::nullopt;
  }
  return broken_down_time_to_time_point(parts);
} catch (...) {
  return std::nullopt;
}

} // namespace sourcemeta::core
