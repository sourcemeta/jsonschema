#ifndef SOURCEMETA_JSONSCHEMA_HTTP_METHOD_H
#define SOURCEMETA_JSONSCHEMA_HTTP_METHOD_H

namespace sourcemeta::jsonschema::http {

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

} // namespace sourcemeta::jsonschema::http

#endif
