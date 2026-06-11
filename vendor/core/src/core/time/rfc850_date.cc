#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <algorithm>   // std::ranges::find
#include <array>       // std::array
#include <cassert>     // assert
#include <cctype>      // std::isdigit
#include <chrono>      // std::chrono::system_clock
#include <ctime>       // std::time_t, std::tm, timegm, gmtime_r, gmtime_s
#include <iomanip>     // std::put_time, std::get_time
#include <locale>      // std::locale
#include <optional>    // std::optional, std::nullopt
#include <sstream>     // std::ostringstream, std::istringstream
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string, std::to_string
#include <string_view> // std::string_view

namespace {

constexpr auto FORMAT_RFC850_OUTPUT{"%A, %d-%b-%y %H:%M:%S GMT"};
constexpr auto FORMAT_RFC850_NORMALISED_INPUT{"%d-%b-%Y %H:%M:%S GMT"};

constexpr std::array<std::string_view, 7> RFC850_DAY_NAMES{
    {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
     "Sunday"}};

auto current_utc_year() noexcept -> std::optional<int> {
  const auto now{std::chrono::system_clock::now()};
  const std::time_t ctime{std::chrono::system_clock::to_time_t(now)};
  std::tm buffer;
#if defined(_MSC_VER)
  if (gmtime_s(&buffer, &ctime) != 0) {
    return std::nullopt;
  }
#else
  if (gmtime_r(&ctime, &buffer) == nullptr) {
    return std::nullopt;
  }
#endif
  return buffer.tm_year + 1900;
}

} // namespace

namespace sourcemeta::core {

auto to_rfc850_date(const std::chrono::system_clock::time_point time)
    -> std::string {
  const std::time_t ctime = std::chrono::system_clock::to_time_t(time);
  std::tm buffer;
#if defined(_MSC_VER)
  if (gmtime_s(&buffer, &ctime) != 0) {
    throw std::runtime_error("Could not convert time point to RFC 850 date");
  }
#else
  if (gmtime_r(&ctime, &buffer) == nullptr) {
    throw std::runtime_error("Could not convert time point to RFC 850 date");
  }
#endif
  std::tm *parts = &buffer;
  assert(parts);
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::put_time(parts, FORMAT_RFC850_OUTPUT);
  return stream.str();
}

auto from_rfc850_date(const std::string_view value) noexcept
    -> std::optional<std::chrono::system_clock::time_point> try {
  const auto comma_position{value.find(',')};
  if (comma_position == std::string_view::npos) {
    return std::nullopt;
  }
  const auto day_name{value.substr(0, comma_position)};
  if (std::ranges::find(RFC850_DAY_NAMES, day_name) == RFC850_DAY_NAMES.end()) {
    return std::nullopt;
  }
  const auto rest{value.substr(comma_position + 1)};
  if (rest.size() != 23 || rest[0] != ' ' || rest[3] != '-' || rest[7] != '-' ||
      rest[10] != ' ' || rest.substr(19) != " GMT") {
    return std::nullopt;
  }
  if (!std::isdigit(static_cast<unsigned char>(rest[1])) ||
      !std::isdigit(static_cast<unsigned char>(rest[2])) ||
      !std::isdigit(static_cast<unsigned char>(rest[8])) ||
      !std::isdigit(static_cast<unsigned char>(rest[9])) ||
      !std::isdigit(static_cast<unsigned char>(rest[11])) ||
      !std::isdigit(static_cast<unsigned char>(rest[12])) ||
      !std::isdigit(static_cast<unsigned char>(rest[14])) ||
      !std::isdigit(static_cast<unsigned char>(rest[15])) ||
      !std::isdigit(static_cast<unsigned char>(rest[17])) ||
      !std::isdigit(static_cast<unsigned char>(rest[18]))) {
    return std::nullopt;
  }
  const int two_digit_year{(rest[8] - '0') * 10 + (rest[9] - '0')};
  const auto current_year{current_utc_year()};
  if (!current_year.has_value()) {
    return std::nullopt;
  }
  const int current_century{(current_year.value() / 100) * 100};
  int full_year{current_century + two_digit_year};
  if (full_year > current_year.value() + 50) {
    full_year -= 100;
  }
  std::string normalised;
  normalised.reserve(24);
  normalised.append(rest.substr(1, 7));
  normalised.append(std::to_string(full_year));
  normalised.append(rest.substr(10));
  std::istringstream stream{normalised};
  stream.imbue(std::locale::classic());
  std::tm parts = {};
  stream >> std::get_time(&parts, FORMAT_RFC850_NORMALISED_INPUT);
  if (stream.fail()) {
    return std::nullopt;
  }
  if (!is_valid_broken_down_time(parts)) {
    return std::nullopt;
  }
#if defined(_MSC_VER)
  return std::chrono::system_clock::from_time_t(_mkgmtime(&parts));
#else
  return std::chrono::system_clock::from_time_t(timegm(&parts));
#endif
} catch (...) {
  return std::nullopt;
}

} // namespace sourcemeta::core
