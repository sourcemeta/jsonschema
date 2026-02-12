#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>    // EXIT_FAILURE
#include <filesystem> // std::filesystem
#include <iostream>   // std::cerr
#include <optional>   // std::optional
#include <string>     // std::string
#include <utility>    // std::move

#include "command.h"
#include "configuration.h"
#include "dependencies.h"
#include "error.h"

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
    const bool has_existing_config{configuration_path.has_value()};
    if (has_existing_config) {
      try {
        add_configuration = sourcemeta::blaze::Configuration::read_json(
            configuration_path.value(), configuration_reader);
      } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
        throw FileError<sourcemeta::blaze::ConfigurationParseError>(
            configuration_path.value(), error.what(), error.location());
      } catch (const sourcemeta::core::JSONParseError &error) {
        throw sourcemeta::core::JSONFileParseError(configuration_path.value(),
                                                   error);
      }
    } else {
      configuration_path = std::filesystem::current_path() / "jsonschema.json";
      add_configuration.absolute_path = std::filesystem::current_path();
    }

    const auto absolute_target{std::filesystem::weakly_canonical(
        std::filesystem::current_path() / input_path)};
    try {
      const sourcemeta::core::URI uri{dependency_uri};
      add_configuration.dependencies.erase(
          sourcemeta::core::URI::canonicalize(uri.recompose()));
      add_configuration.add_dependency(uri, absolute_target);
    } catch (const sourcemeta::core::URIParseError &) {
      throw PositionalArgumentError{
          "The given URI is not valid",
          "jsonschema install https://example.com/schema ./vendor/schema.json"};
    } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
      throw FileError<sourcemeta::blaze::ConfigurationParseError>(
          configuration_path.value(), error.what(), error.location());
    }
    auto config_json{
        has_existing_config
            ? sourcemeta::core::read_json(configuration_path.value())
            : sourcemeta::core::JSON::make_object()};
    config_json.assign("dependencies",
                       add_configuration.to_json().at("dependencies"));
    atomic_write(configuration_path.value(), config_json);

    auto relative_target{std::filesystem::relative(
                             absolute_target, add_configuration.absolute_path)
                             .generic_string()};
    if (!relative_target.starts_with("..")) {
      relative_target = "./" + relative_target;
    }

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
        return dependency_fetch(options, configuration_path.value(), uri);
      }};

  const sourcemeta::core::SchemaResolver resolver{
      [&options, &configuration](std::string_view identifier)
          -> std::optional<sourcemeta::core::JSON> {
        return dependency_resolve(options, configuration, identifier);
      }};

  bool had_error{false};
  const auto on_event{make_on_event(options, had_error, is_json, events_array,
                                    OrphanedBehavior::Delete)};

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
