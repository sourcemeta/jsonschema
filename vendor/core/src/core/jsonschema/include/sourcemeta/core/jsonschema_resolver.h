#ifndef SOURCEMETA_CORE_JSONSCHEMA_RESOLVER_H_
#define SOURCEMETA_CORE_JSONSCHEMA_RESOLVER_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema_types.h>

#include <filesystem>  // std::filesystem
#include <functional>  // std::function
#include <map>         // std::map
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jsonschema
/// This is a convenient helper for constructing schema resolvers at runtime.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// // (1) Create a map resolver that falls back to the official resolver
/// sourcemeta::core::SchemaMapResolver
///   resolver{sourcemeta::core::schema_official_resolver};
///
/// const sourcemeta::core::JSON schema =
///   sourcemeta::core::parse_json(R"JSON({
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
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaMapResolver {
public:
  /// Construct an empty resolver. If you don't add schemas to it, it will
  /// always resolve to nothing
  SchemaMapResolver();

  /// Construct an empty resolver that has another schema resolver as a fallback
  SchemaMapResolver(const SchemaResolver &resolver);

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
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// // (1) Create a flat file resolver that falls back to the official resolver
/// sourcemeta::core::SchemaFlatFileResolver
///   resolver{sourcemeta::core::schema_official_resolver};
///
/// // (2) Register a schema by path
/// resolver.add("path/to/example.schema.json");
///
/// assert(resolver("https://www.example.com").has_value());
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaFlatFileResolver {
public:
  /// Construct an empty resolver. If you don't add schemas to it, it will
  /// always resolve to nothing
  SchemaFlatFileResolver();

  /// Construct an empty resolver that has another schema resolver as a fallback
  SchemaFlatFileResolver(const SchemaResolver &resolver);

  /// Determines how to access the registered file entry, letting you hook
  /// into how schemas are read to support other file formats, like YAML
  using Reader = std::function<JSON(const std::filesystem::path &)>;

  /// Register a schema to the flat file resolver, returning the detected
  /// identifier for the schema
  auto add(const std::filesystem::path &path,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt,
           const Reader &reader = read_json,
           SchemaVisitorReference &&reference_visitor = nullptr)
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
    Reader reader;
    SchemaVisitorReference reference_visitor;
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

} // namespace sourcemeta::core

#endif
