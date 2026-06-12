#ifndef SOURCEMETA_CORE_HTTP_METHOD_H_
#define SOURCEMETA_CORE_HTTP_METHOD_H_

#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

/// @ingroup http
/// A request method per RFC 9110 §9.3 and RFC 5789 §2.
enum class HTTPMethod : std::uint8_t {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH
};

/// @ingroup http
/// Convert a request method into its case-sensitive token per RFC 9110 §9.1.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_method_string(
///            sourcemeta::core::HTTPMethod::GET) == "GET");
/// ```
inline constexpr auto http_method_string(const HTTPMethod method) noexcept
    -> std::string_view {
  switch (method) {
    case HTTPMethod::GET:
      return "GET";
    case HTTPMethod::HEAD:
      return "HEAD";
    case HTTPMethod::POST:
      return "POST";
    case HTTPMethod::PUT:
      return "PUT";
    case HTTPMethod::DELETE:
      return "DELETE";
    case HTTPMethod::CONNECT:
      return "CONNECT";
    case HTTPMethod::OPTIONS:
      return "OPTIONS";
    case HTTPMethod::TRACE:
      return "TRACE";
    case HTTPMethod::PATCH:
      return "PATCH";
  }

  std::unreachable();
}

} // namespace sourcemeta::core

#endif
