#ifndef SOURCEMETA_JSONSCHEMA_CLI_CONFIGURATION_H_
#define SOURCEMETA_JSONSCHEMA_CLI_CONFIGURATION_H_

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/schemaconfig.h>

#include "error.h"
#include "logger.h"

#include <filesystem> // std::filesystem
#include <map>        // std::map
#include <optional>   // std::optional

namespace sourcemeta::jsonschema {

inline auto find_configuration(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  return sourcemeta::core::SchemaConfig::find(path);
}

inline auto read_configuration(
    const sourcemeta::core::Options &options,
    const std::optional<std::filesystem::path> &configuration_path)
    -> const std::optional<sourcemeta::core::SchemaConfig> & {
  using CacheKey = std::optional<std::filesystem::path>;
  static std::map<CacheKey, std::optional<sourcemeta::core::SchemaConfig>>
      configuration_cache;

  // Check if configuration is already cached for this path
  auto iterator{configuration_cache.find(configuration_path)};
  if (iterator != configuration_cache.end()) {
    return iterator->second;
  }

  // Compute and cache the configuration
  std::optional<sourcemeta::core::SchemaConfig> result{std::nullopt};
  if (configuration_path.has_value()) {
    LOG_VERBOSE(options) << "Using configuration file: "
                         << sourcemeta::core::weakly_canonical(
                                configuration_path.value())
                                .string()
                         << "\n";
    try {
      result =
          sourcemeta::core::SchemaConfig::read_json(configuration_path.value());
    } catch (const sourcemeta::core::SchemaConfigParseError &error) {
      throw FileError<sourcemeta::core::SchemaConfigParseError>(
          configuration_path.value(), error);
    }
  }

  auto [inserted_iterator, inserted] =
      configuration_cache.emplace(configuration_path, std::move(result));
  return inserted_iterator->second;
}

} // namespace sourcemeta::jsonschema

#endif
