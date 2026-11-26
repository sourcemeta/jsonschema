#ifndef SOURCEMETA_CORE_TIME_HELPERS_H_
#define SOURCEMETA_CORE_TIME_HELPERS_H_

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>

#include <algorithm>   // std::ranges::find
#include <array>       // std::array
#include <cassert>     // assert
#include <chrono>      // std::chrono::system_clock, std::chrono::sys_days
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int32_t, std::uint8_t, std::uint16_t
#include <iterator>    // std::ranges::distance
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// The calendar fields of a UTC instant, counted the way the formats here write
// them rather than the way the C library breaks a time down, whose year is
// offset from 1900, whose month is offset from January, and whose field widths
// no standard fixes
struct BrokenDownTime {
  std::int32_t year{0};
  std::uint8_t month{1};
  std::uint8_t day{1};
  std::uint8_t hour{0};
  std::uint8_t minute{0};
  std::uint8_t second{0};
  std::uint8_t weekday{0};
};

// RFC 9110 §5.6.7 spells every day and month name with the RFC 7405 case
// sensitive prefix, so both directions read these tables rather than the time
// conversions of the standard library, which match a name case insensitively
// on some implementations and render one through whichever locale is
// installed. The weeks below start where the calendar starts its own week
// count, so a day name is found at the position the calendar gives it
inline constexpr std::array<std::string_view, 7> DAY_NAMES{
    {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"}};
inline constexpr std::array<std::string_view, 7> LONG_DAY_NAMES{
    {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
     "Saturday"}};
inline constexpr std::array<std::string_view, 12> MONTH_NAMES{
    {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
     "Nov", "Dec"}};

// The day name is redundant with the date that follows it, so it is read for
// its spelling alone
inline auto is_day_name(const std::string_view name) -> bool {
  return std::ranges::find(DAY_NAMES, name) != DAY_NAMES.end();
}

inline auto is_long_day_name(const std::string_view name) -> bool {
  return std::ranges::find(LONG_DAY_NAMES, name) != LONG_DAY_NAMES.end();
}

// The month arrives as a name, so the number the calendar needs is the
// position that name holds, and a spelling the table does not carry stands for
// no month at all
inline auto month_from_name(const std::string_view name)
    -> std::optional<std::uint8_t> {
  const auto position{static_cast<std::size_t>(std::ranges::distance(
      MONTH_NAMES.begin(), std::ranges::find(MONTH_NAMES, name)))};
  if (position == MONTH_NAMES.size()) {
    return std::nullopt;
  }

  return static_cast<std::uint8_t>(position + 1);
}

// Each numeric field is a run of digits settled by position before any of them
// is read, so a field here cannot fail to convert and needs no result saying
// that it did
inline auto read_two_digits(const std::string_view value,
                            const std::size_t offset) -> std::uint8_t {
  assert(is_digit(value[offset]) && is_digit(value[offset + 1]));
  return static_cast<std::uint8_t>(((value[offset] - '0') * 10) +
                                   (value[offset + 1] - '0'));
}

inline auto read_four_digits(const std::string_view value,
                             const std::size_t offset) -> std::uint16_t {
  assert(is_digit(value[offset]) && is_digit(value[offset + 1]) &&
         is_digit(value[offset + 2]) && is_digit(value[offset + 3]));
  return static_cast<std::uint16_t>(
      ((value[offset] - '0') * 1000) + ((value[offset + 1] - '0') * 100) +
      ((value[offset + 2] - '0') * 10) + (value[offset + 3] - '0'));
}

inline auto append_two_digits(std::string &output, const std::uint8_t value)
    -> void {
  assert(value < 100);
  output.push_back(static_cast<char>('0' + (value / 10)));
  output.push_back(static_cast<char>('0' + (value % 10)));
}

// RFC 9110 §5.6.7 defines the calendar year as "year = 4DIGIT", a fixed width
// that the ISO 8601 basic format shares, so a year below 1000 is written
// against leading zeros rather than in the shortest spelling, which the readers
// here would turn down. A year wider than the field is left as it reads, since
// neither format can express one and neither writer can report that it could
// not
inline auto append_four_digit_year(std::string &output, const std::int32_t year)
    -> void {
  std::array<char, DIGITS_CAPACITY<std::int32_t>> buffer;
  const auto digits{digits_view(year, buffer)};
  if (digits.size() < 4) {
    output.append(4 - digits.size(), '0');
  }

  output.append(digits);
}

// RFC 9110 §5.6.7 defines the asctime day as "date3 = month SP ( 2DIGIT / ( SP
// 1DIGIT ))", a space padded field of a fixed width
inline auto append_space_padded_day(std::string &output, const std::uint8_t day)
    -> void {
  assert(day < 100);
  if (day < 10) {
    output.push_back(' ');
  } else {
    output.push_back(static_cast<char>('0' + (day / 10)));
  }

  output.push_back(static_cast<char>('0' + (day % 10)));
}

// Convert an already-validated broken-down UTC time to a time point through
// calendar arithmetic rather than timegm, which on a 32-bit time_t cannot
// represent pre-1970 or far-future dates and signals failure with a
// (time_t)-1 that is indistinguishable from a real 1969-12-31T23:59:59Z. A date
// whose seconds since the epoch fall outside what system_clock can represent
// yields no result rather than silently wrapping during the cast to its finer
// duration
inline auto broken_down_time_to_time_point(const BrokenDownTime &parts)
    -> std::optional<std::chrono::system_clock::time_point> {
  const std::chrono::year_month_day date{
      std::chrono::year{parts.year},
      std::chrono::month{static_cast<unsigned>(parts.month)},
      std::chrono::day{static_cast<unsigned>(parts.day)}};
  // Every term is widened to seconds before any of them are added. A day count
  // and an hour count share a representation as narrow as the narrower of the
  // two, which is 32 bits wide on some implementations, and a far future
  // instant overflows it at minute precision before the bounds below are
  // tested, turning into a past instant that those bounds then admit
  const auto since_epoch{
      std::chrono::seconds{std::chrono::sys_days{date}.time_since_epoch()} +
      std::chrono::seconds{std::chrono::hours{parts.hour}} +
      std::chrono::seconds{std::chrono::minutes{parts.minute}} +
      std::chrono::seconds{parts.second}};

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
    const std::chrono::system_clock::time_point time) noexcept
    -> BrokenDownTime {
  const auto seconds{
      std::chrono::floor<std::chrono::seconds>(time.time_since_epoch())};
  const auto days{std::chrono::floor<std::chrono::days>(seconds)};
  const std::chrono::sys_days day_point{days};
  const std::chrono::year_month_day date{day_point};
  const std::chrono::hh_mm_ss clock{seconds - days};
  const std::chrono::weekday weekday{day_point};

  BrokenDownTime parts{};
  parts.year = static_cast<int>(date.year());
  parts.month = static_cast<std::uint8_t>(static_cast<unsigned>(date.month()));
  parts.day = static_cast<std::uint8_t>(static_cast<unsigned>(date.day()));
  parts.hour = static_cast<std::uint8_t>(clock.hours().count());
  parts.minute = static_cast<std::uint8_t>(clock.minutes().count());
  parts.second = static_cast<std::uint8_t>(clock.seconds().count());
  parts.weekday = static_cast<std::uint8_t>(weekday.c_encoding());
  return parts;
}

// Validate the calendar fields of a broken-down time, since the conversion to
// an epoch time point would otherwise silently normalise out-of-range values
//
// A reader settles the syntax of a field and not its range, so every bound
// below is one a reader can drive. The month and the day are counted from one,
// so a field written "00" arrives here as a zero that has to be turned down,
// while the time of day is counted from zero and has nothing below its first
// admissible value to turn down. A month taken from a name arrives in range
// already, which leaves the month bounds to the reader that takes it as digits
inline auto is_valid_broken_down_time(const BrokenDownTime &parts) -> bool {
  if (parts.month < 1 || parts.month > 12) {
    return false;
  }
  // Every reader writes the year as four digits, so the calendar below holds
  // whatever one of them produces
  assert(parts.year >= 0 && parts.year <= 9999);
  const auto year{static_cast<std::uint16_t>(parts.year)};
  if (parts.day < 1 || parts.day > max_day_in_month(parts.month, year)) {
    return false;
  }
  if (parts.hour > 23) {
    return false;
  }
  if (parts.minute > 59) {
    return false;
  }
  // RFC 3339 §5.6: a leap second is represented as a "60" second value
  if (parts.second > 60) {
    return false;
  }
  return true;
}

} // namespace sourcemeta::core

#endif
