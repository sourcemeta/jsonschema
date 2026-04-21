#ifndef SOURCEMETA_CORE_TIME_H_
#define SOURCEMETA_CORE_TIME_H_

#ifndef SOURCEMETA_CORE_TIME_EXPORT
#include <sourcemeta/core/time_export.h>
#endif

#include <chrono>      // std::chrono::system_clock::time_point
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
auto from_gmt(const std::string &time) -> std::chrono::system_clock::time_point;

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

} // namespace sourcemeta::core

#endif
