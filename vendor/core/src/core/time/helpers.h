#ifndef SOURCEMETA_CORE_TIME_HELPERS_H_
#define SOURCEMETA_CORE_TIME_HELPERS_H_

#include <sourcemeta/core/time.h>

#include <array>       // std::array
#include <chrono>      // std::chrono::system_clock, std::chrono::sys_days
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <ctime>       // std::tm
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string, std::to_string
#include <string_view> // std::string_view
#include <utility>     // std::cmp_greater

namespace sourcemeta::core {

// Convert an already-validated broken-down UTC time to a time point through
// calendar arithmetic rather than timegm, which on a 32-bit time_t cannot
// represent pre-1970 or far-future dates and signals failure with a
// (time_t)-1 that is indistinguishable from a real 1969-12-31T23:59:59Z. A date
// whose seconds since the epoch fall outside what system_clock can represent
// yields no result rather than silently wrapping during the cast to its finer
// duration
inline auto broken_down_time_to_time_point(const std::tm &parts)
    -> std::optional<std::chrono::system_clock::time_point> {
  const std::chrono::year_month_day date{
      std::chrono::year{parts.tm_year + 1900},
      std::chrono::month{static_cast<unsigned>(parts.tm_mon + 1)},
      std::chrono::day{static_cast<unsigned>(parts.tm_mday)}};
  const auto since_epoch{std::chrono::sys_days{date}.time_since_epoch() +
                         std::chrono::hours{parts.tm_hour} +
                         std::chrono::minutes{parts.tm_min} +
                         std::chrono::seconds{parts.tm_sec}};

  using TimePoint = std::chrono::system_clock::time_point;
  // The bounds round inwards so the value cast back to the finer duration stays
  // within range
  constexpr auto MINIMUM{std::chrono::ceil<std::chrono::seconds>(
      TimePoint::min().time_since_epoch())};
  constexpr auto MAXIMUM{std::chrono::floor<std::chrono::seconds>(
      TimePoint::max().time_since_epoch())};
  if (since_epoch < MINIMUM || since_epoch > MAXIMUM) {
    return std::nullopt;
  }

  return TimePoint{
      std::chrono::duration_cast<TimePoint::duration>(since_epoch)};
}

// Break a time point down into UTC calendar fields through calendar arithmetic
// rather than gmtime, which on some platforms cannot represent a pre-1970 time
// and computes the wrong weekday for the instant just before the epoch
inline auto time_point_to_broken_down(
    const std::chrono::system_clock::time_point time) noexcept -> std::tm {
  const auto seconds{
      std::chrono::floor<std::chrono::seconds>(time.time_since_epoch())};
  const auto days{std::chrono::floor<std::chrono::days>(seconds)};
  const std::chrono::year_month_day date{std::chrono::sys_days{days}};
  const std::chrono::hh_mm_ss clock{seconds - days};
  const std::chrono::weekday weekday{std::chrono::sys_days{days}};

  std::tm parts{};
  parts.tm_year = static_cast<int>(date.year()) - 1900;
  parts.tm_mon = static_cast<int>(static_cast<unsigned>(date.month())) - 1;
  parts.tm_mday = static_cast<int>(static_cast<unsigned>(date.day()));
  parts.tm_hour = static_cast<int>(clock.hours().count());
  parts.tm_min = static_cast<int>(clock.minutes().count());
  parts.tm_sec = static_cast<int>(clock.seconds().count());
  parts.tm_wday = static_cast<int>(weekday.c_encoding());
  return parts;
}

// RFC 9110 §5.6.7 defines the calendar year as "year = 4DIGIT", a fixed width
// that asctime and the ISO 8601 basic format share, yet rendering the year
// through the standard library omits its leading zeros for a year below 1000 on
// several platforms, which the parsers here then reject. The field is built
// explicitly so a representable year is always at least four digits. The year
// is expected to be non-negative, as every caller derives it from a broken-down
// time whose year is offset from 1900
inline auto format_four_digit_year(const int year) -> std::string {
  std::string digits{std::to_string(year)};
  if (digits.size() < 4) {
    digits.insert(std::size_t{0}, 4 - digits.size(), '0');
  }

  return digits;
}

// RFC 9110 §5.6.7: "HTTP-date is case sensitive". The standard library's
// std::get_time matches day and month names case-insensitively on some
// implementations, so the exact spelling is verified against the parsed index
inline auto is_case_sensitive_day_abbreviation(const std::string_view name,
                                               const int weekday) -> bool {
  static constexpr std::array<std::string_view, 7> NAMES{
      {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}};
  return weekday >= 0 && weekday < 7 &&
         name == NAMES[static_cast<std::size_t>(weekday)];
}

inline auto is_case_sensitive_month_abbreviation(const std::string_view name,
                                                 const int month) -> bool {
  static constexpr std::array<std::string_view, 12> NAMES{
      {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
       "Nov", "Dec"}};
  return month >= 0 && month < 12 &&
         name == NAMES[static_cast<std::size_t>(month)];
}

// Validate the calendar fields of a broken-down time, since the conversion to
// an epoch time point would otherwise silently normalise out-of-range values
inline auto is_valid_broken_down_time(const std::tm &parts) -> bool {
  if (parts.tm_mon < 0 || parts.tm_mon > 11) {
    return false;
  }
  const auto month{static_cast<std::uint8_t>(parts.tm_mon + 1)};
  const auto year{static_cast<std::uint16_t>(parts.tm_year + 1900)};
  if (parts.tm_mday < 1 ||
      std::cmp_greater(parts.tm_mday, max_day_in_month(month, year))) {
    return false;
  }
  if (parts.tm_hour < 0 || parts.tm_hour > 23) {
    return false;
  }
  if (parts.tm_min < 0 || parts.tm_min > 59) {
    return false;
  }
  // RFC 3339 §5.6: a leap second is represented as a "60" second value
  if (parts.tm_sec < 0 || parts.tm_sec > 60) {
    return false;
  }
  return true;
}

} // namespace sourcemeta::core

#endif
