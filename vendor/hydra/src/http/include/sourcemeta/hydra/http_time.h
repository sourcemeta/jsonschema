#ifndef SOURCEMETA_HYDRA_HTTP_TIME_H
#define SOURCEMETA_HYDRA_HTTP_TIME_H

#ifndef SOURCEMETA_HYDRA_HTTP_EXPORT
#include <sourcemeta/hydra/http_export.h>
#endif

#include <chrono> // std::chrono::system_clock::time_point
#include <string> // std::string

namespace sourcemeta::hydra::http {

/// @ingroup http
/// Convert a time point into a GMT string. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
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
/// assert(sourcemeta::hydra::http::to_gmt(point) ==
///   "Wed, 21 Oct 2015 11:28:00 GMT");
/// ```
///
/// On Windows, you might need to use `_mkgmtime` instead of `timegm`.
SOURCEMETA_HYDRA_HTTP_EXPORT
auto to_gmt(const std::chrono::system_clock::time_point time) -> std::string;

/// @ingroup http
/// Parse a GMT string into a time point. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
///
/// #include <chrono>
/// #include <ctime>
/// #include <cassert>
///
/// const auto point{
///     sourcemeta::hydra::http::from_gmt("Wed, 21 Oct 2015 11:28:00 GMT")};
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
SOURCEMETA_HYDRA_HTTP_EXPORT
auto from_gmt(const std::string &time) -> std::chrono::system_clock::time_point;

} // namespace sourcemeta::hydra::http

#endif
