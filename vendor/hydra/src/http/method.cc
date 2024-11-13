#include <sourcemeta/hydra/http_method.h>

#include <cassert>   // assert
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::invalid_argument

namespace sourcemeta::hydra::http {

auto operator<<(std::ostream &stream, const Method method) -> std::ostream & {
  switch (method) {
    case Method::GET:
      stream << "GET";
      return stream;
    case Method::HEAD:
      stream << "HEAD";
      return stream;
    case Method::POST:
      stream << "POST";
      return stream;
    case Method::PUT:
      stream << "PUT";
      return stream;
    case Method::DELETE:
      stream << "DELETE";
      return stream;
    case Method::CONNECT:
      stream << "CONNECT";
      return stream;
    case Method::OPTIONS:
      stream << "OPTIONS";
      return stream;
    case Method::TRACE:
      stream << "TRACE";
      return stream;
    case Method::PATCH:
      stream << "PATCH";
      return stream;
  }

  // Should never happen
  assert(false);
  return stream;
}

auto to_method(std::string_view method) -> Method {
  if (method == "GET" || method == "get") {
    return Method::GET;
  } else if (method == "HEAD" || method == "head") {
    return Method::HEAD;
  } else if (method == "POST" || method == "post") {
    return Method::POST;
  } else if (method == "PUT" || method == "put") {
    return Method::PUT;
  } else if (method == "DELETE" || method == "delete") {
    return Method::DELETE;
  } else if (method == "CONNECT" || method == "connect") {
    return Method::CONNECT;
  } else if (method == "OPTIONS" || method == "options") {
    return Method::OPTIONS;
  } else if (method == "TRACE" || method == "trace") {
    return Method::TRACE;
  } else if (method == "PATCH" || method == "patch") {
    return Method::PATCH;
  } else {
    std::ostringstream error;
    error << "Invalid HTTP method: " << method;
    throw std::invalid_argument(error.str());
  }
}

} // namespace sourcemeta::hydra::http
