#ifndef SOURCEMETA_CORE_TIME_H_
#define SOURCEMETA_CORE_TIME_H_

#ifndef SOURCEMETA_CORE_TIME_EXPORT
#include <sourcemeta/core/time_export.h>
#endif

#include <chrono> // std::chrono::system_clock::time_point
#include <string> // std::string

/// @defgroup time Time
/// @brief A growing implementation of time-related utilities for standard such
/// as RFC 7231 (GMT).
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

} // namespace sourcemeta::core

#endif
