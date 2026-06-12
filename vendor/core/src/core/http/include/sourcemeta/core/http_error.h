#ifndef SOURCEMETA_CORE_HTTP_ERROR_H_
#define SOURCEMETA_CORE_HTTP_ERROR_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/http_method.h>

#include <stdexcept> // std::runtime_error
#include <string>    // std::string
#include <utility>   // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup http
/// An error that prevented obtaining a response, such as a connection
/// failure, a name resolution failure, or a TLS failure. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const sourcemeta::core::HTTPError error{
///     sourcemeta::core::HTTPMethod::GET,
///     "https://example.com", "Connection refused"};
/// assert(error.method() == sourcemeta::core::HTTPMethod::GET);
/// assert(error.url() == "https://example.com");
/// ```
class SOURCEMETA_CORE_HTTP_EXPORT HTTPError : public std::runtime_error {
public:
  HTTPError(const HTTPMethod method, std::string url,
            const std::string &message)
      : std::runtime_error{message}, method_{method}, url_{std::move(url)} {}

  /// Get the request method that triggered the failure
  [[nodiscard]] auto method() const noexcept -> HTTPMethod {
    return this->method_;
  }

  /// Get the request URL that triggered the failure
  [[nodiscard]] auto url() const noexcept -> const std::string & {
    return this->url_;
  }

private:
  HTTPMethod method_;
  std::string url_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
