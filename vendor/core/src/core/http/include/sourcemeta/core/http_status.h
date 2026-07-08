#ifndef SOURCEMETA_CORE_HTTP_STATUS_H_
#define SOURCEMETA_CORE_HTTP_STATUS_H_

#include <cstdint>     // std::uint16_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup http
/// A typed HTTP status code per RFC 9110 §15. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::HTTP_STATUS_OK.code == 200);
/// assert(sourcemeta::core::HTTP_STATUS_OK.phrase == "OK");
/// assert(sourcemeta::core::HTTP_STATUS_OK.wire == "200 OK");
/// ```
struct HTTPStatus {
  /// The numeric status code
  std::uint16_t code;
  /// The reason phrase
  std::string_view phrase;
  /// The status code and reason phrase in their wire form
  std::string_view wire;

  constexpr auto operator==(const HTTPStatus &) const -> bool = default;
};

/// @ingroup http
/// RFC 9110 §15.2.1 (Informational).
inline constexpr HTTPStatus HTTP_STATUS_CONTINUE{
    .code = 100, .phrase = "Continue", .wire = "100 Continue"};

/// @ingroup http
/// RFC 9110 §15.2.2 (Informational).
inline constexpr HTTPStatus HTTP_STATUS_SWITCHING_PROTOCOLS{
    .code = 101,
    .phrase = "Switching Protocols",
    .wire = "101 Switching Protocols"};

/// @ingroup http
/// RFC 2518 §10.1 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_PROCESSING{
    .code = 102, .phrase = "Processing", .wire = "102 Processing"};

/// @ingroup http
/// RFC 8297 §2.
inline constexpr HTTPStatus HTTP_STATUS_EARLY_HINTS{
    .code = 103, .phrase = "Early Hints", .wire = "103 Early Hints"};

/// @ingroup http
/// RFC 9110 §15.3.1 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_OK{
    .code = 200, .phrase = "OK", .wire = "200 OK"};

/// @ingroup http
/// RFC 9110 §15.3.2 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_CREATED{
    .code = 201, .phrase = "Created", .wire = "201 Created"};

/// @ingroup http
/// RFC 9110 §15.3.3 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_ACCEPTED{
    .code = 202, .phrase = "Accepted", .wire = "202 Accepted"};

/// @ingroup http
/// RFC 9110 §15.3.4 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION{
    .code = 203,
    .phrase = "Non-Authoritative Information",
    .wire = "203 Non-Authoritative Information"};

/// @ingroup http
/// RFC 9110 §15.3.5 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_NO_CONTENT{
    .code = 204, .phrase = "No Content", .wire = "204 No Content"};

/// @ingroup http
/// RFC 9110 §15.3.6 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_RESET_CONTENT{
    .code = 205, .phrase = "Reset Content", .wire = "205 Reset Content"};

/// @ingroup http
/// RFC 9110 §15.3.7 (Successful).
inline constexpr HTTPStatus HTTP_STATUS_PARTIAL_CONTENT{
    .code = 206, .phrase = "Partial Content", .wire = "206 Partial Content"};

/// @ingroup http
/// RFC 4918 §11.1 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_MULTI_STATUS{
    .code = 207, .phrase = "Multi-Status", .wire = "207 Multi-Status"};

/// @ingroup http
/// RFC 5842 §7.1 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_ALREADY_REPORTED{
    .code = 208, .phrase = "Already Reported", .wire = "208 Already Reported"};

/// @ingroup http
/// RFC 3229 §10.4.1 (HTTP Delta Encoding).
inline constexpr HTTPStatus HTTP_STATUS_IM_USED{
    .code = 226, .phrase = "IM Used", .wire = "226 IM Used"};

/// @ingroup http
/// RFC 9110 §15.4.1 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_MULTIPLE_CHOICES{
    .code = 300, .phrase = "Multiple Choices", .wire = "300 Multiple Choices"};

/// @ingroup http
/// RFC 9110 §15.4.2 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_MOVED_PERMANENTLY{
    .code = 301,
    .phrase = "Moved Permanently",
    .wire = "301 Moved Permanently"};

/// @ingroup http
/// RFC 9110 §15.4.3 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_FOUND{
    .code = 302, .phrase = "Found", .wire = "302 Found"};

/// @ingroup http
/// RFC 9110 §15.4.4 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_SEE_OTHER{
    .code = 303, .phrase = "See Other", .wire = "303 See Other"};

/// @ingroup http
/// RFC 9110 §15.4.5 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_NOT_MODIFIED{
    .code = 304, .phrase = "Not Modified", .wire = "304 Not Modified"};

/// @ingroup http
/// RFC 9110 §15.4.6 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_USE_PROXY{
    .code = 305, .phrase = "Use Proxy", .wire = "305 Use Proxy"};

/// @ingroup http
/// RFC 9110 §15.4.8 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_TEMPORARY_REDIRECT{
    .code = 307,
    .phrase = "Temporary Redirect",
    .wire = "307 Temporary Redirect"};

/// @ingroup http
/// RFC 9110 §15.4.9 (Redirection).
inline constexpr HTTPStatus HTTP_STATUS_PERMANENT_REDIRECT{
    .code = 308,
    .phrase = "Permanent Redirect",
    .wire = "308 Permanent Redirect"};

/// @ingroup http
/// RFC 9110 §15.5.1 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_BAD_REQUEST{
    .code = 400, .phrase = "Bad Request", .wire = "400 Bad Request"};

/// @ingroup http
/// RFC 9110 §15.5.2 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_UNAUTHORIZED{
    .code = 401, .phrase = "Unauthorized", .wire = "401 Unauthorized"};

/// @ingroup http
/// RFC 9110 §15.5.3 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_PAYMENT_REQUIRED{
    .code = 402, .phrase = "Payment Required", .wire = "402 Payment Required"};

/// @ingroup http
/// RFC 9110 §15.5.4 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_FORBIDDEN{
    .code = 403, .phrase = "Forbidden", .wire = "403 Forbidden"};

/// @ingroup http
/// RFC 9110 §15.5.5 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_NOT_FOUND{
    .code = 404, .phrase = "Not Found", .wire = "404 Not Found"};

/// @ingroup http
/// RFC 9110 §15.5.6 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_METHOD_NOT_ALLOWED{
    .code = 405,
    .phrase = "Method Not Allowed",
    .wire = "405 Method Not Allowed"};

/// @ingroup http
/// RFC 9110 §15.5.7 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_NOT_ACCEPTABLE{
    .code = 406, .phrase = "Not Acceptable", .wire = "406 Not Acceptable"};

/// @ingroup http
/// RFC 9110 §15.5.8 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED{
    .code = 407,
    .phrase = "Proxy Authentication Required",
    .wire = "407 Proxy Authentication Required"};

/// @ingroup http
/// RFC 9110 §15.5.9 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_REQUEST_TIMEOUT{
    .code = 408, .phrase = "Request Timeout", .wire = "408 Request Timeout"};

/// @ingroup http
/// RFC 9110 §15.5.10 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_CONFLICT{
    .code = 409, .phrase = "Conflict", .wire = "409 Conflict"};

/// @ingroup http
/// RFC 9110 §15.5.11 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_GONE{
    .code = 410, .phrase = "Gone", .wire = "410 Gone"};

/// @ingroup http
/// RFC 9110 §15.5.12 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_LENGTH_REQUIRED{
    .code = 411, .phrase = "Length Required", .wire = "411 Length Required"};

/// @ingroup http
/// RFC 9110 §15.5.13 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_PRECONDITION_FAILED{
    .code = 412,
    .phrase = "Precondition Failed",
    .wire = "412 Precondition Failed"};

/// @ingroup http
/// RFC 9110 §15.5.14 (Client Error). RFC 9110 renamed this from "Payload
/// Too Large" (RFC 7231) to "Content Too Large".
inline constexpr HTTPStatus HTTP_STATUS_CONTENT_TOO_LARGE{
    .code = 413,
    .phrase = "Content Too Large",
    .wire = "413 Content Too Large"};

/// @ingroup http
/// RFC 9110 §15.5.15 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_URI_TOO_LONG{
    .code = 414, .phrase = "URI Too Long", .wire = "414 URI Too Long"};

/// @ingroup http
/// RFC 9110 §15.5.16 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE{
    .code = 415,
    .phrase = "Unsupported Media Type",
    .wire = "415 Unsupported Media Type"};

/// @ingroup http
/// RFC 9110 §15.5.17 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_RANGE_NOT_SATISFIABLE{
    .code = 416,
    .phrase = "Range Not Satisfiable",
    .wire = "416 Range Not Satisfiable"};

/// @ingroup http
/// RFC 9110 §15.5.18 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_EXPECTATION_FAILED{
    .code = 417,
    .phrase = "Expectation Failed",
    .wire = "417 Expectation Failed"};

/// @ingroup http
/// RFC 2324 §2.3.2 (HTCPCP).
inline constexpr HTTPStatus HTTP_STATUS_IM_A_TEAPOT{
    .code = 418, .phrase = "I'm a Teapot", .wire = "418 I'm a Teapot"};

/// @ingroup http
/// RFC 9110 §15.5.20 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_MISDIRECTED_REQUEST{
    .code = 421,
    .phrase = "Misdirected Request",
    .wire = "421 Misdirected Request"};

/// @ingroup http
/// RFC 9110 §15.5.21 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_UNPROCESSABLE_CONTENT{
    .code = 422,
    .phrase = "Unprocessable Content",
    .wire = "422 Unprocessable Content"};

/// @ingroup http
/// RFC 4918 §11.3 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_LOCKED{
    .code = 423, .phrase = "Locked", .wire = "423 Locked"};

/// @ingroup http
/// RFC 4918 §11.4 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_FAILED_DEPENDENCY{
    .code = 424,
    .phrase = "Failed Dependency",
    .wire = "424 Failed Dependency"};

/// @ingroup http
/// RFC 8470 §5.2.
inline constexpr HTTPStatus HTTP_STATUS_TOO_EARLY{
    .code = 425, .phrase = "Too Early", .wire = "425 Too Early"};

/// @ingroup http
/// RFC 9110 §15.5.22 (Client Error).
inline constexpr HTTPStatus HTTP_STATUS_UPGRADE_REQUIRED{
    .code = 426, .phrase = "Upgrade Required", .wire = "426 Upgrade Required"};

/// @ingroup http
/// RFC 6585 §3.
inline constexpr HTTPStatus HTTP_STATUS_PRECONDITION_REQUIRED{
    .code = 428,
    .phrase = "Precondition Required",
    .wire = "428 Precondition Required"};

/// @ingroup http
/// RFC 6585 §4.
inline constexpr HTTPStatus HTTP_STATUS_TOO_MANY_REQUESTS{
    .code = 429,
    .phrase = "Too Many Requests",
    .wire = "429 Too Many Requests"};

/// @ingroup http
/// RFC 6585 §5.
inline constexpr HTTPStatus HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE{
    .code = 431,
    .phrase = "Request Header Fields Too Large",
    .wire = "431 Request Header Fields Too Large"};

/// @ingroup http
/// RFC 7725 §3.
inline constexpr HTTPStatus HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS{
    .code = 451,
    .phrase = "Unavailable For Legal Reasons",
    .wire = "451 Unavailable For Legal Reasons"};

/// @ingroup http
/// RFC 9110 §15.6.1 (Server Error).
inline constexpr HTTPStatus HTTP_STATUS_INTERNAL_SERVER_ERROR{
    .code = 500,
    .phrase = "Internal Server Error",
    .wire = "500 Internal Server Error"};

/// @ingroup http
/// RFC 9110 §15.6.2 (Server Error).
inline constexpr HTTPStatus HTTP_STATUS_NOT_IMPLEMENTED{
    .code = 501, .phrase = "Not Implemented", .wire = "501 Not Implemented"};

/// @ingroup http
/// RFC 9110 §15.6.3 (Server Error).
inline constexpr HTTPStatus HTTP_STATUS_BAD_GATEWAY{
    .code = 502, .phrase = "Bad Gateway", .wire = "502 Bad Gateway"};

/// @ingroup http
/// RFC 9110 §15.6.4 (Server Error).
inline constexpr HTTPStatus HTTP_STATUS_SERVICE_UNAVAILABLE{
    .code = 503,
    .phrase = "Service Unavailable",
    .wire = "503 Service Unavailable"};

/// @ingroup http
/// RFC 9110 §15.6.5 (Server Error).
inline constexpr HTTPStatus HTTP_STATUS_GATEWAY_TIMEOUT{
    .code = 504, .phrase = "Gateway Timeout", .wire = "504 Gateway Timeout"};

/// @ingroup http
/// RFC 9110 §15.6.6 (Server Error).
inline constexpr HTTPStatus HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED{
    .code = 505,
    .phrase = "HTTP Version Not Supported",
    .wire = "505 HTTP Version Not Supported"};

/// @ingroup http
/// RFC 2295 §8.1 (Transparent Content Negotiation).
inline constexpr HTTPStatus HTTP_STATUS_VARIANT_ALSO_NEGOTIATES{
    .code = 506,
    .phrase = "Variant Also Negotiates",
    .wire = "506 Variant Also Negotiates"};

/// @ingroup http
/// RFC 4918 §11.5 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_INSUFFICIENT_STORAGE{
    .code = 507,
    .phrase = "Insufficient Storage",
    .wire = "507 Insufficient Storage"};

/// @ingroup http
/// RFC 5842 §7.2 (WebDAV).
inline constexpr HTTPStatus HTTP_STATUS_LOOP_DETECTED{
    .code = 508, .phrase = "Loop Detected", .wire = "508 Loop Detected"};

/// @ingroup http
/// RFC 2774 §7.
inline constexpr HTTPStatus HTTP_STATUS_NOT_EXTENDED{
    .code = 510, .phrase = "Not Extended", .wire = "510 Not Extended"};

/// @ingroup http
/// RFC 6585 §6.
inline constexpr HTTPStatus HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED{
    .code = 511,
    .phrase = "Network Authentication Required",
    .wire = "511 Network Authentication Required"};

/// @ingroup http
/// Resolve a numeric status code into its registered status, with unknown
/// codes resolving to an empty reason phrase. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_status_from_code(200) ==
///        sourcemeta::core::HTTP_STATUS_OK);
/// assert(sourcemeta::core::http_status_from_code(599).phrase.empty());
/// ```
inline constexpr auto http_status_from_code(const std::uint16_t code) noexcept
    -> HTTPStatus {
  switch (code) {
    case 100:
      return HTTP_STATUS_CONTINUE;
    case 101:
      return HTTP_STATUS_SWITCHING_PROTOCOLS;
    case 102:
      return HTTP_STATUS_PROCESSING;
    case 103:
      return HTTP_STATUS_EARLY_HINTS;
    case 200:
      return HTTP_STATUS_OK;
    case 201:
      return HTTP_STATUS_CREATED;
    case 202:
      return HTTP_STATUS_ACCEPTED;
    case 203:
      return HTTP_STATUS_NON_AUTHORITATIVE_INFORMATION;
    case 204:
      return HTTP_STATUS_NO_CONTENT;
    case 205:
      return HTTP_STATUS_RESET_CONTENT;
    case 206:
      return HTTP_STATUS_PARTIAL_CONTENT;
    case 207:
      return HTTP_STATUS_MULTI_STATUS;
    case 208:
      return HTTP_STATUS_ALREADY_REPORTED;
    case 226:
      return HTTP_STATUS_IM_USED;
    case 300:
      return HTTP_STATUS_MULTIPLE_CHOICES;
    case 301:
      return HTTP_STATUS_MOVED_PERMANENTLY;
    case 302:
      return HTTP_STATUS_FOUND;
    case 303:
      return HTTP_STATUS_SEE_OTHER;
    case 304:
      return HTTP_STATUS_NOT_MODIFIED;
    case 305:
      return HTTP_STATUS_USE_PROXY;
    case 307:
      return HTTP_STATUS_TEMPORARY_REDIRECT;
    case 308:
      return HTTP_STATUS_PERMANENT_REDIRECT;
    case 400:
      return HTTP_STATUS_BAD_REQUEST;
    case 401:
      return HTTP_STATUS_UNAUTHORIZED;
    case 402:
      return HTTP_STATUS_PAYMENT_REQUIRED;
    case 403:
      return HTTP_STATUS_FORBIDDEN;
    case 404:
      return HTTP_STATUS_NOT_FOUND;
    case 405:
      return HTTP_STATUS_METHOD_NOT_ALLOWED;
    case 406:
      return HTTP_STATUS_NOT_ACCEPTABLE;
    case 407:
      return HTTP_STATUS_PROXY_AUTHENTICATION_REQUIRED;
    case 408:
      return HTTP_STATUS_REQUEST_TIMEOUT;
    case 409:
      return HTTP_STATUS_CONFLICT;
    case 410:
      return HTTP_STATUS_GONE;
    case 411:
      return HTTP_STATUS_LENGTH_REQUIRED;
    case 412:
      return HTTP_STATUS_PRECONDITION_FAILED;
    case 413:
      return HTTP_STATUS_CONTENT_TOO_LARGE;
    case 414:
      return HTTP_STATUS_URI_TOO_LONG;
    case 415:
      return HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE;
    case 416:
      return HTTP_STATUS_RANGE_NOT_SATISFIABLE;
    case 417:
      return HTTP_STATUS_EXPECTATION_FAILED;
    case 418:
      return HTTP_STATUS_IM_A_TEAPOT;
    case 421:
      return HTTP_STATUS_MISDIRECTED_REQUEST;
    case 422:
      return HTTP_STATUS_UNPROCESSABLE_CONTENT;
    case 423:
      return HTTP_STATUS_LOCKED;
    case 424:
      return HTTP_STATUS_FAILED_DEPENDENCY;
    case 425:
      return HTTP_STATUS_TOO_EARLY;
    case 426:
      return HTTP_STATUS_UPGRADE_REQUIRED;
    case 428:
      return HTTP_STATUS_PRECONDITION_REQUIRED;
    case 429:
      return HTTP_STATUS_TOO_MANY_REQUESTS;
    case 431:
      return HTTP_STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE;
    case 451:
      return HTTP_STATUS_UNAVAILABLE_FOR_LEGAL_REASONS;
    case 500:
      return HTTP_STATUS_INTERNAL_SERVER_ERROR;
    case 501:
      return HTTP_STATUS_NOT_IMPLEMENTED;
    case 502:
      return HTTP_STATUS_BAD_GATEWAY;
    case 503:
      return HTTP_STATUS_SERVICE_UNAVAILABLE;
    case 504:
      return HTTP_STATUS_GATEWAY_TIMEOUT;
    case 505:
      return HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED;
    case 506:
      return HTTP_STATUS_VARIANT_ALSO_NEGOTIATES;
    case 507:
      return HTTP_STATUS_INSUFFICIENT_STORAGE;
    case 508:
      return HTTP_STATUS_LOOP_DETECTED;
    case 510:
      return HTTP_STATUS_NOT_EXTENDED;
    case 511:
      return HTTP_STATUS_NETWORK_AUTHENTICATION_REQUIRED;
    default:
      return HTTPStatus{.code = code, .phrase = {}, .wire = {}};
  }
}

} // namespace sourcemeta::core

#endif
