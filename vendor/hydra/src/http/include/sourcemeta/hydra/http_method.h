#ifndef SOURCEMETA_HYDRA_HTTP_METHOD_H
#define SOURCEMETA_HYDRA_HTTP_METHOD_H

#ifndef SOURCEMETA_HYDRA_HTTP_EXPORT
#include <sourcemeta/hydra/http_export.h>
#endif

#include <ostream>     // std::ostream
#include <string_view> // std::string_view

namespace sourcemeta::hydra::http {

/// @ingroup http
/// The list of possible HTTP methods.
enum class Method {
  GET,
  HEAD,
  POST,
  PUT,
  DELETE,
  CONNECT,
  OPTIONS,
  TRACE,
  PATCH
};

/// @ingroup http
/// Pipe a method into an output string. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::ostringstream output;
/// output << sourcemeta::hydra::http::Method::GET;
/// assert(output.str() == "GET");
/// ```
auto SOURCEMETA_HYDRA_HTTP_EXPORT operator<<(std::ostream &stream,
                                             const Method method)
    -> std::ostream &;

/// @ingroup http
/// Construct a HTTP method from a string. Note that casing is not supported.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/http.h>
/// #include <cassert>
///
/// const sourcemeta::hydra::http::Method method =
///   sourcemeta::hydra::http::to_method("GET");
/// assert(method == sourcemeta::hydra::http::Method::GET);
/// ```
auto SOURCEMETA_HYDRA_HTTP_EXPORT to_method(std::string_view method) -> Method;

} // namespace sourcemeta::hydra::http

#endif
