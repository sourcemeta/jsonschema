#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_RESOLVER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_RESOLVER_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#include <sourcemeta/jsontoolkit/jsonschema_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>

#include <functional>  // std::function
#include <map>         // std::map
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
using SchemaResolver = std::function<std::optional<JSON>(std::string_view)>;

/// @ingroup jsonschema
/// A default resolver that relies on built-in official schemas.
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto official_resolver(std::string_view identifier)
    -> std::optional<sourcemeta::jsontoolkit::JSON>;

/// @ingroup jsonschema
/// This is a convenient helper for constructing schema resolvers at runtime.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
/// #include <utility>
///
/// // (1) Create a map resolver that falls back to the official resolver
/// sourcemeta::jsontoolkit::MapSchemaResolver
///   resolver{sourcemeta::jsontoolkit::official_resolver};
///
/// const sourcemeta::jsontoolkit::JSON schema =
///   sourcemeta::jsontoolkit::parse(R"JSON({
///   "$id": "https://www.example.com"
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "type": "string"
/// })JSON");
///
/// // (2) Register a schema
/// resolver.add(schema);
///
/// assert(resolver("https://www.example.com").has_value());
/// ```
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT MapSchemaResolver {
public:
  /// Construct an empty map resolver. If you don't add schemas to it, it will
  /// always resolve to nothing
  MapSchemaResolver();

  /// Construct an empty map resolver that has another schema resolver as a
  /// fallback
  MapSchemaResolver(const SchemaResolver &resolver);

  /// Register a schema to the map resolver
  auto add(const JSON &schema,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt) -> void;

  /// Attempt to resolve a schema
  auto operator()(std::string_view identifier) const -> std::optional<JSON>;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::map<std::string, JSON> schemas;
  SchemaResolver default_resolver = nullptr;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::jsontoolkit

#endif
