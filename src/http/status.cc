#include <sourcemeta/jsonschema/http_status.h>

#include <cassert>     // assert
#include <ostream>     // std::ostream
#include <type_traits> // std::underlying_type_t

namespace sourcemeta::jsonschema::http {

auto operator<<(std::ostream &stream, const Status value) -> std::ostream & {
  switch (value) {
    case Status::CONTINUE:
      return stream << "100 Continue";
    case Status::SWITCHING_PROTOCOLS:
      return stream << "101 Switching Protocols";
    case Status::PROCESSING:
      return stream << "102 Processing";
    case Status::EARLY_HINTS:
      return stream << "103 Early Hints";
    case Status::OK:
      return stream << "200 OK";
    case Status::CREATED:
      return stream << "201 Created";
    case Status::ACCEPTED:
      return stream << "202 Accepted";
    case Status::NON_AUTHORITATIVE_INFORMATION:
      return stream << "203 Non-Authoritative Information";
    case Status::NO_CONTENT:
      return stream << "204 No Content";
    case Status::RESET_CONTENT:
      return stream << "205 Reset Content";
    case Status::PARTIAL_CONTENT:
      return stream << "206 Partial Content";
    case Status::MULTI_STATUS:
      return stream << "207 Multi-Status";
    case Status::ALREADY_REPORTED:
      return stream << "208 Already Reported";
    case Status::IM_USED:
      return stream << "226 IM Used";
    case Status::MULTIPLE_CHOICES:
      return stream << "300 Multiple Choices";
    case Status::MOVED_PERMANENTLY:
      return stream << "301 Moved Permanently";
    case Status::FOUND:
      return stream << "302 Found";
    case Status::SEE_OTHER:
      return stream << "303 See Other";
    case Status::NOT_MODIFIED:
      return stream << "304 Not Modified";
    case Status::USE_PROXY:
      return stream << "305 Use Proxy";
    case Status::TEMPORARY_REDIRECT:
      return stream << "307 Temporary Redirect";
    case Status::PERMANENT_REDIRECT:
      return stream << "308 Permanent Redirect";
    case Status::BAD_REQUEST:
      return stream << "400 Bad Request";
    case Status::UNAUTHORIZED:
      return stream << "401 Unauthorized";
    case Status::PAYMENT_REQUIRED:
      return stream << "402 Payment Required";
    case Status::FORBIDDEN:
      return stream << "403 Forbidden";
    case Status::NOT_FOUND:
      return stream << "404 Not Found";
    case Status::METHOD_NOT_ALLOWED:
      return stream << "405 Method Not Allowed";
    case Status::NOT_ACCEPTABLE:
      return stream << "406 Not Acceptable";
    case Status::PROXY_AUTHENTICATION_REQUIRED:
      return stream << "407 Proxy Authentication Required";
    case Status::REQUEST_TIMEOUT:
      return stream << "408 Request Timeout";
    case Status::CONFLICT:
      return stream << "409 Conflict";
    case Status::GONE:
      return stream << "410 Gone";
    case Status::LENGTH_REQUIRED:
      return stream << "411 Length Required";
    case Status::PRECONDITION_FAILED:
      return stream << "412 Precondition Failed";
    case Status::PAYLOAD_TOO_LARGE:
      return stream << "413 Payload Too Large";
    case Status::URI_TOO_LONG:
      return stream << "414 URI Too Long";
    case Status::UNSUPPORTED_MEDIA_TYPE:
      return stream << "415 Unsupported Media Type";
    case Status::RANGE_NOT_SATISFIABLE:
      return stream << "416 Range Not Satisfiable";
    case Status::EXPECTATION_FAILED:
      return stream << "417 Expectation Failed";
    case Status::IM_A_TEAPOT:
      return stream << "418 I'm a Teapot";
    case Status::MISDIRECTED_REQUEST:
      return stream << "421 Misdirected Request";
    case Status::UNPROCESSABLE_CONTENT:
      return stream << "422 Unprocessable Content";
    case Status::LOCKED:
      return stream << "423 Locked";
    case Status::FAILED_DEPENDENCY:
      return stream << "424 Failed Dependency";
    case Status::TOO_EARLY:
      return stream << "425 Too Early";
    case Status::UPGRADE_REQUIRED:
      return stream << "426 Upgrade Required";
    case Status::PRECONDITION_REQUIRED:
      return stream << "428 Precondition Required";
    case Status::TOO_MANY_REQUESTS:
      return stream << "429 Too Many Requests";
    case Status::REQUEST_HEADER_FIELDS_TOO_LARGE:
      return stream << "431 Request Header Fields Too Large";
    case Status::UNAVAILABLE_FOR_LEGAL_REASONS:
      return stream << "451 Unavailable For Legal Reasons";
    case Status::INTERNAL_SERVER_ERROR:
      return stream << "500 Internal Server Error";
    case Status::NOT_IMPLEMENTED:
      return stream << "501 Not Implemented";
    case Status::BAD_GATEWAY:
      return stream << "502 Bad Gateway";
    case Status::SERVICE_UNAVAILABLE:
      return stream << "503 Service Unavailable";
    case Status::GATEWAY_TIMEOUT:
      return stream << "504 Gateway Timeout";
    case Status::HTTP_VERSION_NOT_SUPPORTED:
      return stream << "505 HTTP Version Not Supported";
    case Status::VARIANT_ALSO_NEGOTIATES:
      return stream << "506 Variant Also Negotiates";
    case Status::INSUFFICIENT_STORAGE:
      return stream << "507 Insufficient Storage";
    case Status::LOOP_DETECTED:
      return stream << "508 Loop Detected";
    case Status::NOT_EXTENDED:
      return stream << "510 Not Extended";
    case Status::NETWORK_AUTHENTICATION_REQUIRED:
      return stream << "511 Network Authentication Required";
    default:
      // In theory should never happen, but let is go through
      // in some sensible way in production.
      assert(false);
      return stream << static_cast<std::underlying_type_t<Status>>(value)
                    << " Unknown";
  }
}

} // namespace sourcemeta::jsonschema::http
