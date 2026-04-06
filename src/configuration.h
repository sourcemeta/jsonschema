#ifndef SOURCEMETA_JSONSCHEMA_CLI_CONFIGURATION_H_
#define SOURCEMETA_JSONSCHEMA_CLI_CONFIGURATION_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/uri.h>

#include "error.h"
#include "logger.h"

#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <cstdint>    // std::uint64_t
#include <deque>      // std::deque
#include <filesystem> // std::filesystem
#include <map>        // std::map
#include <memory>     // std::shared_ptr, std::make_shared
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
    sourcemeta::core::PointerPositionTracker positions;
    auto property_storage = std::make_shared<std::deque<std::string>>();
    try {
      auto stream{sourcemeta::core::read_file(configuration_path.value())};
      std::ostringstream buffer;
      buffer << stream.rdbuf();
      sourcemeta::core::JSON config_json{nullptr};
      sourcemeta::core::parse_json(
          buffer.str(), config_json,
          [&positions, &property_storage](
              const sourcemeta::core::JSON::ParsePhase phase,
              const sourcemeta::core::JSON::Type type, const std::uint64_t line,
              const std::uint64_t column,
              const sourcemeta::core::JSON::ParseContext context,
              const std::size_t index,
              const sourcemeta::core::JSON::String &property) {
            property_storage->emplace_back(property);
            positions(phase, type, line, column, context, index,
                      property_storage->back());
          });
      result = sourcemeta::blaze::Configuration::from_json(
          config_json, configuration_path.value().parent_path());
    } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::ConfigurationParseError>(
          configuration_path.value(), error);
    }

    assert(result.has_value());
    for (const auto &[resolve_uri, resolve_value] : result.value().resolve) {
      const sourcemeta::core::URI value_uri{resolve_value};
      if (value_uri.is_relative()) {
        const auto resolved_path{std::filesystem::weakly_canonical(
            result.value().base_path / value_uri.to_path())};
        if (!std::filesystem::exists(resolved_path)) {
          const sourcemeta::core::Pointer resolve_pointer{"resolve",
                                                          resolve_uri};
          const auto position{positions.get(resolve_pointer)};
          assert(position.has_value());
          throw ConfigurationResolveFileNotFoundError(
              configuration_path.value(), resolve_pointer, resolved_path,
              std::get<0>(position.value()), std::get<1>(position.value()));
        }
      }
    }

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
