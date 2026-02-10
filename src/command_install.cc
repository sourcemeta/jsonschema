#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

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
  auto official_result{sourcemeta::core::schema_resolver(uri)};
  if (official_result.has_value()) {
    return std::move(official_result.value());
  }

  const sourcemeta::core::URI parsed_uri{std::string{uri}};

  if (parsed_uri.is_file()) {
    return sourcemeta::core::read_json(parsed_uri.to_path());
  }

  const auto scheme{parsed_uri.scheme()};
  if (scheme.has_value() &&
      (scheme.value() == "https" || scheme.value() == "http")) {
    // TODO: Use sourcemeta::core::URI to set query parameters once
    // the URI module supports setters for query strings
    std::string fetch_url{uri};
    if (fetch_url.find('?') != std::string::npos) {
      fetch_url += "&bundle=1";
    } else {
      fetch_url += "?bundle=1";
    }

    return sourcemeta::jsonschema::http_fetch(fetch_url, options);
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

  const auto match{sourcemeta::jsonschema::find_resolve_match(
      configuration.resolve, string_identifier)};
  if (match != configuration.resolve.cend()) {
    const sourcemeta::core::URI new_uri{match->second};
    if (new_uri.is_relative()) {
      const auto resolved_path{configuration.absolute_path / new_uri.to_path()};
      if (std::filesystem::exists(resolved_path)) {
        return sourcemeta::core::read_json(resolved_path);
      }
    } else if (new_uri.is_file()) {
      const auto path{new_uri.to_path()};
      if (std::filesystem::exists(path)) {
        return sourcemeta::core::read_json(path);
      }
    }
  }

  for (const auto &[dependency_uri, dependency_path] :
       configuration.dependencies) {
    if (dependency_uri == string_identifier &&
        std::filesystem::exists(dependency_path)) {
      return sourcemeta::core::read_json(dependency_path);
    }
  }

  auto official_result{sourcemeta::core::schema_resolver(identifier)};
  if (official_result.has_value()) {
    return official_result;
  }

  sourcemeta::core::URI uri;
  try {
    uri = sourcemeta::core::URI{std::string{identifier}};
  } catch (const sourcemeta::core::URIParseError &) {
    return std::nullopt;
  }

  if (uri.is_file()) {
    const auto path{uri.to_path()};
    if (std::filesystem::exists(path)) {
      return sourcemeta::core::read_json(path);
    }
  }

  const auto scheme{uri.scheme()};
  if (!uri.is_urn() && scheme.has_value() &&
      (scheme.value() == "https" || scheme.value() == "http")) {
    try {
      return sourcemeta::jsonschema::http_fetch(std::string{identifier},
                                                options);
    } catch (...) {
      return std::nullopt;
    }
  }

  return std::nullopt;
}

auto emit_debug(const sourcemeta::core::Options &options,
                const sourcemeta::blaze::Configuration::FetchEvent &event)
    -> void {
  if (!options.contains("debug")) {
    return;
  }

  static const char *type_names[] = {
      "fetch/start",   "fetch/end",    "bundle/start", "bundle/end",
      "write/start",   "write/end",    "verify/start", "verify/end",
      "up-to-date",    "file-missing", "orphaned",     "mismatched",
      "path-mismatch", "untracked",    "error"};
  const auto type_index{static_cast<std::uint8_t>(event.type)};
  std::cerr << "debug: " << type_names[type_index] << ": " << event.uri << " ("
            << (event.index + 1) << "/" << event.total << ")";
  if (!event.path.empty()) {
    std::cerr << " -> " << event.path.string();
  }
  std::cerr << "\n";
}

} // namespace

auto sourcemeta::jsonschema::install(const sourcemeta::core::Options &options)
    -> void {
  const auto configuration_path{
      sourcemeta::blaze::Configuration::find(std::filesystem::current_path())};
  if (!configuration_path.has_value()) {
    throw ConfigurationNotFoundError{};
  }

  const auto configuration{sourcemeta::blaze::Configuration::read_json(
      configuration_path.value(), configuration_reader)};

  if (configuration.dependencies.empty()) {
    std::cerr << "No dependencies to install\n";
    return;
  }

  const auto lock_path{configuration.absolute_path / "jsonschema.lock.json"};
  sourcemeta::blaze::Configuration::Lock lock;
  if (std::filesystem::exists(lock_path)) {
    lock = sourcemeta::blaze::Configuration::Lock::from_json(
        sourcemeta::core::read_json(lock_path));
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
      [&options, &had_error](
          const sourcemeta::blaze::Configuration::FetchEvent &event) -> bool {
        using Type = sourcemeta::blaze::Configuration::FetchEvent::Type;

        emit_debug(options, event);

        switch (event.type) {
          case Type::FetchStart:
            std::cerr << padded_label("Fetching") << event.uri << "\n";
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
            std::cerr << padded_label("Installed") << event.path.string()
                      << "\n";
            break;
          case Type::UpToDate:
            std::cerr << padded_label("Up to date") << event.uri << "\n";
            break;
          case Type::FileMissing:
            std::cerr << padded_label("File missing") << event.path.string()
                      << "\n";
            break;
          case Type::Mismatched:
            std::cerr << padded_label("Mismatched") << event.path.string()
                      << "\n";
            break;
          case Type::PathMismatch:
            std::cerr << padded_label("Path mismatch") << event.uri << "\n";
            break;
          case Type::Untracked:
            std::cerr << padded_label("Untracked") << event.uri << "\n";
            break;
          case Type::Orphaned:
            std::cerr << padded_label("Orphaned") << event.uri << "\n";
            break;
          case Type::Error:
            had_error = true;
            std::cerr << "error: " << event.details << "\n"
                      << "  " << event.uri << "\n";
            break;
        }

        return true;
      }};

  configuration.fetch(lock, fetcher, resolver, configuration_reader, writer,
                      on_event, fetch_mode);

  if (had_error) {
    throw Fail{EXIT_FAILURE};
  }

  atomic_write(lock_path, lock.to_json());
}
