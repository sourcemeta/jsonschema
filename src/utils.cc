#include <sourcemeta/hydra/httpclient.h>
#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include "utils.h"

#include <algorithm> // std::any_of, std::none_of
#include <cassert>   // assert
#include <fstream>   // std::ofstream
#include <iostream>  // std::cerr
#include <optional>  // std::optional, std::nullopt
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

namespace {

bool path_starts_with(const std::filesystem::path &path,
                      const std::filesystem::path &prefix) {
  auto path_iterator = path.begin();
  auto prefix_iterator = prefix.begin();

  while (prefix_iterator != prefix.end()) {
    if (path_iterator == path.end() || *path_iterator != *prefix_iterator) {
      return false;
    }

    ++path_iterator;
    ++prefix_iterator;
  }

  return true;
}

auto handle_json_entry(
    const std::filesystem::path &entry_path,
    const std::set<std::filesystem::path> &blacklist,
    const std::set<std::string> &extensions,
    std::vector<std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>
        &result) -> void {
  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      const auto canonical{std::filesystem::canonical(entry.path())};
      if (!std::filesystem::is_directory(entry) &&
          std::any_of(extensions.cbegin(), extensions.cend(),
                      [&canonical](const auto &extension) {
                        return canonical.string().ends_with(extension);
                      }) &&
          std::none_of(blacklist.cbegin(), blacklist.cend(),
                       [&canonical](const auto &prefix) {
                         return prefix == canonical ||
                                path_starts_with(canonical, prefix);
                       })) {
        // TODO: Print a verbose message for what is getting parsed
        result.emplace_back(canonical,
                            sourcemeta::jsontoolkit::from_file(canonical));
      }
    }
  } else {
    const auto canonical{std::filesystem::canonical(entry_path)};
    if (!std::filesystem::exists(canonical)) {
      std::ostringstream error;
      error << "No such file or directory: " << canonical.string();
      throw std::runtime_error(error.str());
    }

    if (std::any_of(extensions.cbegin(), extensions.cend(),
                    [&canonical](const auto &extension) {
                      return canonical.string().ends_with(extension);
                    }) &&
        std::none_of(blacklist.cbegin(), blacklist.cend(),
                     [&canonical](const auto &prefix) {
                       return prefix == canonical ||
                              path_starts_with(canonical, prefix);
                     })) {
      // TODO: Print a verbose message for what is getting parsed
      result.emplace_back(canonical,
                          sourcemeta::jsontoolkit::from_file(canonical));
    }
  }
}

auto normalize_extension(const std::string &extension) -> std::string {
  if (extension.starts_with('.')) {
    return extension;
  }

  std::ostringstream result;
  result << '.' << extension;
  return result.str();
}

} // namespace

namespace intelligence::jsonschema::cli {

auto for_each_json(const std::vector<std::string> &arguments,
                   const std::set<std::filesystem::path> &blacklist,
                   const std::set<std::string> &extensions)
    -> std::vector<
        std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>> {
  std::vector<std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>
      result;

  if (arguments.empty()) {
    handle_json_entry(std::filesystem::current_path(), blacklist, extensions,
                      result);
  } else {
    for (const auto &entry : arguments) {
      handle_json_entry(entry, blacklist, extensions, result);
    }
  }

  return result;
}

auto parse_options(const std::span<const std::string> &arguments,
                   const std::set<std::string> &flags)
    -> std::map<std::string, std::vector<std::string>> {
  std::map<std::string, std::vector<std::string>> options;
  std::set<std::string> effective_flags{flags};
  effective_flags.insert("v");
  effective_flags.insert("verbose");

  options.insert({"", {}});
  std::optional<std::string> current_option;
  for (const auto &argument : arguments) {
    // Long option
    if (argument.starts_with("--") && argument.size() > 2) {
      current_option = argument.substr(2);
      assert(current_option.has_value());
      assert(!current_option.value().empty());
      options.insert({current_option.value(), {}});
      assert(options.contains(current_option.value()));
      if (effective_flags.contains(current_option.value())) {
        current_option = std::nullopt;
      }

      // Short option
    } else if (argument.starts_with("-") && argument.size() == 2) {
      current_option = argument.substr(1);
      assert(current_option.has_value());
      assert(current_option.value().size() == 1);
      options.insert({current_option.value(), {}});
      assert(options.contains(current_option.value()));
      if (effective_flags.contains(current_option.value())) {
        current_option = std::nullopt;
      }

      // Option value
    } else if (current_option.has_value()) {
      assert(options.contains(current_option.value()));
      options.at(current_option.value()).emplace_back(argument);
      current_option = std::nullopt;
      // Positional
    } else {
      assert(options.contains(""));
      options.at("").emplace_back(argument);
    }
  }

  return options;
}

auto pretty_evaluate_callback(std::ostringstream &output)
    -> sourcemeta::jsontoolkit::SchemaCompilerEvaluationCallback {
  return [&output](
             bool result,
             const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type
                 &step,
             const sourcemeta::jsontoolkit::Pointer &evaluate_path,
             const sourcemeta::jsontoolkit::Pointer &instance_location,
             const sourcemeta::jsontoolkit::JSON &,
             const sourcemeta::jsontoolkit::JSON &) -> void {
    if (result) {
      return;
    }

    output << "error: " << sourcemeta::jsontoolkit::describe(step) << "\n";
    output << "    at instance location \"";
    sourcemeta::jsontoolkit::stringify(instance_location, output);
    output << "\"\n";

    output << "    at evaluate path \"";
    sourcemeta::jsontoolkit::stringify(evaluate_path, output);
    output << "\"\n";
  };
}

static auto fallback_resolver(
    const std::map<std::string, std::vector<std::string>> &options,
    std::string_view identifier)
    -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
  auto official_result{
      sourcemeta::jsontoolkit::official_resolver(identifier).get()};
  if (official_result.has_value()) {
    std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
    promise.set_value(std::move(official_result));
    return promise.get_future();
  }

  // If the URI is not an HTTP URL, then abort
  const sourcemeta::jsontoolkit::URI uri{std::string{identifier}};
  const auto maybe_scheme{uri.scheme()};
  if (uri.is_urn() || !maybe_scheme.has_value() ||
      (maybe_scheme.value() != "https" && maybe_scheme.value() != "http")) {
    std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
    promise.set_value(std::nullopt);
    return promise.get_future();
  }

  log_verbose(options) << "Attempting to fetch over HTTP: " << identifier
                       << "\n";
  sourcemeta::hydra::http::ClientRequest request{std::string{identifier}};
  request.method(sourcemeta::hydra::http::Method::GET);
  sourcemeta::hydra::http::ClientResponse response{request.send().get()};
  if (response.status() != sourcemeta::hydra::http::Status::OK) {
    std::ostringstream error;
    error << "Failed to fetch " << identifier
          << " over HTTP. Got status code: " << response.status();
    throw std::runtime_error(error.str());
  }

  std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
  promise.set_value(sourcemeta::jsontoolkit::parse(response.body()));
  return promise.get_future();
}

auto resolver(const std::map<std::string, std::vector<std::string>> &options,
              const bool remote) -> sourcemeta::jsontoolkit::SchemaResolver {
  sourcemeta::jsontoolkit::MapSchemaResolver dynamic_resolver{
      [&remote, &options](std::string_view identifier) {
        if (remote) {
          return fallback_resolver(options, identifier);
        } else {
          return sourcemeta::jsontoolkit::official_resolver(identifier);
        }
      }};

  if (options.contains("resolve")) {
    for (const auto &entry :
         for_each_json(options.at("resolve"), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Loading schema: " << entry.first << "\n";
      dynamic_resolver.add(entry.second);
    }
  }

  if (options.contains("r")) {
    for (const auto &entry :
         for_each_json(options.at("r"), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Loading schema: " << entry.first << "\n";
      dynamic_resolver.add(entry.second);
    }
  }

  return dynamic_resolver;
}

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream & {
  if (options.contains("verbose") || options.contains("v")) {
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

auto parse_extensions(const std::map<std::string, std::vector<std::string>>
                          &options) -> std::set<std::string> {
  std::set<std::string> result;

  if (options.contains("extension")) {
    for (const auto &extension : options.at("extension")) {
      log_verbose(options) << "Using extension: " << extension << "\n";
      result.insert(normalize_extension(extension));
    }
  }

  if (options.contains("e")) {
    for (const auto &extension : options.at("e")) {
      log_verbose(options) << "Using extension: " << extension << "\n";
      result.insert(normalize_extension(extension));
    }
  }

  if (result.empty()) {
    result.insert({".json"});
  }

  return result;
}

auto parse_ignore(const std::map<std::string, std::vector<std::string>>
                      &options) -> std::set<std::filesystem::path> {
  std::set<std::filesystem::path> result;

  if (options.contains("ignore")) {
    for (const auto &ignore : options.at("ignore")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      log_verbose(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  if (options.contains("i")) {
    for (const auto &ignore : options.at("e")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      log_verbose(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  return result;
}

} // namespace intelligence::jsonschema::cli
