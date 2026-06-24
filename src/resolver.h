#ifndef SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_
#define SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>
#include <sourcemeta/core/http.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
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
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread::sleep_for
#include <utility> // std::pair, std::piecewise_construct, std::forward_as_tuple
#include <vector>  // std::vector

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

static inline auto
resolve_map_uri(const sourcemeta::blaze::Configuration &configuration,
                const std::string &identifier) -> std::optional<std::string> {
  const auto match{find_resolve_match(configuration.resolve, identifier)};
  if (match == configuration.resolve.cend()) {
    return std::nullopt;
  }

  return resolve_relative_uri(match->second, configuration.base_path);
}

static constexpr std::string_view HTTP_HEADER_EXAMPLE{
    "--header \"Authorization: Bearer ${TOKEN}\""};

static inline auto parse_http_header(const std::string_view input)
    -> std::pair<std::string_view, std::string_view> {
  const auto colon{input.find(':')};
  if (colon == std::string_view::npos) {
    throw PositionalArgumentError{
        "HTTP headers must be in the form `Name: Value`",
        std::string{HTTP_HEADER_EXAMPLE}};
  }

  const auto raw_name{input.substr(0, colon)};
  if (raw_name.empty()) {
    throw PositionalArgumentError{"HTTP header names cannot be empty",
                                  std::string{HTTP_HEADER_EXAMPLE}};
  }

  for (const auto character : raw_name) {
    if (character == ' ' || character == '\t') {
      throw PositionalArgumentError{
          "HTTP header names cannot contain whitespace",
          std::string{HTTP_HEADER_EXAMPLE}};
    }
    if (static_cast<unsigned char>(character) < 0x20 ||
        static_cast<unsigned char>(character) == 0x7F) {
      throw PositionalArgumentError{
          "HTTP header names cannot contain control characters",
          std::string{HTTP_HEADER_EXAMPLE}};
    }
  }

  auto raw_value{input.substr(colon + 1)};
  while (!raw_value.empty() &&
         (raw_value.front() == ' ' || raw_value.front() == '\t')) {
    raw_value.remove_prefix(1);
  }

  for (const auto character : raw_value) {
    if (character == '\r' || character == '\n' || character == '\0') {
      throw PositionalArgumentError{
          "HTTP header values cannot contain control characters",
          std::string{HTTP_HEADER_EXAMPLE}};
    }
  }

  return {raw_name, raw_value};
}

static inline auto
validate_http_headers(const sourcemeta::core::Options &options) -> void {
  if (!options.contains("header")) {
    return;
  }
  for (const auto &raw : options.at("header")) {
    parse_http_header(raw);
  }
}

static inline auto
collect_http_headers(const sourcemeta::core::Options &options)
    -> std::vector<std::pair<std::string_view, std::string_view>> {
  std::vector<std::pair<std::string_view, std::string_view>> headers;
  if (!options.contains("header")) {
    return headers;
  }
  for (const auto &raw : options.at("header")) {
    headers.emplace_back(parse_http_header(raw));
  }
  return headers;
}

static inline auto http_fetch(const std::string &url,
                              const sourcemeta::core::Options &options)
    -> sourcemeta::core::JSON {
  sourcemeta::core::HTTPSystemRequest request{url};
  for (const auto &header : collect_http_headers(options)) {
    request.header(std::string{header.first}, std::string{header.second});
  }

  sourcemeta::core::HTTPResponse response;
  for (std::uint8_t attempt{1}; attempt <= HTTP_MAXIMUM_RETRIES; ++attempt) {
    LOG_VERBOSE(options) << "Resolving over HTTP (attempt "
                         << static_cast<int>(attempt) << "/"
                         << static_cast<int>(HTTP_MAXIMUM_RETRIES)
                         << "): " << url << "\n";
    try {
      response = request.send();
    } catch (const sourcemeta::core::HTTPError &error) {
      if (attempt == HTTP_MAXIMUM_RETRIES) {
        throw;
      }

      LOG_VERBOSE(options) << "Request failed (" << error.what()
                           << "), retrying...\n";
      std::this_thread::sleep_for(std::chrono::seconds(1));
      continue;
    }

    if (response.status == sourcemeta::core::HTTP_STATUS_OK) {
      break;
    }

    if (attempt < HTTP_MAXIMUM_RETRIES) {
      LOG_VERBOSE(options) << "Request failed with HTTP "
                           << response.status.code << ", retrying...\n";
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  if (response.status != sourcemeta::core::HTTP_STATUS_OK) {
    throw sourcemeta::core::HTTPStatusError{sourcemeta::core::HTTPMethod::GET,
                                            url, response.status};
  }

  const auto content_type{
      sourcemeta::core::http_header_find(response.headers, "content-type")};
  if (content_type.has_value() && sourcemeta::core::http_content_type_matches(
                                      content_type.value(), "text/yaml")) {
    try {
      return sourcemeta::core::parse_yaml(response.body);
    } catch (const sourcemeta::core::YAMLParseError &error) {
      throw sourcemeta::core::YAMLFileParseError{url, error};
    }
  }

  return sourcemeta::core::parse_json(response.body);
}

static inline auto fetch_schema(const sourcemeta::core::Options &options,
                                std::string_view identifier,
                                const bool remote = true,
                                const bool bundle = false)
    -> std::optional<sourcemeta::core::JSON> {
  auto official_result{sourcemeta::blaze::schema_resolver(identifier)};
  if (official_result.has_value()) {
    return official_result;
  }

  sourcemeta::core::URI uri;
  try {
    uri = sourcemeta::core::URI{identifier};
  } catch (const sourcemeta::core::URIParseError &) {
    return std::nullopt;
  }

  if (uri.is_file()) {
    const auto path{uri.to_path()};
    LOG_DEBUG(options) << "Attempting to read file reference from disk: "
                       << path.string() << "\n";
    if (std::filesystem::exists(path)) {
      return sourcemeta::core::read_yaml_or_json(path);
    }

    return std::nullopt;
  }

  if (remote) {
    const auto scheme{uri.scheme()};
    if (!uri.is_urn() && scheme.has_value() &&
        (scheme.value() == "https" || scheme.value() == "http")) {
      std::string fetch_url{identifier};
      if (bundle) {
        // TODO: Use sourcemeta::core::URI to set query parameters once
        // the URI module supports setters for query strings
        if (fetch_url.find('?') != std::string::npos) {
          fetch_url += "&bundle=1";
        } else {
          fetch_url += "?bundle=1";
        }
      }

      return http_fetch(fetch_url, options);
    }
  }

  return std::nullopt;
}

static inline auto
ensure_identifier(sourcemeta::core::JSON &schema, const std::string_view target,
                  const sourcemeta::blaze::SchemaResolver &resolver) -> void {
  if (!sourcemeta::blaze::is_schema(schema) || !schema.is_object()) {
    return;
  }

  const auto resolved_base_dialect{
      sourcemeta::blaze::base_dialect(schema, resolver, "")};
  if (!resolved_base_dialect.has_value()) {
    return;
  }

  if (!sourcemeta::blaze::identify(schema, resolved_base_dialect.value())
           .empty()) {
    return;
  }

  sourcemeta::blaze::reidentify(schema, target, resolved_base_dialect.value());
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
                           << entry.first << "\n";

        if (!sourcemeta::blaze::is_schema(entry.second)) {
          throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
              entry.resolution_base,
              "The file you provided does not represent a valid JSON Schema");
        }

        try {
          const auto result = this->add(
              entry.second, default_dialect,
              sourcemeta::jsonschema::default_id(entry),
              [&options](const auto &identifier) {
                LOG_DEBUG(options)
                    << "Importing schema into the resolution context: "
                    << identifier << "\n";
              });
          if (!result) {
            LOG_WARNING()
                << "No schema resources were imported from this file\n"
                << "  at " << entry.first << "\n"
                << "Are you sure this schema sets any identifiers?\n";
          }
        } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaKeywordError>(entry.resolution_base,
                                                     error);
        } catch (const sourcemeta::blaze::SchemaFrameError &error) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaFrameError>(
              entry.resolution_base, error.identifier(), error.what());
        } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
          const auto position{entry.positions.get(error.location())};
          if (position.has_value()) {
            throw PositionError<sourcemeta::core::FileError<
                sourcemeta::blaze::SchemaAnchorCollisionError>>(
                std::get<0>(position.value()), std::get<1>(position.value()),
                entry.resolution_base, error);
          }

          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaAnchorCollisionError>(
              entry.resolution_base, error);
        } catch (const sourcemeta::blaze::SchemaReferenceError &error) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaReferenceError>(
              entry.resolution_base, error.identifier(), error.location(),
              error.what());
        } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaUnknownBaseDialectError>(
              entry.resolution_base);
        } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaUnknownDialectError>(
              entry.resolution_base);
        } catch (
            const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError
                &error) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
              entry.resolution_base, error);
        } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
          throw sourcemeta::core::FileError<
              sourcemeta::blaze::SchemaResolutionError>(
              entry.resolution_base, error.identifier(), error.what());
        } catch (const sourcemeta::blaze::SchemaError &error) {
          throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
              entry.resolution_base, error.what());
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
        if (!sourcemeta::blaze::is_schema(schema)) {
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
    assert(sourcemeta::blaze::is_schema(schema));

    // Registering the top-level schema is not enough. We need to check
    // and register every embedded schema resource too
    sourcemeta::blaze::SchemaFrame frame{
        sourcemeta::blaze::SchemaFrame::Mode::References};
    frame.analyse(schema, sourcemeta::blaze::schema_walker, *this,
                  default_dialect, default_id);

    bool added_any_schema{false};
    for (const auto &[key, entry] : frame.locations()) {
      if (entry.type !=
          sourcemeta::blaze::SchemaFrame::LocationType::Resource) {
        continue;
      }

      auto subschema{sourcemeta::core::get(schema, entry.pointer)};
      const auto subschema_vocabularies{frame.vocabularies(entry, *this)};

      // Given we might be resolving embedded resources, we fully
      // resolve their dialect and identifiers, otherwise the
      // consumer might have no idea what to do with them
      subschema.assign("$schema", sourcemeta::core::JSON{entry.dialect});
      sourcemeta::blaze::reidentify(subschema, key.second, entry.base_dialect);

      const auto result{this->schemas.emplace(key.second, subschema)};
      if (!result.second && result.first->second != subschema) {
        throw sourcemeta::blaze::SchemaFrameError(
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
    const auto mapped_result = this->configuration_.and_then(
        [&string_identifier](const sourcemeta::blaze::Configuration &config)
            -> std::optional<std::string> {
          return resolve_map_uri(config, string_identifier);
        });
    const std::string &target{mapped_result.has_value() ? mapped_result.value()
                                                        : string_identifier};
    if (mapped_result.has_value()) {
      LOG_DEBUG(this->options_) << "Resolving " << identifier << " as "
                                << target << " given the configuration file\n";
    }

    if (this->schemas.contains(target)) {
      return this->schemas.at(target);
    }

    auto fetched{fetch_schema(this->options_, target, this->remote_)};
    if (fetched.has_value()) {
      ensure_identifier(fetched.value(), string_identifier, *this);
    }

    return fetched;
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
