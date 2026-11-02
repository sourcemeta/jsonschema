#ifndef SOURCEMETA_CORE_HTTP_ERROR_H_
#define SOURCEMETA_CORE_HTTP_ERROR_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/error.h>
#include <sourcemeta/core/http_method.h>
#include <sourcemeta/core/http_status.h>

#include <chrono>      // std::chrono::milliseconds
#include <cstdint>     // std::uint16_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup http
/// An error that prevented obtaining a response, such as a connection
/// failure, a name resolution failure, or a TLS failure. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const sourcemeta::core::HTTPError error{
///     "Connection refused", sourcemeta::core::HTTPMethod::GET,
///     "https://example.com"};
/// assert(error.method() == sourcemeta::core::HTTPMethod::GET);
/// assert(error.url() == "https://example.com");
/// ```
class SOURCEMETA_CORE_HTTP_EXPORT HTTPError : public Error {
public:
  /// Construct an error from a message, the request method, and the URL
  HTTPError(const StaticString message, const HTTPMethod method,
            std::string url)
      : Error{message}, method_{method}, url_{std::move(url)} {}

  /// Construct an error that also carries what the backend reported, which is
  /// the only account of the failure that names the condition the system hit
  HTTPError(const StaticString message, const HTTPMethod method,
            std::string url, std::string detail)
      : Error{message}, method_{method}, url_{std::move(url)},
        detail_{std::move(detail)} {}

  /// Get the request method that triggered the failure
  [[nodiscard]] auto method() const noexcept -> HTTPMethod {
    return this->method_;
  }

  /// Get the request URL that triggered the failure
  [[nodiscard]] auto url() const noexcept -> const std::string & {
    return this->url_;
  }

  /// Get what the backend reported, empty when it reported nothing
  [[nodiscard]] auto detail() const noexcept -> std::string_view {
    return this->detail_;
  }

private:
  HTTPMethod method_;
  std::string url_;
  std::string detail_;
};

/// @ingroup http
/// An error for a request that did not complete within its configured
/// timeout. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const sourcemeta::core::HTTPTimeoutError error{
///     sourcemeta::core::HTTPMethod::GET,
///     "https://example.com", std::chrono::seconds{30}};
/// assert(error.timeout() == std::chrono::seconds{30});
/// ```
class SOURCEMETA_CORE_HTTP_EXPORT HTTPTimeoutError : public HTTPError {
public:
  /// Construct an error from the request method, URL, and the total timeout
  /// that was configured for the request
  HTTPTimeoutError(const HTTPMethod method, std::string url,
                   const std::chrono::milliseconds timeout)
      : HTTPError{"The HTTP request timed out", method, std::move(url)},
        timeout_{timeout} {}

  /// Get the total timeout that was configured for the request. A backend
  /// does not report which of the configured bounds elapsed, so this is the
  /// total timeout even when a narrower connection timeout was the one hit
  [[nodiscard]] auto timeout() const noexcept -> std::chrono::milliseconds {
    return this->timeout_;
  }

private:
  std::chrono::milliseconds timeout_;
};

/// @ingroup http
/// An error for a response with an unsuccessful status code, owning a copy
/// of the status data. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const sourcemeta::core::HTTPStatusError error{
///     sourcemeta::core::HTTPMethod::GET,
///     "https://example.com", sourcemeta::core::HTTP_STATUS_NOT_FOUND};
/// assert(error.status() == sourcemeta::core::HTTP_STATUS_NOT_FOUND);
/// ```
class SOURCEMETA_CORE_HTTP_EXPORT HTTPStatusError : public HTTPError {
public:
  /// Construct an error from the request method, URL, and the unsuccessful
  /// response status
  HTTPStatusError(const HTTPMethod method, std::string url,
                  const HTTPStatus &status)
      : HTTPError{"Unsuccessful HTTP response", method, std::move(url)},
        code_{status.code}, phrase_{status.phrase}, wire_{status.wire} {}

  /// Get the response status that triggered the failure. The contained
  /// views borrow from this error and stay valid for its lifetime
  [[nodiscard]] auto status() const noexcept -> HTTPStatus {
    return {.code = this->code_, .phrase = this->phrase_, .wire = this->wire_};
  }

private:
  std::uint16_t code_;
  std::string phrase_;
  std::string wire_;
};

/// @ingroup http
/// An error raised when no non-zero trace context identifier could be drawn.
class SOURCEMETA_CORE_HTTP_EXPORT HTTPEntropyError : public Error {
public:
  HTTPEntropyError() noexcept
      : Error{"Could not draw a non-zero trace context identifier"} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
