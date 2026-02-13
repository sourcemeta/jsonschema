#ifndef SOURCEMETA_JSONSCHEMA_CLI_CONFIGURATION_H_
#define SOURCEMETA_JSONSCHEMA_CLI_CONFIGURATION_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/options.h>

#include "error.h"
#include "logger.h"

#include <filesystem> // std::filesystem
#include <map>        // std::map
#include <optional>   // std::optional
#include <sstream>    // std::ostringstream
#include <string>     // std::string

namespace {
auto configuration_reader(const std::filesystem::path &path) -> std::string {
  auto stream{sourcemeta::core::read_file(path)};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}
} // namespace

namespace sourcemeta::jsonschema {

inline auto find_configuration(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  return sourcemeta::blaze::Configuration::find(path);
}

inline auto read_configuration(
    const sourcemeta::core::Options &options,
    const std::optional<std::filesystem::path> &configuration_path,
    const std::optional<std::filesystem::path> &schema_path = std::nullopt)
    -> const std::optional<sourcemeta::blaze::Configuration> & {
  using CacheKey = std::optional<std::filesystem::path>;
  static std::map<CacheKey, std::optional<sourcemeta::blaze::Configuration>>
      configuration_cache;

  // Check if configuration is already cached for this path
  auto iterator{configuration_cache.find(configuration_path)};
  if (iterator != configuration_cache.end()) {
    return iterator->second;
  }

  // Compute and cache the configuration
  std::optional<sourcemeta::blaze::Configuration> result{std::nullopt};
  if (configuration_path.has_value()) {
    LOG_DEBUG(options) << "Using configuration file: "
                       << sourcemeta::core::weakly_canonical(
                              configuration_path.value())
                              .string()
                       << "\n";
    try {
      result = sourcemeta::blaze::Configuration::read_json(
          configuration_path.value(), configuration_reader);
    } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
      throw FileError<sourcemeta::blaze::ConfigurationParseError>(
          configuration_path.value(), error);
    }

    assert(result.has_value());
    if (schema_path.has_value() &&
        !result.value().applies_to(schema_path.value())) {
      LOG_DEBUG(options)
          << "Ignoring configuration file given extensions mismatch: "
          << sourcemeta::core::weakly_canonical(configuration_path.value())
                 .string()
          << "\n";
      result = std::nullopt;
    }
  }

  auto [inserted_iterator, inserted] =
      configuration_cache.emplace(configuration_path, std::move(result));
  return inserted_iterator->second;
}

} // namespace sourcemeta::jsonschema

#endif
