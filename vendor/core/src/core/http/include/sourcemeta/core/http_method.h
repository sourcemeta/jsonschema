#ifndef SOURCEMETA_CORE_HTTP_METHOD_H_
#define SOURCEMETA_CORE_HTTP_METHOD_H_

#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

/// @ingroup http
/// A request method per RFC 9110 §9.3 and RFC 5789 §2.
enum class HTTPMethod : std::uint8_t {
  /// Transfer a current representation of the target resource.
  GET,
  /// Identical to a retrieval but without transferring the response content.
  HEAD,
  /// Process the enclosed representation according to the resource semantics.
  POST,
  /// Replace the target resource with the enclosed representation.
  PUT,
  /// Remove the association between the target resource and its function.
  DELETE,
  /// Establish a tunnel to the server identified by the target resource.
  CONNECT,
  /// Describe the communication options available for the target resource.
  OPTIONS,
  /// Perform a message loop-back test along the path to the target resource.
  TRACE,
  /// Apply a partial modification to the target resource per RFC 5789 §2.
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
