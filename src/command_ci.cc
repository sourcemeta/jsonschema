#include <sourcemeta/core/json.h>

#include <cstdlib>    // EXIT_FAILURE
#include <filesystem> // std::filesystem
#include <iostream>   // std::cerr
#include <optional>   // std::optional

#include "command.h"
#include "configuration.h"
#include "dependencies.h"
#include "error.h"

auto sourcemeta::jsonschema::ci(const sourcemeta::core::Options &options)
    -> void {
  const auto is_json{options.contains("json")};
  auto events_array{sourcemeta::core::JSON::make_array()};

  auto configuration_path{
      sourcemeta::blaze::Configuration::find(std::filesystem::current_path())};

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
  if (!std::filesystem::exists(lock_path)) {
    throw LockNotFoundError{lock_path};
  }

  sourcemeta::blaze::Configuration::Lock lock;
  try {
    lock = sourcemeta::blaze::Configuration::Lock::from_json(
        sourcemeta::core::read_json(lock_path));
  } catch (const sourcemeta::core::JSONParseError &error) {
    throw sourcemeta::core::JSONFileParseError(lock_path, error);
  } catch (...) {
    throw LockParseError{lock_path};
  }

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
                                    OrphanedBehavior::Error)};

  configuration.fetch(lock, fetcher, resolver, configuration_reader, writer,
                      on_event);

  if (is_json) {
    output_json(events_array);
  }

  if (had_error) {
    throw Fail{EXIT_FAILURE};
  }
}
