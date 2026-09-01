#ifndef SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_
#define SOURCEMETA_JSONSCHEMA_CLI_RESOLVER_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/blaze/foundation.h>
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

#include <cassert> // assert
#include <chrono>  // std::chrono::seconds
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t
#include <exception> // std::exception_ptr, std::current_exception, std::rethrow_exception
#include <filesystem>  // std::filesystem
#include <functional>  // std::function, std::ref
#include <iostream>    // std::cerr
#include <map>         // std::map
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread::sleep_for
#include <utility> // std::pair, std::piecewise_construct, std::forward_as_tuple, std::move
#include <vector> // std::vector

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
    -> sourcemeta::blaze::SchemaResolverResult {
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
                       << path.generic_string() << "\n";
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
anonymous_base_dialect(const sourcemeta::core::JSON &schema,
                       const sourcemeta::blaze::SchemaResolver &resolver)
    -> std::optional<sourcemeta::blaze::SchemaBaseDialect> {
  if (!schema.is_object()) {
    return std::nullopt;
  }

  try {
    const sourcemeta::blaze::SchemaFrame frame{
        sourcemeta::blaze::SchemaFrame::Mode::Root, schema,
        sourcemeta::blaze::schema_walker, resolver};
    if (!frame.root().empty()) {
      return std::nullopt;
    }

    const auto location{frame.root_location()};
    if (!location.has_value()) {
      return std::nullopt;
    }

    return location.value().get().base_dialect;
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    return std::nullopt;
  }
}

class CustomResolver {
public:
  CustomResolver(
      const sourcemeta::core::Options &options,
      const std::optional<sourcemeta::blaze::Configuration> &configuration,
      const bool remote, const std::string_view default_dialect)
      : options_{options}, configuration_{configuration}, remote_{remote} {
    if (options.contains("resolve")) {
      const auto entries{for_each_json(options.at("resolve"), options)};
      std::vector<std::size_t> pending;
      pending.reserve(entries.size());
      for (std::size_t index = 0; index < entries.size(); index++) {
        pending.push_back(index);
      }

      // Importing a schema requires resolving its meta-schema, which may well
      // be another one of the schemas that the user is importing. Rather than
      // forcing the user to declare their files in dependency order, keep
      // retrying the ones that cannot resolve yet for as long as every pass
      // manages to import at least one more schema
      while (!pending.empty()) {
        std::vector<std::size_t> deferred;
        std::exception_ptr failure;

        for (const auto index : pending) {
          try {
            this->import_entry(entries[index], default_dialect);
          } catch (const sourcemeta::core::FileError<
                   sourcemeta::blaze::SchemaResolutionError> &) {
            if (!failure) {
              failure = std::current_exception();
            }

            LOG_DEBUG(options)
                << "Deferring import until the remaining schemas are "
                   "imported: "
                << entries[index].first << "\n";
            deferred.push_back(index);
          }
        }

        // Nothing can make progress anymore, so report the first failure,
        // which is exactly what the user would have seen if imports were
        // never retried. Note that when several entries remain stuck, the
        // one we report might be waiting on another stuck entry rather than
        // on the schema that is genuinely missing
        if (deferred.size() == pending.size()) {
          std::rethrow_exception(failure);
        }

        pending = std::move(deferred);
      }
    }

    if (this->configuration_.has_value()) {
      for (const auto &[dependency_uri, dependency_path] :
           this->configuration_.value().dependencies) {
        if (!std::filesystem::exists(dependency_path)) {
          continue;
        }

        auto schema{sourcemeta::core::read_json(dependency_path)};
        if (!schema.is_object() && !schema.is_boolean()) {
          continue;
        }

        try {
          this->add(schema, dependency_path, default_dialect);
        } catch (...) {
          continue;
        }

        if (this->schemas.emplace(dependency_uri, schema).second) {
          this->origins.emplace(
              dependency_uri,
              std::make_pair(dependency_path, sourcemeta::core::Pointer{}));
        }
      }
    }
  }

  // Prevent accidental copies, as every schema this imported would come
  // along. Passing this resolver by value to anything that takes a
  // sourcemeta::blaze::SchemaResolver would do exactly that
  CustomResolver(const CustomResolver &) = delete;
  auto operator=(const CustomResolver &) -> CustomResolver & = delete;
  CustomResolver(CustomResolver &&) = default;
  auto operator=(CustomResolver &&) -> CustomResolver & = delete;
  ~CustomResolver() = default;

  auto add(const sourcemeta::core::JSON &schema,
           const std::filesystem::path &origin,
           const std::string_view default_dialect = "",
           const std::string_view default_id = "",
           const std::function<void(const sourcemeta::core::JSON::String &)>
               &callback = nullptr) -> bool {
    assert(schema.is_object() || schema.is_boolean());

    // Registering the top-level schema is not enough. We need to check
    // and register every embedded schema resource too
    const sourcemeta::blaze::SchemaFrame frame{
        sourcemeta::blaze::SchemaFrame::Mode::References,
        schema,
        sourcemeta::blaze::schema_walker,
        std::ref(*this),
        default_dialect,
        default_id};

    bool added_any_schema{false};
    frame.for_each_resource(
        [this, &schema, &frame, &origin, &callback, &added_any_schema](
            const std::string_view uri,
            const sourcemeta::blaze::SchemaFrame::Location &entry) -> void {
          auto subschema{sourcemeta::core::get(schema, entry.pointer)};
          // Reject a resource whose vocabularies we cannot make sense of
          // upfront, rather than at the point some consumer relies on them
          [[maybe_unused]] const auto &subschema_vocabularies{
              frame.vocabularies(entry, std::ref(*this))};

          // Given we might be resolving embedded resources, we fully
          // resolve their dialect and identifiers, otherwise the
          // consumer might have no idea what to do with them
          subschema.assign("$schema", sourcemeta::core::JSON{entry.dialect});
          sourcemeta::blaze::schema_reidentify(subschema, uri,
                                               entry.base_dialect);

          const std::string identifier{uri};
          const auto result{this->schemas.emplace(identifier, subschema)};
          if (!result.second && result.first->second != subschema) {
            const auto other{this->origins.find(identifier)};
            assert(other != this->origins.cend());
            throw SchemaIdentifierConflictError{
                identifier, sourcemeta::core::to_pointer(entry.pointer),
                other->second.first, other->second.second};
          }

          this->origins.emplace(
              identifier, std::make_pair(origin, sourcemeta::core::to_pointer(
                                                     entry.pointer)));

          if (callback) {
            callback(identifier);
          }

          added_any_schema = true;
        });

    return added_any_schema;
  }

  auto operator()(std::string_view identifier) const
      -> sourcemeta::blaze::SchemaResolverResult {
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

    const auto match{this->schemas.find(target)};
    if (match != this->schemas.cend()) {
      return match->second;
    }

    auto fetched{fetch_schema(this->options_, target, this->remote_)};
    if (!fetched.has_value()) {
      return fetched;
    }

    // Only a schema that declares no identifier of its own needs one, and
    // taking ownership just to set it would defeat handing schemas back by
    // reference
    const auto base_dialect{
        anonymous_base_dialect(fetched.value(), std::ref(*this))};
    if (!base_dialect.has_value()) {
      return fetched;
    }

    auto schema{std::move(fetched).to_owned()};
    sourcemeta::blaze::schema_reidentify(schema, string_identifier,
                                         base_dialect.value());
    return std::move(schema);
  }

private:
  auto import_entry(const InputJSON &entry,
                    const std::string_view default_dialect) -> void {
    LOG_DEBUG(this->options_)
        << "Detecting schema resources from file: " << entry.first << "\n";

    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
          entry.resolution_base,
          "The file you provided does not represent a valid JSON Schema");
    }

    try {
      const auto result =
          this->add(entry.second, entry.resolution_base, default_dialect,
                    sourcemeta::jsonschema::default_id(entry),
                    [this](const auto &identifier) {
                      LOG_DEBUG(this->options_)
                          << "Importing schema into the resolution context: "
                          << identifier << "\n";
                    });
      if (!result) {
        LOG_WARNING() << "No schema resources were imported from this file\n"
                      << "  at " << entry.first << "\n"
                      << "Are you sure this schema sets any identifiers?\n";
      }
    } catch (const SchemaIdentifierConflictError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<
            sourcemeta::core::FileError<SchemaIdentifierConflictError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error.identifier(), error.location(),
            error.other_path(), error.other());
      }

      throw sourcemeta::core::FileError<SchemaIdentifierConflictError>(
          entry.resolution_base, error.identifier(), error.location(),
          error.other_path(), error.other());
    } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
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
          sourcemeta::blaze::SchemaAnchorCollisionError>(entry.resolution_base,
                                                         error);
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
          sourcemeta::blaze::SchemaUnknownDialectError>(entry.resolution_base);
    } catch (const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError
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

  std::map<std::string, sourcemeta::core::JSON> schemas{};
  std::map<std::string,
           std::pair<std::filesystem::path, sourcemeta::core::Pointer>>
      origins{};
  const sourcemeta::core::Options &options_;
  const std::optional<sourcemeta::blaze::Configuration> configuration_;
  bool remote_{false};
};

inline auto
resolver(const sourcemeta::core::Options &options, const bool remote,
         const std::string_view default_dialect,
         const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> const sourcemeta::blaze::SchemaResolver & {
  using CacheKey = std::pair<bool, std::string>;
  static std::map<CacheKey, CustomResolver> resolver_cache;
  // What callers get is a handle that refers back to the cached resolver,
  // as the resolver itself must never be copied into the callee
  static std::map<CacheKey, sourcemeta::blaze::SchemaResolver> handle_cache;
  const CacheKey cache_key{remote, std::string{default_dialect}};

  const auto handle{handle_cache.find(cache_key)};
  if (handle != handle_cache.cend()) {
    return handle->second;
  }

  const auto iterator{
      resolver_cache
          .emplace(std::piecewise_construct, std::forward_as_tuple(cache_key),
                   std::forward_as_tuple(options, configuration, remote,
                                         default_dialect))
          .first};
  return handle_cache.emplace(cache_key, std::ref(iterator->second))
      .first->second;
}

} // namespace sourcemeta::jsonschema

#endif
