#ifndef SOURCEMETA_JSONSCHEMA_CLI_DEPENDENCIES_H_
#define SOURCEMETA_JSONSCHEMA_CLI_DEPENDENCIES_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>

#include "error.h"
#include "logger.h"
#include "resolver.h"

#include <cassert>    // assert
#include <cstdint>    // std::uint8_t
#include <cstdlib>    // EXIT_FAILURE
#include <filesystem> // std::filesystem
#include <fstream>    // std::ofstream
#include <iostream>   // std::cerr, std::cout
#include <optional>   // std::optional
#include <string>     // std::string
#include <utility>    // std::move

namespace sourcemeta::jsonschema {

static inline auto padded_label(const std::string_view label) -> std::string {
  assert(label.size() <= 14);
  std::string result{label};
  result.append(14 - label.size(), ' ');
  result += " : ";
  return result;
}

// TODO: Elevate this to Core's IO module
static inline auto atomic_write(const std::filesystem::path &path,
                                const sourcemeta::core::JSON &document)
    -> void {
  auto temporary_path{path};
  temporary_path += ".tmp";
  std::ofstream stream{temporary_path};
  stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  sourcemeta::core::prettify(document, stream);
  stream << "\n";
  stream.close();
  std::filesystem::rename(temporary_path, path);
}

static inline auto
dependency_fetch(const sourcemeta::core::Options &options,
                 const std::filesystem::path &configuration_path,
                 std::string_view uri) -> sourcemeta::core::JSON {
  auto result{sourcemeta::jsonschema::fetch_schema(options, uri, true, true)};
  if (result.has_value()) {
    return std::move(result.value());
  }

  throw sourcemeta::jsonschema::FileError<
      sourcemeta::core::SchemaResolutionError>(
      configuration_path, std::string{uri}, "Could not resolve schema");
}

static inline auto
dependency_resolve(const sourcemeta::core::Options &options,
                   const sourcemeta::blaze::Configuration &configuration,
                   std::string_view identifier)
    -> std::optional<sourcemeta::core::JSON> {
  const std::string string_identifier{identifier};

  const auto mapped{sourcemeta::jsonschema::resolve_map_uri(configuration,
                                                            string_identifier)};
  if (mapped.has_value()) {
    try {
      auto result{
          sourcemeta::jsonschema::fetch_schema(options, mapped.value())};
      if (result.has_value()) {
        return result;
      }
    } catch (...) {
    }
  }

  for (const auto &[dependency_uri, dependency_path] :
       configuration.dependencies) {
    if (dependency_uri == string_identifier &&
        std::filesystem::exists(dependency_path)) {
      return sourcemeta::core::read_json(dependency_path);
    }
  }

  try {
    return sourcemeta::jsonschema::fetch_schema(options, identifier);
  } catch (...) {
    return std::nullopt;
  }
}

static inline auto
emit_debug(const sourcemeta::core::Options &options,
           const sourcemeta::blaze::Configuration::FetchEvent &event) -> void {
  if (!options.contains("debug")) {
    return;
  }

  using Type = sourcemeta::blaze::Configuration::FetchEvent::Type;
  static const char *type_names[] = {
      "fetch/start",   "fetch/end",    "bundle/start", "bundle/end",
      "write/start",   "write/end",    "verify/start", "verify/end",
      "up-to-date",    "file-missing", "orphaned",     "mismatched",
      "path-mismatch", "untracked",    "error"};
  static_assert(sizeof(type_names) / sizeof(type_names[0]) ==
                static_cast<std::uint8_t>(Type::Error) + 1);
  const auto type_index{static_cast<std::uint8_t>(event.type)};
  std::cerr << "debug: " << type_names[type_index] << ": " << event.uri << " ("
            << (event.index + 1) << "/" << event.total << ")";
  if (!event.path.empty()) {
    std::cerr << " -> " << event.path.string();
  }
  std::cerr << "\n";
}

static inline auto emit_json(sourcemeta::core::JSON &events_array,
                             const std::string_view type,
                             const std::string_view key,
                             const std::string_view value) -> void {
  auto json_event{sourcemeta::core::JSON::make_object()};
  json_event.assign("type", sourcemeta::core::JSON{std::string{type}});
  json_event.assign(std::string{key},
                    sourcemeta::core::JSON{std::string{value}});
  events_array.push_back(std::move(json_event));
}

static inline auto output_json(sourcemeta::core::JSON &events_array) -> void {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign("events", std::move(events_array));
  sourcemeta::core::prettify(result, std::cout);
  std::cout << "\n";
}

enum class OrphanedBehavior : std::uint8_t { Delete, Error };

static inline auto make_on_event(const sourcemeta::core::Options &options,
                                 bool &had_error, const bool is_json,
                                 sourcemeta::core::JSON &events_array,
                                 const OrphanedBehavior orphaned_behavior)
    -> sourcemeta::blaze::Configuration::FetchEvent::Callback {
  return [&options, &had_error, is_json, &events_array, orphaned_behavior](
             const sourcemeta::blaze::Configuration::FetchEvent &event)
             -> bool {
    using Type = sourcemeta::blaze::Configuration::FetchEvent::Type;

    emit_debug(options, event);

    switch (event.type) {
      case Type::FetchStart:
        if (is_json) {
          emit_json(events_array, "fetching", "uri", event.uri);
        } else {
          std::cerr << padded_label("Fetching") << event.uri << "\n";
        }

        break;
      case Type::FetchEnd:
        break;
      case Type::BundleStart:
        LOG_VERBOSE(options) << padded_label("Bundling") << event.uri << "\n";
        break;
      case Type::BundleEnd:
        break;
      case Type::WriteStart:
        LOG_VERBOSE(options)
            << padded_label("Writing") << event.path.string() << "\n";
        break;
      case Type::WriteEnd:
        break;
      case Type::VerifyStart:
        LOG_VERBOSE(options)
            << padded_label("Verifying") << event.path.string() << "\n";
        break;
      case Type::VerifyEnd:
        if (is_json) {
          auto json_event{sourcemeta::core::JSON::make_object()};
          json_event.assign("type", sourcemeta::core::JSON{"installed"});
          json_event.assign("uri",
                            sourcemeta::core::JSON{std::string{event.uri}});
          json_event.assign("path",
                            sourcemeta::core::JSON{event.path.string()});
          events_array.push_back(std::move(json_event));
        } else {
          std::cerr << padded_label("Installed") << event.path.string() << "\n";
        }

        break;
      case Type::UpToDate:
        if (is_json) {
          emit_json(events_array, "up-to-date", "uri", event.uri);
        } else {
          std::cerr << padded_label("Up to date") << event.uri << "\n";
        }

        break;
      case Type::FileMissing:
        if (is_json) {
          emit_json(events_array, "file-missing", "path", event.path.string());
        } else {
          std::cerr << padded_label("File missing") << event.path.string()
                    << "\n";
        }

        break;
      case Type::Mismatched:
        if (is_json) {
          emit_json(events_array, "mismatched", "path", event.path.string());
        } else {
          std::cerr << padded_label("Mismatched") << event.path.string()
                    << "\n";
        }

        break;
      case Type::PathMismatch:
        if (is_json) {
          emit_json(events_array, "path-mismatch", "uri", event.uri);
        } else {
          std::cerr << padded_label("Path mismatch") << event.uri << "\n";
        }

        break;
      case Type::Untracked:
        had_error = true;
        if (is_json) {
          emit_json(events_array, "untracked", "uri", event.uri);
        } else {
          std::cerr << padded_label("Untracked") << event.uri << "\n";
        }

        break;
      case Type::Orphaned:
        if (is_json) {
          emit_json(events_array, "orphaned", "uri", event.uri);
        } else {
          std::cerr << padded_label("Orphaned") << event.uri << "\n";
        }

        if (orphaned_behavior == OrphanedBehavior::Delete) {
          std::filesystem::remove(event.path);
        } else {
          had_error = true;
        }

        break;
      case Type::Error:
        had_error = true;
        if (is_json) {
          auto json_event{sourcemeta::core::JSON::make_object()};
          json_event.assign("type", sourcemeta::core::JSON{"error"});
          json_event.assign("uri",
                            sourcemeta::core::JSON{std::string{event.uri}});
          json_event.assign("message",
                            sourcemeta::core::JSON{std::string{event.details}});
          events_array.push_back(std::move(json_event));
        } else {
          try_catch(options, [&]() -> int {
            throw InstallError{std::string{event.details},
                               std::string{event.uri}};
          });
        }

        break;
    }

    return true;
  };
}

} // namespace sourcemeta::jsonschema

#endif
