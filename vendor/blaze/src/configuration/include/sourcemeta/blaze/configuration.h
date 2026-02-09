#ifndef SOURCEMETA_BLAZE_CONFIGURATION_H_
#define SOURCEMETA_BLAZE_CONFIGURATION_H_

/// @defgroup configuration Configuration
/// @brief A configuration manifest for JSON Schema projects
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/configuration.h>
/// ```

#ifndef SOURCEMETA_BLAZE_CONFIGURATION_EXPORT
#include <sourcemeta/blaze/configuration_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/configuration_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>

#include <filesystem>    // std::filesystem
#include <optional>      // std::optional
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup configuration
/// A configuration file format for JSON Schema projects
struct SOURCEMETA_BLAZE_CONFIGURATION_EXPORT Configuration {
  std::optional<sourcemeta::core::JSON::String> title;
  std::optional<sourcemeta::core::JSON::String> description;
  std::optional<sourcemeta::core::JSON::String> email;
  std::optional<sourcemeta::core::JSON::String> github;
  std::optional<sourcemeta::core::JSON::String> website;
  std::filesystem::path absolute_path;
  sourcemeta::core::JSON::String base;
  std::optional<sourcemeta::core::JSON::String> default_dialect;
  std::unordered_set<sourcemeta::core::JSON::String> extension;
  std::unordered_map<sourcemeta::core::JSON::String,
                     sourcemeta::core::JSON::String>
      resolve;
  sourcemeta::core::JSON extra = sourcemeta::core::JSON::make_object();

  /// Check if the given path represents a schema described by this
  /// configuration
  [[nodiscard]]
  auto applies_to(const std::filesystem::path &path) const -> bool;

  /// Parse a configuration file from its contents
  [[nodiscard]]
  static auto from_json(const sourcemeta::core::JSON &value,
                        const std::filesystem::path &base_path)
      -> Configuration;

  /// Read and parse a configuration file from disk
  [[nodiscard]]
  static auto read_json(const std::filesystem::path &path) -> Configuration;

  /// A nearest ancestor configuration lookup
  [[nodiscard]]
  static auto find(const std::filesystem::path &path)
      -> std::optional<std::filesystem::path>;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
