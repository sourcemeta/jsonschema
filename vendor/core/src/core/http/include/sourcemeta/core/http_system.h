#ifndef SOURCEMETA_CORE_HTTP_SYSTEM_H_
#define SOURCEMETA_CORE_HTTP_SYSTEM_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/http_aws_sigv4.h>
#include <sourcemeta/core/http_method.h>
#include <sourcemeta/core/http_status.h>
#include <sourcemeta/core/text.h>

#include <chrono>      // std::chrono::milliseconds, std::chrono::seconds
#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

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
  /// Construct an error from a message, the environment variable that overrides
  /// the backend path, and the paths that were searched
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
  /// Construct a request for the given URL and method
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

  /// Get the value of the first header with the given name, compared case
  /// insensitively, or no value when no such header was added. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/http.h>
  /// #include <cassert>
  ///
  /// sourcemeta::core::HTTPSystemRequest request{"https://example.com"};
  /// request.header("Accept", "application/json");
  /// assert(request.header("accept").value() == "application/json");
  /// ```
  [[nodiscard]] auto header(const std::string_view name) const
      -> std::optional<std::string_view> {
    for (const auto &[key, value] : this->headers_) {
      if (equals_ignore_case(key, name)) {
        return value;
      }
    }

    return std::nullopt;
  }

  /// Get the request headers configured so far, in the order they were added
  [[nodiscard]] auto headers() const noexcept -> const auto & {
    return this->headers_;
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

  /// Sign this request with AWS Signature Version 4, stamping the `x-amz-date`,
  /// `x-amz-content-sha256`, and `Authorization` headers, plus
  /// `x-amz-security-token` when the credentials carry a session token. The
  /// path is normalised for every service except Amazon S3. The timestamp
  /// defaults to the current time. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/http.h>
  ///
  /// sourcemeta::core::HTTPSystemRequest request{
  ///     "https://example.s3.amazonaws.com/key",
  ///     sourcemeta::core::HTTPMethod::GET};
  /// request.sign_aws_sigv4({.access_key_id = "AKIDEXAMPLE",
  ///                         .secret_access_key = "secret"},
  ///                        "us-east-1", "s3");
  /// ```
  auto sign_aws_sigv4(const HTTPAWSCredentials &credentials,
                      const std::string_view region,
                      const std::string_view service,
                      const std::chrono::system_clock::time_point moment =
                          std::chrono::system_clock::now())
      -> HTTPSystemRequest &;

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
