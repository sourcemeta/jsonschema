#ifndef SOURCEMETA_CORE_HTTP_SYSTEM_H_
#define SOURCEMETA_CORE_HTTP_SYSTEM_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/http_method.h>
#include <sourcemeta/core/http_status.h>

#include <chrono>    // std::chrono::milliseconds, std::chrono::seconds
#include <cstddef>   // std::size_t
#include <optional>  // std::optional
#include <stdexcept> // std::runtime_error
#include <string>    // std::string
#include <utility>   // std::move, std::pair
#include <vector>    // std::vector

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup http
/// The result of performing a request against a system HTTP backend. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// sourcemeta::core::HTTPSystemRequest request{"https://example.com"};
/// const auto response{request.send()};
/// assert(response.status == sourcemeta::core::HTTP_STATUS_OK);
/// ```
struct HTTPResponse {
  /// The response status code
  HTTPStatus status{};
  /// The response headers, with names normalised to lowercase. Repeated
  /// headers are preserved as separate entries, except on backends that fold
  /// them into a single comma-separated entry, which is semantically
  /// equivalent per RFC 9110
  std::vector<std::pair<std::string, std::string>> headers;
  /// The response body, owned by this result
  std::string body;
  /// The effective URL after any followed redirects
  std::string url;
};

/// @ingroup http
/// An error that prevented loading the underlying system HTTP backend, such
/// as a missing dynamically loaded library. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const sourcemeta::core::HTTPSystemBackendError error{
///     "Could not find the system cURL library", "SOURCEMETA_CORE_CURL_SO",
///     {"libcurl.so.4"}};
/// assert(error.variable() == "SOURCEMETA_CORE_CURL_SO");
/// ```
class SOURCEMETA_CORE_HTTP_EXPORT HTTPSystemBackendError
    : public std::runtime_error {
public:
  HTTPSystemBackendError(const std::string &message, std::string variable,
                         std::vector<std::string> paths)
      : std::runtime_error{message}, variable_{std::move(variable)},
        paths_{std::move(paths)} {}

  /// Get the name of the environment variable that overrides the backend path
  [[nodiscard]] auto variable() const noexcept -> const std::string & {
    return this->variable_;
  }

  /// Get the paths that were searched while looking for the backend
  [[nodiscard]] auto paths() const noexcept
      -> const std::vector<std::string> & {
    return this->paths_;
  }

private:
  std::string variable_;
  std::vector<std::string> paths_;
};

/// @ingroup http
/// A simple cross-platform HTTP request that delegates to the system HTTP
/// stack, NSURLSession on Apple platforms, WinHTTP on Windows, and cURL
/// everywhere else. The request owns its data, configure it with the builder
/// methods and perform it with `send`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// sourcemeta::core::HTTPSystemRequest request{
///     "https://example.com", sourcemeta::core::HTTPMethod::POST};
/// request.header("Accept", "application/json");
/// request.body("{}", "application/json");
/// const auto response{request.send()};
/// assert(response.status == sourcemeta::core::HTTP_STATUS_OK);
/// ```
class SOURCEMETA_CORE_HTTP_EXPORT HTTPSystemRequest {
public:
  explicit HTTPSystemRequest(std::string url,
                             const HTTPMethod method = HTTPMethod::GET)
      : url_{std::move(url)}, method_{method} {}

  /// Set the request method
  auto method(const HTTPMethod method) -> HTTPSystemRequest & {
    this->method_ = method;
    return *this;
  }

  /// Add a request header. Repeated names are permitted
  auto header(std::string name, std::string value) -> HTTPSystemRequest & {
    this->headers_.emplace_back(std::move(name), std::move(value));
    return *this;
  }

  /// Set the request body, sent along with the given `Content-Type` header
  auto body(std::string data, std::string content_type) -> HTTPSystemRequest & {
    this->body_ =
        Body{.data = std::move(data), .content_type = std::move(content_type)};
    return *this;
  }

  /// Set whether to follow redirects, on by default
  auto follow_redirects(const bool value) -> HTTPSystemRequest & {
    this->follow_redirects_ = value;
    return *this;
  }

  /// Set the maximum number of redirects to follow, 20 by default
  auto maximum_redirects(const std::size_t value) -> HTTPSystemRequest & {
    this->maximum_redirects_ = value;
    return *this;
  }

  /// Set the total request timeout, 30 seconds by default
  auto timeout(const std::chrono::milliseconds value) -> HTTPSystemRequest & {
    this->timeout_ = value;
    return *this;
  }

  /// Set a best-effort timeout for establishing the connection, applied as
  /// each backend allows and falling back to the backend default when unset
  auto connect_timeout(const std::chrono::milliseconds value)
      -> HTTPSystemRequest & {
    this->connect_timeout_ = value;
    return *this;
  }

  /// Abort with an error if the response body exceeds this number of bytes
  auto maximum_response_size(const std::size_t value) -> HTTPSystemRequest & {
    this->maximum_response_size_ = value;
    return *this;
  }

  /// Perform the request. A failure to obtain a response is reported as an
  /// error, while unsuccessful status codes are returned on the result
  [[nodiscard]] auto send() const -> HTTPResponse;

private:
  struct Body {
    std::string data;
    std::string content_type;
  };

  std::string url_;
  HTTPMethod method_;
  std::vector<std::pair<std::string, std::string>> headers_;
  std::optional<Body> body_;
  bool follow_redirects_{true};
  std::size_t maximum_redirects_{20};
  std::chrono::milliseconds timeout_{std::chrono::seconds{30}};
  std::optional<std::chrono::milliseconds> connect_timeout_;
  std::optional<std::size_t> maximum_response_size_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
