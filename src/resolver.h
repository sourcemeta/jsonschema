#ifndef SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_
#define SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_

#ifndef NOMINMAX
#define NOMINMAX
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnewline-eof"
#endif
#include <cpr/cpr.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include "error.h"
#include "input.h"
#include "logger.h"
#include "utils.h"

#include <cassert>     // assert
#include <chrono>      // std::chrono::seconds
#include <cstdint>     // std::uint8_t
#include <filesystem>  // std::filesystem
#include <functional>  // std::function, std::ref
#include <iostream>    // std::cerr
#include <map>         // std::map
#include <optional>    // std::optional
#include <sstream>     // std::ostringstream
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread::sleep_for
#include <utility> // std::pair, std::piecewise_construct, std::forward_as_tuple

namespace sourcemeta::jsonschema {

static constexpr std::uint8_t HTTP_MAXIMUM_RETRIES{3};

static inline auto find_resolve_match(
    const std::unordered_map<std::string, std::string> &resolve_map,
    const std::string &identifier)
    -> std::unordered_map<std::string, std::string>::const_iterator {
  auto match{resolve_map.find(identifier)};
  if (match == resolve_map.cend() && !identifier.ends_with(".json")) {
    match = resolve_map.find(identifier + ".json");
  }
  if (match == resolve_map.cend() && identifier.ends_with(".json")) {
    match = resolve_map.find(identifier.substr(0, identifier.size() - 5));
  }
  return match;
}

static inline auto http_fetch(const std::string &url,
                              const sourcemeta::core::Options &options)
    -> sourcemeta::core::JSON {
  cpr::Response response;
  for (std::uint8_t attempt{1}; attempt <= HTTP_MAXIMUM_RETRIES; ++attempt) {
    LOG_VERBOSE(options) << "Resolving over HTTP (attempt "
                         << static_cast<int>(attempt) << "/"
                         << static_cast<int>(HTTP_MAXIMUM_RETRIES)
                         << "): " << url << "\n";
    response = cpr::Get(cpr::Url{url}, cpr::Redirect{true});

    if (response.status_code == 200) {
      break;
    }

    if (attempt < HTTP_MAXIMUM_RETRIES) {
      LOG_VERBOSE(options) << "Request failed with HTTP "
                           << response.status_code << ", retrying...\n";
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  if (response.status_code != 200) {
    std::ostringstream error;
    error << "HTTP " << response.status_code << "\n  at " << url;
    throw std::runtime_error(error.str());
  }

  const auto content_type_iterator{response.header.find("content-type")};
  if (content_type_iterator != response.header.end() &&
      content_type_iterator->second.starts_with("text/yaml")) {
    return sourcemeta::core::parse_yaml(response.text);
  }

  return sourcemeta::core::parse_json(response.text);
}

static inline auto fallback_resolver(const sourcemeta::core::Options &options,
                                     std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON> {
  auto official_result{sourcemeta::core::schema_resolver(identifier)};
  if (official_result.has_value()) {
    return official_result;
  }

  // If the URI is not an HTTP URL, then abort
  const sourcemeta::core::URI uri{std::string{identifier}};
  const auto maybe_scheme{uri.scheme()};
  if (uri.is_urn() || !maybe_scheme.has_value() ||
      (maybe_scheme.value() != "https" && maybe_scheme.value() != "http")) {
    return std::nullopt;
  }

  return http_fetch(std::string{identifier}, options);
}

class CustomResolver {
public:
  CustomResolver(
      const sourcemeta::core::Options &options,
      const std::optional<sourcemeta::blaze::Configuration> &configuration,
      const bool remote, const std::string_view default_dialect)
      : options_{options}, configuration_{configuration}, remote_{remote} {
    if (options.contains("resolve")) {
      for (const auto &entry : for_each_json(options.at("resolve"), options)) {
        LOG_DEBUG(options) << "Detecting schema resources from file: "
                           << entry.first.string() << "\n";

        if (!sourcemeta::core::is_schema(entry.second)) {
          throw FileError<sourcemeta::core::SchemaError>(
              entry.first,
              "The file you provided does not represent a valid JSON Schema");
        }

        try {
          const auto result = this->add(
              entry.second, default_dialect,
              sourcemeta::jsonschema::default_id(entry.first),
              [&options](const auto &identifier) {
                LOG_DEBUG(options)
                    << "Importing schema into the resolution context: "
                    << identifier << "\n";
              });
          if (!result) {
            LOG_WARNING()
                << "No schema resources were imported from this file\n"
                << "  at " << entry.first.string() << "\n"
                << "Are you sure this schema sets any identifiers?\n";
          }
        } catch (const sourcemeta::core::SchemaFrameError &error) {
          throw FileError<sourcemeta::core::SchemaFrameError>(
              entry.first, std::string{error.identifier()}, error.what());
        } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
          throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
              entry.first);
        }
      }
    }

    if (this->configuration_.has_value()) {
      for (const auto &[dependency_uri, dependency_path] :
           this->configuration_.value().dependencies) {
        if (!std::filesystem::exists(dependency_path)) {
          continue;
        }

        auto schema{sourcemeta::core::read_json(dependency_path)};
        if (!sourcemeta::core::is_schema(schema)) {
          continue;
        }

        try {
          this->add(schema, default_dialect);
        } catch (...) {
          continue;
        }

        this->schemas.emplace(dependency_uri, schema);
      }
    }
  }

  auto add(const sourcemeta::core::JSON &schema,
           const std::string_view default_dialect = "",
           const std::string_view default_id = "",
           const std::function<void(const sourcemeta::core::JSON::String &)>
               &callback = nullptr) -> bool {
    assert(sourcemeta::core::is_schema(schema));

    // Registering the top-level schema is not enough. We need to check
    // and register every embedded schema resource too
    sourcemeta::core::SchemaFrame frame{
        sourcemeta::core::SchemaFrame::Mode::References};
    frame.analyse(schema, sourcemeta::core::schema_walker, *this,
                  default_dialect, default_id);

    bool added_any_schema{false};
    for (const auto &[key, entry] : frame.locations()) {
      if (entry.type != sourcemeta::core::SchemaFrame::LocationType::Resource) {
        continue;
      }

      auto subschema{sourcemeta::core::get(schema, entry.pointer)};
      const auto subschema_vocabularies{frame.vocabularies(entry, *this)};

      // Given we might be resolving embedded resources, we fully
      // resolve their dialect and identifiers, otherwise the
      // consumer might have no idea what to do with them
      subschema.assign("$schema", sourcemeta::core::JSON{entry.dialect});
      sourcemeta::core::reidentify(subschema, key.second, entry.base_dialect);

      const auto result{this->schemas.emplace(key.second, subschema)};
      if (!result.second && result.first->second != schema) {
        throw sourcemeta::core::SchemaFrameError(
            key.second, "Cannot register the same identifier twice");
      }

      if (callback) {
        callback(key.second);
      }

      added_any_schema = true;
    }

    return added_any_schema;
  }

  auto operator()(std::string_view identifier) const
      -> std::optional<sourcemeta::core::JSON> {
    const std::string string_identifier{identifier};
    if (this->configuration_.has_value()) {

      // TODO: Abstract this fallback logic as a Configuration method
      const auto match{find_resolve_match(this->configuration_.value().resolve,
                                          string_identifier)};
      if (match != this->configuration_.value().resolve.cend()) {
        const sourcemeta::core::URI new_uri{match->second};
        if (new_uri.is_relative()) {
          const auto file_uri{sourcemeta::core::URI::from_path(
              this->configuration_.value().absolute_path / new_uri.to_path())};
          const auto result{file_uri.recompose()};
          LOG_DEBUG(this->options_)
              << "Resolving " << identifier << " as " << result
              << " given the configuration file\n";
          return this->operator()(result);
        } else {
          LOG_DEBUG(this->options_)
              << "Resolving " << identifier << " as " << match->second
              << " given the configuration file\n";
          return this->operator()(match->second);
        }
      }
    }

    if (this->schemas.contains(string_identifier)) {
      return this->schemas.at(string_identifier);
    }

    // Fallback resolution logic
    sourcemeta::core::URI uri;
    try {
      uri = sourcemeta::core::URI{std::string{identifier}};
    } catch (const sourcemeta::core::URIParseError &) {
      return std::nullopt;
    }

    if (uri.is_file()) {
      const auto path{uri.to_path()};
      LOG_DEBUG(this->options_)
          << "Attempting to read file reference from disk: " << path.string()
          << "\n";
      if (std::filesystem::exists(path)) {
        return std::optional<sourcemeta::core::JSON>{
            sourcemeta::core::read_yaml_or_json(path)};
      }
    }

    if (this->remote_) {
      return fallback_resolver(this->options_, identifier);
    } else {
      return sourcemeta::core::schema_resolver(identifier);
    }
  }

private:
  std::map<std::string, sourcemeta::core::JSON> schemas{};
  const sourcemeta::core::Options &options_;
  const std::optional<sourcemeta::blaze::Configuration> configuration_;
  bool remote_{false};
};

inline auto
resolver(const sourcemeta::core::Options &options, const bool remote,
         const std::string_view default_dialect,
         const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> const CustomResolver & {
  using CacheKey = std::pair<bool, std::string>;
  static std::map<CacheKey, CustomResolver> resolver_cache;
  const CacheKey cache_key{remote, std::string{default_dialect}};

  // Check if resolver is already cached
  auto iterator{resolver_cache.find(cache_key)};
  if (iterator != resolver_cache.end()) {
    return iterator->second;
  }

  // Construct resolver directly in cache
  auto [inserted_iterator, inserted] = resolver_cache.emplace(
      std::piecewise_construct, std::forward_as_tuple(cache_key),
      std::forward_as_tuple(options, configuration, remote, default_dialect));
  return inserted_iterator->second;
}

} // namespace sourcemeta::jsonschema

#endif
