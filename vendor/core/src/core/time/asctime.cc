#include <sourcemeta/core/time.h>

#include "helpers.h"

#include <cassert>     // assert
#include <cctype>      // std::isdigit
#include <chrono>      // std::chrono::system_clock
#include <ctime>       // std::time_t, std::tm, timegm, gmtime_r, gmtime_s
#include <iomanip>     // std::put_time, std::get_time
#include <locale>      // std::locale
#include <optional>    // std::optional, std::nullopt
#include <sstream>     // std::ostringstream, std::istringstream
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {
constexpr auto FORMAT_ASCTIME_OUTPUT{"%a %b %e %H:%M:%S %Y"};
constexpr auto FORMAT_ASCTIME_NORMALISED_INPUT{"%a %b %d %H:%M:%S %Y"};
} // namespace

namespace sourcemeta::core {

auto to_asctime(const std::chrono::system_clock::time_point time)
    -> std::string {
  const std::time_t ctime = std::chrono::system_clock::to_time_t(time);
  std::tm buffer;
#if defined(_MSC_VER)
  if (gmtime_s(&buffer, &ctime) != 0) {
    throw std::runtime_error("Could not convert time point to asctime");
  }
#else
  if (gmtime_r(&ctime, &buffer) == nullptr) {
    throw std::runtime_error("Could not convert time point to asctime");
  }
#endif
  std::tm *parts = &buffer;
  assert(parts);
  std::ostringstream stream;
  stream.imbue(std::locale::classic());
  stream << std::put_time(parts, FORMAT_ASCTIME_OUTPUT);
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
       !std::isdigit(static_cast<unsigned char>(value[8]))) ||
      !std::isdigit(static_cast<unsigned char>(value[9])) ||
      !std::isdigit(static_cast<unsigned char>(value[11])) ||
      !std::isdigit(static_cast<unsigned char>(value[12])) ||
      !std::isdigit(static_cast<unsigned char>(value[14])) ||
      !std::isdigit(static_cast<unsigned char>(value[15])) ||
      !std::isdigit(static_cast<unsigned char>(value[17])) ||
      !std::isdigit(static_cast<unsigned char>(value[18])) ||
      !std::isdigit(static_cast<unsigned char>(value[20])) ||
      !std::isdigit(static_cast<unsigned char>(value[21])) ||
      !std::isdigit(static_cast<unsigned char>(value[22])) ||
      !std::isdigit(static_cast<unsigned char>(value[23]))) {
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
#if defined(_MSC_VER)
  return std::chrono::system_clock::from_time_t(_mkgmtime(&parts));
#else
  return std::chrono::system_clock::from_time_t(timegm(&parts));
#endif
} catch (...) {
  return std::nullopt;
}

} // namespace sourcemeta::core
