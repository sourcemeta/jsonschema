#ifndef SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_
#define SOURCEMETA_JSONSCHEMA_CLI_HTTP_H_

#include <sourcemeta/core/http_status.h>
#include <sourcemeta/core/text.h>

#include <concepts>    // std::invocable
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <optional>    // std::optional
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair, std::move
#include <vector>      // std::vector

namespace sourcemeta::jsonschema {

// TODO: Upstream this enumeration into the `sourcemeta::core` HTTP module
enum class HTTPMethod : std::uint8_t {
  Get,
  Head,
  Post,
  Put,
  Delete,
  Patch,
  Options
};

// Convert an HTTP method into its RFC 9110 token
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
inline constexpr auto http_method_string(const HTTPMethod method) noexcept
    -> std::string_view {
  switch (method) {
    case HTTPMethod::Head:
      return "HEAD";
    case HTTPMethod::Post:
      return "POST";
    case HTTPMethod::Put:
      return "PUT";
    case HTTPMethod::Delete:
      return "DELETE";
    case HTTPMethod::Patch:
      return "PATCH";
    case HTTPMethod::Options:
      return "OPTIONS";
    case HTTPMethod::Get:
    default:
      return "GET";
  }
}

// Whether a raw header line is an HTTP status line, marking the start of
// a new message header block per RFC 9112
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
inline constexpr auto http_is_status_line(const std::string_view line) noexcept
    -> bool {
  return line.starts_with("HTTP/");
}

// Accumulate raw HTTP header lines into any buffer that can be cleared
// and appended to, retaining only the block of the most recent message,
// given that transparently following redirects or receiving interim 1xx
// responses produces one header block per message
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
template <typename Buffer>
inline auto http_accumulate_header_line(Buffer &buffer,
                                        const std::string_view line) -> void {
  if (http_is_status_line(line)) {
    buffer.clear();
  }

  buffer.append(line);
}

// Parse the field lines of a raw HTTP message header block (skipping the
// start line), invoking the callback with the raw field name and the
// field value trimmed of optional whitespace per RFC 9112. This function
// does not allocate any memory: both arguments are views into the input
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
template <typename Callback>
  requires std::invocable<Callback, std::string_view, std::string_view>
inline auto http_parse_headers(const std::string_view input,
                               Callback &&callback) -> void {
  std::size_t cursor{input.find("\r\n")};
  while (cursor != std::string_view::npos) {
    cursor += 2;
    const auto end{input.find("\r\n", cursor)};
    if (end == std::string_view::npos || end == cursor) {
      break;
    }

    const auto parts{
        sourcemeta::core::split_once(input.substr(cursor, end - cursor), ':')};
    if (parts.has_value()) {
      callback(parts->first, sourcemeta::core::trim(parts->second));
    }

    cursor = end;
  }
}

// Parse the field lines of a raw HTTP message header block (skipping the
// start line) into any container of name/value pairs, normalising header
// names to lowercase and preserving repeated headers as separate entries
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
template <typename Container>
  requires requires(Container container, std::string name, std::string value) {
    container.emplace_back(std::move(name), std::move(value));
  }
inline auto http_parse_headers(const std::string_view input, Container &headers)
    -> void {
  http_parse_headers(input, [&headers](const std::string_view name,
                                       const std::string_view value) {
    std::string header_name{name};
    sourcemeta::core::to_lowercase(header_name);
    headers.emplace_back(std::move(header_name), std::string{value});
  });
}

// Serialise HTTP headers, given as any range of name/value pairs, into
// CRLF-delimited field lines
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
template <typename Headers>
inline auto http_serialize_headers(const Headers &headers) -> std::string {
  std::size_t total_size{0};
  for (const auto &[name, value] : headers) {
    // Account for the colon, the space, and the trailing CRLF
    total_size += name.size() + value.size() + 4;
  }

  std::string result;
  result.reserve(total_size);
  for (const auto &[name, value] : headers) {
    result += name;
    result += ": ";
    result += value;
    result += "\r\n";
  }

  return result;
}

// Find the value of the first header with the given lowercase name in
// any range of name/value pairs
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
template <typename Headers>
inline auto http_header_find(const Headers &headers,
                             const std::string_view name)
    -> std::optional<std::string_view> {
  for (const auto &[header_name, header_value] : headers) {
    if (header_name == name) {
      return std::string_view{header_value};
    }
  }

  return std::nullopt;
}

// Resolve a numeric status code into its registered RFC 9110 status.
// Unknown codes resolve to a status with an empty reason phrase
// TODO: Upstream this function into the `sourcemeta::core` HTTP module
inline constexpr auto http_status_from_code(const std::uint16_t code) noexcept
    -> sourcemeta::core::HTTPStatus {
  switch (code) {
    case 100:
      return sourcemeta::core::HTTP_STATUS_CONTINUE;
    case 101:
      return sourcemeta::core::HTTP_STATUS_SWITCHING_PROTOCOLS;
    case 102:
      return sourcemeta::core::HTTP_STATUS_PROCESSING;
    case 103:
      return sourcemeta::core::HTTP_STATUS_EARLY_HINTS;
    case 200:
      return sourcemeta::core::HTTP_STATUS_OK;
    case 201:
      return sourcemeta::core::HTTP_STATUS_CREATED;
    case 202:
      return sourcemeta::core::HTTP_STATUS_ACCEPTED;
    case 203:
      return sourcemeta::core::HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION;
    case 204:
      return sourcemeta::core::HTTP_STATUS_NO_CONTENT;
    case 205:
      return sourcemeta::core::HTTP_STATUS_RESET_CONTENT;
    case 206:
      return sourcemeta::core::HTTP_STATUS_PARTIAL_CONTENT;
    case 207:
      return sourcemeta::core::HTTP_STATUS_MULTI_STATUS;
    case 208:
      return sourcemeta::core::HTTP_STATUS_ALREADY_REPORTED;
    case 226:
      return sourcemeta::core::HTTP_STATUS_IM_USED;
    case 300:
      return sourcemeta::core::HTTP_STATUS_MULTIPLE_CHOICES;
    case 301:
      return sourcemeta::core::HTTP_STATUS_MOVED_PERMANENTLY;
    case 302:
      return sourcemeta::core::HTTP_STATUS_FOUND;
    case 303:
      return sourcemeta::core::HTTP_STATUS_SEE_OTHER;
    case 304:
      return sourcemeta::core::HTTP_STATUS_NOT_MODIFIED;
    case 305:
      return sourcemeta::core::HTTP_STATUS_USE_PROXY;
    case 307:
      return sourcemeta::core::HTTP_STATUS_TEMPORARY_REDIRECT;
    case 308:
      return sourcemeta::core::HTTP_STATUS_PERMANENT_REDIRECT;
    case 400:
      return sourcemeta::core::HTTP_STATUS_BAD_REQUEST;
    case 401:
      return sourcemeta::core::HTTP_STATUS_UNAUTHORIZED;
    case 402:
      return sourcemeta::core::HTTP_STATUS_PAYMENT_REQUIRED;
    case 403:
      return sourcemeta::core::HTTP_STATUS_FORBIDDEN;
    case 404:
      return sourcemeta::core::HTTP_STATUS_NOT_FOUND;
    case 405:
      return sourcemeta::core::HTTP_STATUS_METHOD_NOT_ALLOWED;
    case 406:
      return sourcemeta::core::HTTP_STATUS_NOT_ACCEPTABLE;
    case 407:
      return sourcemeta::core::HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED;
    case 408:
      return sourcemeta::core::HTTP_STATUS_REQUEST_TIMEOUT;
    case 409:
      return sourcemeta::core::HTTP_STATUS_CONFLICT;
    case 410:
      return sourcemeta::core::HTTP_STATUS_GONE;
    case 411:
      return sourcemeta::core::HTTP_STATUS_LENGTH_REQUIRED;
    case 412:
      return sourcemeta::core::HTTP_STATUS_PRECONDITION_FAILED;
    case 413:
      return sourcemeta::core::HTTP_STATUS_CONTENT_TOO_LARGE;
    case 414:
      return sourcemeta::core::HTTP_STATUS_URI_TOO_LONG;
    case 415:
      return sourcemeta::core::HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE;
    case 416:
      return sourcemeta::core::HTTP_STATUS_RANGE_NOT_SATISFIABLE;
    case 417:
      return sourcemeta::core::HTTP_STATUS_EXPECTATION_FAILED;
    case 418:
      return sourcemeta::core::HTTP_STATUS_IM_A_TEAPOT;
    case 421:
      return sourcemeta::core::HTTP_STATUS_MISDIRECTED_REQUEST;
    case 422:
      return sourcemeta::core::HTTP_STATUS_UNPROCESSABLE_CONTENT;
    case 423:
      return sourcemeta::core::HTTP_STATUS_LOCKED;
    case 424:
      return sourcemeta::core::HTTP_STATUS_FAILED_DEPENDENCY;
    case 425:
      return sourcemeta::core::HTTP_STATUS_TOO_EARLY;
    case 426:
      return sourcemeta::core::HTTP_STATUS_UPGRADE_REQUIRED;
    case 428:
      return sourcemeta::core::HTTP_STATUS_PRECONDITION_REQUIRED;
    case 429:
      return sourcemeta::core::HTTP_STATUS_TOO_MANY_REQUESTS;
    case 431:
      return sourcemeta::core::HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;
    case 451:
      return sourcemeta::core::HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS;
    case 500:
      return sourcemeta::core::HTTP_STATUS_INTERNAL_SERVER_ERROR;
    case 501:
      return sourcemeta::core::HTTP_STATUS_NOT_IMPLEMENTED;
    case 502:
      return sourcemeta::core::HTTP_STATUS_BAD_GATEWAY;
    case 503:
      return sourcemeta::core::HTTP_STATUS_SERVICE_UNAVAILABLE;
    case 504:
      return sourcemeta::core::HTTP_STATUS_GATEWAY_TIMEOUT;
    case 505:
      return sourcemeta::core::HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED;
    case 506:
      return sourcemeta::core::HTTP_STATUS_VARIANT_ALSO_NEGOTIATES;
    case 507:
      return sourcemeta::core::HTTP_STATUS_INSUFFICIENT_STORAGE;
    case 508:
      return sourcemeta::core::HTTP_STATUS_LOOP_DETECTED;
    case 510:
      return sourcemeta::core::HTTP_STATUS_NOT_EXTENDED;
    case 511:
      return sourcemeta::core::HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED;
    default:
      return sourcemeta::core::HTTPStatus{.code = code};
  }
}

// Thrown on any failure that prevented obtaining an HTTP response, such
// as a connection error, a name resolution error, or a TLS error
// TODO: Upstream this class into the `sourcemeta::core` HTTP module
class HTTPError : public std::runtime_error {
public:
  HTTPError(const HTTPMethod method, std::string url,
            const std::string &message)
      : std::runtime_error{message}, method_{method}, url_{std::move(url)} {}

  [[nodiscard]] auto method() const noexcept -> HTTPMethod {
    return this->method_;
  }

  [[nodiscard]] auto url() const noexcept -> const std::string & {
    return this->url_;
  }

private:
  HTTPMethod method_;
  std::string url_;
};

inline constexpr std::string_view HTTP_RESPONSE_TOO_LARGE_MESSAGE{
    "The response exceeds the maximum allowed size"};

struct HTTPRequestBody {
  // Sent as the `Content-Type` header of the request, so do not also
  // set that header by hand
  std::string_view content_type;
  std::string_view data;
};

struct HTTPRequest {
  HTTPMethod method{HTTPMethod::Get};
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
  // preserved as separate entries. The response owns its data, as the
  // backend buffers it was read from do not outlive the request
  std::vector<std::pair<std::string, std::string>> headers;
  std::string body;
};

// Perform an HTTP request, following redirects. This function is
// implemented on top of NSURLSession on Apple platforms, WinHTTP on
// Windows, and cURL everywhere else. Failures to obtain a response are
// reported as `HTTPError` exceptions
auto http_request(const HTTPRequest &request) -> HTTPResponse;

} // namespace sourcemeta::jsonschema

#endif
