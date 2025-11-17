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

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/schemaconfig.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include "error.h"
#include "input.h"
#include "logger.h"

#include <cassert>     // assert
#include <filesystem>  // std::filesystem
#include <functional>  // std::function, std::ref
#include <iostream>    // std::cerr
#include <map>         // std::map
#include <optional>    // std::optional
#include <sstream>     // std::ostringstream
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility> // std::pair, std::piecewise_construct, std::forward_as_tuple

namespace sourcemeta::jsonschema {

static inline auto fallback_resolver(const sourcemeta::core::Options &options,
                                     std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON> {
  auto official_result{sourcemeta::core::schema_official_resolver(identifier)};
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

  LOG_VERBOSE(options) << "Resolving over HTTP: " << identifier << "\n";
  const cpr::Response response{
      cpr::Get(cpr::Url{identifier}, cpr::Redirect{true})};

  if (response.status_code != 200) {
    std::ostringstream error;
    error << "HTTP " << response.status_code << "\n  at " << identifier;
    throw std::runtime_error(error.str());
  }

  const auto content_type_iterator{response.header.find("content-type")};
  if (content_type_iterator != response.header.end() &&
      content_type_iterator->second.starts_with("text/yaml")) {
    return sourcemeta::core::parse_yaml(response.text);
  } else {
    return sourcemeta::core::parse_json(response.text);
  }
}

class CustomResolver {
public:
  CustomResolver(
      const sourcemeta::core::Options &options,
      const std::optional<sourcemeta::core::SchemaConfig> &configuration,
      const bool remote, const std::optional<std::string> &default_dialect)
      : options_{options}, configuration_{configuration}, remote_{remote} {
    if (options.contains("resolve")) {
      for (const auto &entry : for_each_json(options.at("resolve"), options)) {
        LOG_VERBOSE(options)
            << "Detecting schema resources from file: " << entry.first.string()
            << "\n";

        if (!sourcemeta::core::is_schema(entry.second)) {
          throw FileError<sourcemeta::core::SchemaError>(
              entry.first,
              "The file you provided does not represent a valid JSON Schema");
        }

        const auto result =
            this->add(entry.second, default_dialect,
                      sourcemeta::core::URI::from_path(entry.first).recompose(),
                      [&options](const auto &identifier) {
                        LOG_VERBOSE(options)
                            << "Importing schema into the resolution context: "
                            << identifier << "\n";
                      });
        if (!result) {
          LOG_WARNING() << "No schema resources were imported from this file\n"
                        << "  at " << entry.first.string() << "\n"
                        << "Are you sure this schema sets any identifiers?\n";
        }
      }
    }
  }

  auto add(const sourcemeta::core::JSON &schema,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt,
           const std::function<void(const sourcemeta::core::JSON::String &)>
               &callback = nullptr) -> bool {
    assert(sourcemeta::core::is_schema(schema));

    // Registering the top-level schema is not enough. We need to check
    // and register every embedded schema resource too
    sourcemeta::core::SchemaFrame frame{
        sourcemeta::core::SchemaFrame::Mode::References};
    frame.analyse(schema, sourcemeta::core::schema_official_walker, *this,
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
        std::ostringstream error;
        error << "Cannot register the same identifier twice: " << key.second;
        throw sourcemeta::core::SchemaError(error.str());
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
      const auto match{
          this->configuration_.value().resolve.find(string_identifier)};
      if (match != this->configuration_.value().resolve.cend()) {
        const sourcemeta::core::URI new_uri{match->second};
        if (new_uri.is_relative()) {
          const auto file_uri{sourcemeta::core::URI::from_path(
              this->configuration_.value().absolute_path / new_uri.to_path())};
          const auto result{file_uri.recompose()};
          LOG_VERBOSE(this->options_)
              << "Resolving " << identifier << " as " << result
              << " given the configuration file\n";
          return this->operator()(result);
        } else {
          LOG_VERBOSE(this->options_)
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
    const sourcemeta::core::URI uri{std::string{identifier}};
    if (uri.is_file()) {
      const auto path{uri.to_path()};
      LOG_VERBOSE(this->options_)
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
      return sourcemeta::core::schema_official_resolver(identifier);
    }
  }

private:
  std::map<std::string, sourcemeta::core::JSON> schemas{};
  const sourcemeta::core::Options &options_;
  const std::optional<sourcemeta::core::SchemaConfig> configuration_;
  bool remote_{false};
};

inline auto
resolver(const sourcemeta::core::Options &options, const bool remote,
         const std::optional<std::string> &default_dialect,
         const std::optional<sourcemeta::core::SchemaConfig> &configuration)
    -> const CustomResolver & {
  using CacheKey = std::pair<bool, std::optional<std::string>>;
  static std::map<CacheKey, CustomResolver> resolver_cache;
  const CacheKey cache_key{remote, default_dialect};

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
