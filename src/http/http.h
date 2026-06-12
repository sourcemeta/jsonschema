#ifndef SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_
#define SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_

#include <sourcemeta/core/http.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair, std::move
#include <vector>      // std::vector

namespace sourcemeta::jsonschema {

inline constexpr std::string_view HTTP_RESPONSE_TOO_LARGE_MESSAGE{
    "The response exceeds the maximum allowed size"};

struct HTTPRequestBody {
  // Sent as the `Content-Type` header of the request, so do not also
  // set that header by hand
  std::string_view content_type;
  std::string_view data;
};

struct HTTPRequest {
  sourcemeta::core::HTTPMethod method{sourcemeta::core::HTTPMethod::GET};
  // All request fields are views: the caller must keep the data they
  // refer to alive until `http_request` returns
  std::string_view url;
  // Repeated header names are permitted. Note that some platform HTTP
  // stacks fold them into a single comma-separated field line, which is
  // semantically equivalent per RFC 9110
  std::vector<std::pair<std::string_view, std::string_view>> headers;
  std::optional<HTTPRequestBody> body;
  // When set, abort with an `HTTPError` if the response body exceeds
  // this number of bytes
  std::optional<std::size_t> maximum_response_size;
};

struct HTTPResponse {
  sourcemeta::core::HTTPStatus status{};
  // Header names are normalised to lowercase and repeated headers are
  // preserved as separate entries, except on platform HTTP stacks like
  // NSURLSession that fold them into a single comma-separated entry. The
  // response owns its data, as the backend buffers it was read from do
  // not outlive the request
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

// Perform an HTTP request, following redirects. This function is
// implemented on top of NSURLSession on Apple platforms, WinHTTP on
// Windows, and cURL everywhere else. Failures to obtain a response are
// reported as `sourcemeta::core::HTTPError` exceptions
auto http_request(const HTTPRequest &request) -> HTTPResponse;

} // namespace sourcemeta::jsonschema

#endif
