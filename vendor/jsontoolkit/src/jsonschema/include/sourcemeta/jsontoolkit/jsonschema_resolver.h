#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_RESOLVER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_RESOLVER_H_

#if defined(__EMSCRIPTEN__) || defined(__Unikraft__)
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#else
#include "jsonschema_export.h"
#endif

#include <sourcemeta/jsontoolkit/json.h>

#include <functional>  // std::function
#include <future>      // std::future
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::jsontoolkit {

// Take a URI and get back a schema
/// @ingroup jsonschema
///
/// Some functions need to reference other schemas by their URIs. To accomplish
/// this in a generic and flexible way, these functions take resolver functions
/// as arguments, of the type sourcemeta::jsontoolkit::SchemaResolver.
///
/// For convenience, we provide the following default resolvers:
///
/// - sourcemeta::jsontoolkit::official_resolver
///
/// You can implement resolvers to read from a local storage, to send HTTP
/// requests, or anything your application might require. Unless your resolver
/// is trivial, it is recommended to create a callable object that implements
/// the function interface.
using SchemaResolver =
    std::function<std::future<std::optional<JSON>>(std::string_view)>;

/// @ingroup jsonschema
/// A default resolver that relies on built-in official schemas.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto official_resolver(std::string_view identifier)
    -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>>;

} // namespace sourcemeta::jsontoolkit

#endif
