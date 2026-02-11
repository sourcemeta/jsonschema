#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cassert>     // assert
#include <cstdint>     // std::uint8_t
#include <cstdlib>     // EXIT_FAILURE
#include <filesystem>  // std::filesystem
#include <fstream>     // std::ofstream
#include <iostream>    // std::cerr
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"

namespace {

auto padded_label(const std::string_view label) -> std::string {
  assert(label.size() <= 14);
  std::string result{label};
  result.append(14 - label.size(), ' ');
  result += " : ";
  return result;
}

// TODO: Elevate this to Core's IO module
auto atomic_write(const std::filesystem::path &path,
                  const sourcemeta::core::JSON &document) -> void {
  auto temporary_path{path};
  temporary_path += ".tmp";
  std::ofstream stream{temporary_path};
  stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  sourcemeta::core::prettify(document, stream);
  stream << "\n";
  stream.close();
  std::filesystem::rename(temporary_path, path);
}

auto install_fetch(const sourcemeta::core::Options &options,
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

auto install_resolve(const sourcemeta::core::Options &options,
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

auto emit_debug(const sourcemeta::core::Options &options,
                const sourcemeta::blaze::Configuration::FetchEvent &event)
    -> void {
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

auto emit_json(sourcemeta::core::JSON &events_array,
               const std::string_view type, const std::string_view key,
               const std::string_view value) -> void {
  auto json_event{sourcemeta::core::JSON::make_object()};
  json_event.assign("type", sourcemeta::core::JSON{std::string{type}});
  json_event.assign(std::string{key},
                    sourcemeta::core::JSON{std::string{value}});
  events_array.push_back(std::move(json_event));
}

auto output_json(sourcemeta::core::JSON &events_array) -> void {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign("events", std::move(events_array));
  sourcemeta::core::prettify(result, std::cout);
  std::cout << "\n";
}

} // namespace

auto sourcemeta::jsonschema::install(const sourcemeta::core::Options &options)
    -> void {
  const auto is_json{options.contains("json")};
  auto events_array{sourcemeta::core::JSON::make_array()};

  const auto &positional_arguments{options.positional()};
  if (positional_arguments.size() != 0 && positional_arguments.size() != 2) {
    throw PositionalArgumentError{
        "The install command takes either zero or two positional arguments",
        "jsonschema install https://example.com/schema ./vendor/schema.json"};
  }

  auto configuration_path{
      sourcemeta::blaze::Configuration::find(std::filesystem::current_path())};

  if (positional_arguments.size() == 2) {
    const std::string dependency_uri{positional_arguments.at(0)};
    const std::filesystem::path input_path{positional_arguments.at(1)};

    sourcemeta::blaze::Configuration add_configuration;
    if (configuration_path.has_value()) {
      try {
        add_configuration = sourcemeta::blaze::Configuration::read_json(
            configuration_path.value(), configuration_reader);
      } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
        throw FileError<sourcemeta::blaze::ConfigurationParseError>(
            configuration_path.value(), error.what(), error.location());
      } catch (const sourcemeta::core::JSONParseError &error) {
        throw sourcemeta::core::JSONFileParseError(
            configuration_path.value(), error);
      }
    } else {
      configuration_path = std::filesystem::current_path() / "jsonschema.json";
      add_configuration.absolute_path = std::filesystem::current_path();
      add_configuration.base =
          sourcemeta::core::URI::from_path(add_configuration.absolute_path)
              .recompose();
    }

    const sourcemeta::core::URI uri{dependency_uri};
    const auto absolute_target{std::filesystem::weakly_canonical(
        std::filesystem::current_path() / input_path)};
    add_configuration.dependencies.erase(
        sourcemeta::core::URI::canonicalize(uri.recompose()));
    add_configuration.add_dependency(uri, absolute_target);
    atomic_write(configuration_path.value(), add_configuration.to_json());

    const auto relative_target{
        "./" + std::filesystem::relative(absolute_target,
                                         add_configuration.absolute_path)
                   .generic_string()};

    if (is_json) {
      auto json_event{sourcemeta::core::JSON::make_object()};
      json_event.assign("type", sourcemeta::core::JSON{"adding"});
      json_event.assign("uri", sourcemeta::core::JSON{dependency_uri});
      json_event.assign("path", sourcemeta::core::JSON{relative_target});
      events_array.push_back(std::move(json_event));
    } else {
      std::cerr << padded_label("Adding") << dependency_uri << " -> "
                << relative_target << "\n";
    }
  }

  if (!configuration_path.has_value()) {
    throw ConfigurationNotFoundError{std::filesystem::current_path()};
  }

  sourcemeta::blaze::Configuration configuration;
  try {
    configuration = sourcemeta::blaze::Configuration::read_json(
        configuration_path.value(), configuration_reader);
  } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
    throw FileError<sourcemeta::blaze::ConfigurationParseError>(
        configuration_path.value(), error.what(), error.location());
  } catch (const sourcemeta::core::JSONParseError &error) {
    throw sourcemeta::core::JSONFileParseError(configuration_path.value(),
                                               error);
  }

  if (configuration.dependencies.empty()) {
    if (is_json) {
      output_json(events_array);
    } else {
      std::cerr << "No dependencies found\n  at "
                << configuration_path.value().string() << "\n";
    }

    return;
  }

  const auto lock_path{configuration.absolute_path / "jsonschema.lock.json"};
  sourcemeta::blaze::Configuration::Lock lock;
  if (std::filesystem::exists(lock_path)) {
    try {
      lock = sourcemeta::blaze::Configuration::Lock::from_json(
          sourcemeta::core::read_json(lock_path));
    } catch (...) {
      if (is_json) {
        emit_json(events_array, "warning", "message",
                  "Ignoring corrupted lock file");
      } else {
        std::cerr << "warning: Ignoring corrupted lock file\n  at "
                  << lock_path.string() << "\n";
      }
    }
  }

  const auto fetch_mode{
      options.contains("force")
          ? sourcemeta::blaze::Configuration::FetchMode::All
          : sourcemeta::blaze::Configuration::FetchMode::Missing};

  const sourcemeta::blaze::Configuration::WriteCallback writer{
      [](const std::filesystem::path &path,
         const sourcemeta::core::JSON &document) -> void {
        std::filesystem::create_directories(path.parent_path());
        atomic_write(path, document);
      }};

  const sourcemeta::blaze::Configuration::FetchCallback fetcher{
      [&options,
       &configuration_path](std::string_view uri) -> sourcemeta::core::JSON {
        return install_fetch(options, configuration_path.value(), uri);
      }};

  const sourcemeta::core::SchemaResolver resolver{
      [&options, &configuration](std::string_view identifier)
          -> std::optional<sourcemeta::core::JSON> {
        return install_resolve(options, configuration, identifier);
      }};

  bool had_error{false};
  const sourcemeta::blaze::Configuration::FetchEvent::Callback on_event{
      [&options, &had_error, &is_json, &events_array](
          const sourcemeta::blaze::Configuration::FetchEvent &event) -> bool {
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
            LOG_VERBOSE(options)
                << padded_label("Bundling") << event.uri << "\n";
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
              std::cerr << padded_label("Installed") << event.path.string()
                        << "\n";
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
              emit_json(events_array, "file-missing", "path",
                        event.path.string());
            } else {
              std::cerr << padded_label("File missing") << event.path.string()
                        << "\n";
            }

            break;
          case Type::Mismatched:
            if (is_json) {
              emit_json(events_array, "mismatched", "path",
                        event.path.string());
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

            std::filesystem::remove(event.path);
            break;
          case Type::Error:
            had_error = true;
            if (is_json) {
              auto json_event{sourcemeta::core::JSON::make_object()};
              json_event.assign("type", sourcemeta::core::JSON{"error"});
              json_event.assign("uri",
                                sourcemeta::core::JSON{std::string{event.uri}});
              json_event.assign("message", sourcemeta::core::JSON{
                                               std::string{event.details}});
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
      }};

  configuration.fetch(lock, fetcher, resolver, configuration_reader, writer,
                      on_event, fetch_mode);

  if (is_json) {
    output_json(events_array);
  }

  if (had_error) {
    throw Fail{EXIT_FAILURE};
  }

  atomic_write(lock_path, lock.to_json());
}
