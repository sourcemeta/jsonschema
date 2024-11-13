#ifndef SOURCEMETA_HYDRA_HTTP_HEADER_H
#define SOURCEMETA_HYDRA_HTTP_HEADER_H

#ifndef SOURCEMETA_HYDRA_HTTP_EXPORT
#include <sourcemeta/hydra/http_export.h>
#endif

#include <chrono>  // std::chrono::system_clock::time_point
#include <string>  // std::string
#include <utility> // std::pair
#include <vector>  // std::vector

namespace sourcemeta::hydra::http {

/// @ingroup http
/// Parse a header that consists of a GMT timestamp. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
/// #include <cassert>
///
/// const auto point{
///   sourcemeta::hydra::http::header_gmt("Wed, 21 Oct 2015 11:28:00 GMT")};
/// ```
auto SOURCEMETA_HYDRA_HTTP_EXPORT header_gmt(const std::string &value)
    -> std::chrono::system_clock::time_point;

/// A header list element consists of the element value and its quality value
/// See https://developer.mozilla.org/en-US/docs/Glossary/Quality_values
using HeaderListElement = std::pair<std::string, float>;

/// @ingroup http
/// Parse a header that consists of comma-separated ordered lists of elements.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
/// #include <cassert>
///
/// const auto elements{
///   sourcemeta::hydra::http::header_list("gzip, brotli")};
/// assert(elements.size() == 2);
/// assert(elements.at(0).first == "gzip");
/// assert(elements.at(1).first == "brotli");
/// ```
auto SOURCEMETA_HYDRA_HTTP_EXPORT header_list(const std::string &value)
    -> std::vector<HeaderListElement>;

} // namespace sourcemeta::hydra::http

#endif
