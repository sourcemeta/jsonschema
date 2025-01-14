#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_RESOLVER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_RESOLVER_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#include <sourcemeta/jsontoolkit/jsonschema_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>

#include <filesystem>  // std::filesystem
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
  /// Construct an empty resolver. If you don't add schemas to it, it will
  /// always resolve to nothing
  MapSchemaResolver();

  /// Construct an empty resolver that has another schema resolver as a fallback
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

/// @ingroup jsonschema
/// A convenient dynamic schema resolver capable of storing a high number of
/// schemas by deferencing them from disk on-demand. It is called a flat
/// resolver as it only looks at the top-level identifier of every schema. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// // (1) Create a flat file resolver that falls back to the official resolver
/// sourcemeta::jsontoolkit::FlatFileSchemaResolver
///   resolver{sourcemeta::jsontoolkit::official_resolver};
///
/// // (2) Register a schema by path
/// resolver.add("path/to/example.schema.json");
///
/// assert(resolver("https://www.example.com").has_value());
/// ```
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT FlatFileSchemaResolver {
public:
  /// Construct an empty resolver. If you don't add schemas to it, it will
  /// always resolve to nothing
  FlatFileSchemaResolver();

  /// Construct an empty resolver that has another schema resolver as a fallback
  FlatFileSchemaResolver(const SchemaResolver &resolver);

  /// Register a schema to the flat file resolver, returning the detected
  /// identifier for the schema
  auto add(const std::filesystem::path &path,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt)
      -> const std::string &;

  // Change the identifier of a registered schema
  auto reidentify(const std::string &schema, const std::string &new_identifier)
      -> void;

  /// Attempt to resolve a schema
  auto operator()(std::string_view identifier) const -> std::optional<JSON>;

  /// Traverse the registered schemas using iterators
  inline auto begin() const -> auto { return this->schemas.begin(); }

  /// Traverse the registered schemas using iterators
  inline auto end() const -> auto { return this->schemas.end(); }

  /// Represent an entry in the resolver
  struct Entry {
    std::filesystem::path path;
    std::optional<std::string> default_dialect;
    std::string original_identifier;
  };

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::map<std::string, Entry> schemas;
  SchemaResolver default_resolver = nullptr;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::jsontoolkit

#endif
