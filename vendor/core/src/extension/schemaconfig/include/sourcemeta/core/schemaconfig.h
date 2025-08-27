#ifndef SOURCEMETA_CORE_EXTENSION_SCHEMACONFIG_H_
#define SOURCEMETA_CORE_EXTENSION_SCHEMACONFIG_H_

/// @defgroup schemaconfig SchemaConfig
/// @brief A configuration manifest for JSON Schema projects
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/schemaconfig.h>
/// ```

#ifndef SOURCEMETA_CORE_SCHEMACONFIG_EXPORT
#include <sourcemeta/core/schemaconfig_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/schemaconfig_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>

#include <filesystem>    // std::filesystem
#include <optional>      // std::optional
#include <unordered_map> // std::unordered_map

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup schemaconfig
/// A configuration file format for JSON Schema projects
struct SOURCEMETA_CORE_SCHEMACONFIG_EXPORT SchemaConfig {
  std::optional<JSON::String> title;
  std::optional<JSON::String> description;
  std::optional<JSON::String> email;
  std::optional<JSON::String> github;
  std::optional<JSON::String> website;
  std::filesystem::path absolute_path;
  JSON::String base;
  std::optional<JSON::String> default_dialect;
  std::unordered_map<JSON::String, JSON::String> resolve;
  JSON extra = JSON::make_object();

  /// Parse a configuration file from its contents
  [[nodiscard]]
  static auto from_json(const JSON &value,
                        const std::filesystem::path &base_path) -> SchemaConfig;

  /// Read and parse a configuration file from disk
  [[nodiscard]]
  static auto read_json(const std::filesystem::path &path) -> SchemaConfig;

  /// A nearest ancestor configuration lookup
  [[nodiscard]]
  static auto find(const std::filesystem::path &path)
      -> std::optional<std::filesystem::path>;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
