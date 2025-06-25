#ifndef SOURCEMETA_CORE_JSONSCHEMA_RESOLVER_H_
#define SOURCEMETA_CORE_JSONSCHEMA_RESOLVER_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema_types.h>

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

  /// Register a schema to the map resolver. Returns whether at least one
  /// schema was imported into the resolver
  auto add(const JSON &schema,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt,
           const std::function<void(const JSON::String &)> &callback = nullptr)
      -> bool;

  /// Attempt to resolve a schema
  auto operator()(std::string_view identifier) const -> std::optional<JSON>;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  std::map<std::string, JSON> schemas{};
  SchemaResolver default_resolver = nullptr;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
