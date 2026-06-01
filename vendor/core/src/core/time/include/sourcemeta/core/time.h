#ifndef SOURCEMETA_CORE_TIME_H_
#define SOURCEMETA_CORE_TIME_H_

#ifndef SOURCEMETA_CORE_TIME_EXPORT
#include <sourcemeta/core/time_export.h>
#endif

#include <array>       // std::array
#include <cassert>     // assert
#include <chrono>      // std::chrono::system_clock::time_point
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup time Time
/// @brief A growing implementation of time-related utilities for standards
/// such as RFC 7231 (GMT) and RFC 3339 (Internet Date/Time Format).
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
/// ```

namespace sourcemeta::core {

/// @ingroup time
/// Convert a time point into a GMT string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <chrono>
/// #include <ctime>
/// #include <cassert>
///
/// std::tm parts = {};
/// parts.tm_year = 115;
/// parts.tm_mon = 9;
/// parts.tm_mday = 21;
/// parts.tm_hour = 11;
/// parts.tm_min = 28;
/// parts.tm_sec = 0;
/// parts.tm_isdst = 0;
///
/// const auto point{std::chrono::system_clock::from_time_t(timegm(&parts))};
///
/// assert(sourcemeta::core::to_gmt(point) ==
///   "Wed, 21 Oct 2015 11:28:00 GMT");
/// ```
///
/// On Windows, you might need to use `_mkgmtime` instead of `timegm`.
SOURCEMETA_CORE_TIME_EXPORT
auto to_gmt(const std::chrono::system_clock::time_point time) -> std::string;

/// @ingroup time
/// Parse a GMT string into a time point. For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <chrono>
/// #include <ctime>
/// #include <cassert>
///
/// const auto point{
///     sourcemeta::core::from_gmt("Wed, 21 Oct 2015 11:28:00 GMT")};
///
/// std::tm parts = {};
/// parts.tm_year = 115;
/// parts.tm_mon = 9;
/// parts.tm_mday = 21;
/// parts.tm_hour = 11;
/// parts.tm_min = 28;
/// parts.tm_sec = 0;
/// parts.tm_isdst = 0;
/// const auto expected{std::chrono::system_clock::from_time_t(timegm(&parts))};
///
/// assert(point = expected);
/// ```
///
/// On Windows, you might need to use `_mkgmtime` instead of `timegm`.
SOURCEMETA_CORE_TIME_EXPORT
auto from_gmt(const std::string_view time)
    -> std::chrono::system_clock::time_point;

/// @ingroup time
/// Check whether the given string is a valid date-time value per RFC 3339
/// Section 5.6 (Internet Date/Time Format). This implements the full
/// `date-time` production rule:
///
/// ```
/// date-time = full-date "T" full-time
/// ```
///
/// where "T" may also be lowercase "t" (per RFC 3339 §5.6 NOTE). For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_rfc3339_datetime("1985-04-12T23:20:50.52Z"));
/// assert(sourcemeta::core::is_rfc3339_datetime("1996-12-19T16:39:57-08:00"));
/// assert(sourcemeta::core::is_rfc3339_datetime("1990-12-31T23:59:60Z"));
/// assert(!sourcemeta::core::is_rfc3339_datetime("2024-01-15T14:30:00"));
/// assert(!sourcemeta::core::is_rfc3339_datetime("2024-01-15 14:30:00Z"));
/// ```
SOURCEMETA_CORE_TIME_EXPORT
auto is_rfc3339_datetime(const std::string_view value) -> bool;

/// @ingroup time
/// Check whether the given string is a valid full-date value per RFC 3339
/// Section 5.6 (Internet Date/Time Format). This implements the `full-date`
/// production rule:
///
/// ```
/// full-date = date-fullyear "-" date-month "-" date-mday
/// ```
///
/// with the day-of-month and leap year restrictions from RFC 3339 §5.7 and
/// Appendix C. For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_rfc3339_fulldate("2024-01-15"));
/// assert(sourcemeta::core::is_rfc3339_fulldate("2000-02-29"));
/// assert(!sourcemeta::core::is_rfc3339_fulldate("2021-02-29"));
/// assert(!sourcemeta::core::is_rfc3339_fulldate("2024-01-15T00:00:00Z"));
/// ```
SOURCEMETA_CORE_TIME_EXPORT
auto is_rfc3339_fulldate(const std::string_view value) -> bool;

/// @ingroup time
/// Check whether the given string is a valid full-time value per RFC 3339
/// Section 5.6 (Internet Date/Time Format). This implements the `full-time`
/// production rule:
///
/// ```
/// full-time = partial-time time-offset
/// ```
///
/// where "Z" may also be lowercase "z" (per RFC 3339 §5.6 NOTE). When
/// `time-second` is `60` (leap second), only the UTC `23:59` alignment is
/// verified; the §5.7 calendar restriction to June 30 or December 31
/// requires date context and is therefore out of scope. For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_rfc3339_fulltime("23:20:50.52Z"));
/// assert(sourcemeta::core::is_rfc3339_fulltime("16:39:57-08:00"));
/// assert(sourcemeta::core::is_rfc3339_fulltime("23:59:60Z"));
/// assert(!sourcemeta::core::is_rfc3339_fulltime("14:30:00"));
/// ```
SOURCEMETA_CORE_TIME_EXPORT
auto is_rfc3339_fulltime(const std::string_view value) -> bool;

/// @ingroup time
/// Check whether the given string is a valid partial-time value per RFC 3339
/// Section 5.6 (Internet Date/Time Format), excluding the optional
/// fractional seconds component. This implements the `partial-time`
/// production rule without `[time-secfrac]`:
///
/// ```
/// partial-time = time-hour ":" time-minute ":" time-second
/// ```
///
/// This matches the JSON Schema Draft 3 `time` format (`hh:mm:ss`). For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_rfc3339_partialtime_no_secfrac("08:30:06"));
/// assert(sourcemeta::core::is_rfc3339_partialtime_no_secfrac("23:59:60"));
/// assert(!sourcemeta::core::is_rfc3339_partialtime_no_secfrac("08:30:06.5"));
/// assert(!sourcemeta::core::is_rfc3339_partialtime_no_secfrac("08:30:06Z"));
/// assert(!sourcemeta::core::is_rfc3339_partialtime_no_secfrac("8:30 AM"));
/// ```
SOURCEMETA_CORE_TIME_EXPORT
auto is_rfc3339_partialtime_no_secfrac(const std::string_view value) -> bool;

/// @ingroup time
/// Check whether the given string is a valid duration value per RFC 3339
/// Appendix A (ISO 8601 Collected ABNF). This implements the `duration`
/// production rule:
///
/// ```
/// duration = "P" (dur-date / dur-time / dur-week)
/// ```
///
/// where `dur-week` is mutually exclusive with the date and time components.
/// Only ASCII digits and uppercase unit letters are accepted; fractional
/// values are not permitted by the RFC 3339 ABNF. For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_rfc3339_duration("P4DT12H30M5S"));
/// assert(sourcemeta::core::is_rfc3339_duration("P1Y2M3D"));
/// assert(sourcemeta::core::is_rfc3339_duration("P2W"));
/// assert(!sourcemeta::core::is_rfc3339_duration("PT1D"));
/// ```
SOURCEMETA_CORE_TIME_EXPORT
auto is_rfc3339_duration(const std::string_view value) -> bool;

/// @ingroup time
/// Check whether the given year is a leap year per the Gregorian calendar
/// (RFC 3339 Appendix C). For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_leap_year(2020));
/// assert(sourcemeta::core::is_leap_year(2000));
/// assert(!sourcemeta::core::is_leap_year(1900));
/// assert(!sourcemeta::core::is_leap_year(2021));
/// ```
inline constexpr auto is_leap_year(const std::uint16_t year) -> bool {
  return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

/// @ingroup time
/// Compute the maximum day-of-month for the given month and year per the
/// Gregorian calendar. The month must be in the range 1-12. For example:
///
/// ```cpp
/// #include <sourcemeta/core/time.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::max_day_in_month(1, 2024) == 31);
/// assert(sourcemeta::core::max_day_in_month(2, 2020) == 29);
/// assert(sourcemeta::core::max_day_in_month(2, 2021) == 28);
/// assert(sourcemeta::core::max_day_in_month(4, 2024) == 30);
/// ```
inline constexpr auto max_day_in_month(const std::uint8_t month,
                                       const std::uint16_t year)
    -> std::uint8_t {
  assert(month >= 1 && month <= 12);
  constexpr std::array<std::uint8_t, 13> days{
      {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
  if (month == 2 && is_leap_year(year)) {
    return 29;
  }
  return days[month];
}

} // namespace sourcemeta::core

#endif
