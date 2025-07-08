#include <sourcemeta/jsonschema/http.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include "utils.h"

#include <algorithm> // std::any_of, std::none_of, std::sort
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
    std::vector<std::pair<std::filesystem::path, sourcemeta::core::JSON>>
        &result) -> void {
  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      const auto canonical{
          sourcemeta::jsonschema::cli::safe_weakly_canonical(entry.path())};
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
        if (std::filesystem::is_empty(canonical)) {
          continue;
        }

        // TODO: Print a verbose message for what is getting parsed
        result.emplace_back(canonical,
                            sourcemeta::jsonschema::cli::read_file(canonical));
      }
    }
  } else {
    const auto canonical{
        sourcemeta::jsonschema::cli::safe_weakly_canonical(entry_path)};
    if (!std::filesystem::exists(canonical)) {
      std::ostringstream error;
      error << "No such file or directory\n  " << canonical.string();
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
      if (std::filesystem::is_empty(canonical)) {
        return;
      }

      // TODO: Print a verbose message for what is getting parsed
      result.emplace_back(canonical,
                          sourcemeta::jsonschema::cli::read_file(canonical));
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

namespace sourcemeta::jsonschema::cli {

auto read_file(const std::filesystem::path &path) -> sourcemeta::core::JSON {
  if (path.extension() == ".yaml" || path.extension() == ".yml") {
    return sourcemeta::core::read_yaml(path);
  }

  return sourcemeta::core::read_json(path);
}

auto for_each_json(const std::vector<std::string> &arguments,
                   const std::set<std::filesystem::path> &blacklist,
                   const std::set<std::string> &extensions)
    -> std::vector<std::pair<std::filesystem::path, sourcemeta::core::JSON>> {
  std::vector<std::pair<std::filesystem::path, sourcemeta::core::JSON>> result;

  if (arguments.empty()) {
    handle_json_entry(std::filesystem::current_path(), blacklist, extensions,
                      result);
  } else {
    for (const auto &entry : arguments) {
      handle_json_entry(entry, blacklist, extensions, result);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &left, const auto &right) { return left < right; });

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

auto print(const sourcemeta::blaze::SimpleOutput &output, std::ostream &stream)
    -> void {
  stream << "error: Schema validation failure\n";
  output.stacktrace(stream, "  ");
}

auto print_annotations(
    const sourcemeta::blaze::SimpleOutput &output,
    const std::map<std::string, std::vector<std::string>> &options,
    std::ostream &stream) -> void {
  if (options.contains("verbose") || options.contains("v")) {
    for (const auto &annotation : output.annotations()) {
      for (const auto &value : annotation.second) {
        stream << "annotation: ";
        sourcemeta::core::stringify(value, stream);
        stream << "\n  at instance location \"";
        sourcemeta::core::stringify(annotation.first.instance_location, stream);
        stream << "\"\n  at evaluate path \"";
        sourcemeta::core::stringify(annotation.first.evaluate_path, stream);
        stream << "\"\n";
      }
    }
  }
}

// TODO: Move this as an operator<< overload for TraceOutput in Blaze itself
auto print(const sourcemeta::blaze::TraceOutput &output, std::ostream &stream)
    -> void {
  for (auto iterator = output.cbegin(); iterator != output.cend(); iterator++) {
    const auto &entry{*iterator};

    if (entry.evaluate_path.empty()) {
      continue;
    }

    switch (entry.type) {
      case sourcemeta::blaze::TraceOutput::EntryType::Push:
        stream << "-> (push) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Pass:
        stream << "<- (pass) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Fail:
        stream << "<- (fail) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Annotation:
        stream << "@- (annotation) ";
        break;
      default:
        assert(false);
        break;
    }

    stream << "\"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\" (" << entry.name << ")\n";

    if (entry.annotation.has_value()) {
      stream << "   value ";

      if (entry.annotation.value().is_object()) {
        sourcemeta::core::stringify(entry.annotation.value(), stream);
      } else {
        sourcemeta::core::prettify(entry.annotation.value(), stream,
                                   sourcemeta::core::schema_format_compare);
      }

      stream << "\n";
    }

    stream << "   at \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"\n";
    stream << "   at keyword location \"" << entry.keyword_location << "\"\n";

    if (entry.vocabulary.first) {
      stream << "   at vocabulary \""
             << entry.vocabulary.second.value_or("<unknown>") << "\"\n";
    }

    // To make it easier to read
    if (std::next(iterator) != output.cend()) {
      stream << "\n";
    }
  }
}

static auto fallback_resolver(
    const std::map<std::string, std::vector<std::string>> &options,
    std::string_view identifier) -> std::optional<sourcemeta::core::JSON> {
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

  log_verbose(options) << "Resolving over HTTP: " << identifier << "\n";
  sourcemeta::jsonschema::http::ClientRequest request{std::string{identifier}};
  request.method(sourcemeta::jsonschema::http::Method::GET);
  request.capture("content-type");
  sourcemeta::jsonschema::http::ClientResponse response{request.send()};
  if (response.status() != sourcemeta::jsonschema::http::Status::OK) {
    std::ostringstream error;
    error << response.status() << "\n  at " << identifier;
    throw std::runtime_error(error.str());
  }

  if (response.header("content-type").value_or("").starts_with("text/yaml")) {
    return sourcemeta::core::parse_yaml(response.body());
  } else {
    return sourcemeta::core::parse_json(response.body());
  }
}

auto resolver(const std::map<std::string, std::vector<std::string>> &options,
              const bool remote,
              const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::SchemaResolver {
  sourcemeta::core::SchemaMapResolver dynamic_resolver{
      [remote, &options](std::string_view identifier) {
        if (remote) {
          return fallback_resolver(options, identifier);
        } else {
          return sourcemeta::core::schema_official_resolver(identifier);
        }
      }};

  if (options.contains("resolve")) {
    for (const auto &entry :
         for_each_json(options.at("resolve"), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Detecting schema resources from file: "
                           << entry.first.string() << "\n";
      const auto result = dynamic_resolver.add(
          entry.second, default_dialect,
          sourcemeta::core::URI::from_path(entry.first).recompose(),
          [&options](const auto &identifier) {
            log_verbose(options)
                << "Importing schema into the resolution context: "
                << identifier << "\n";
          });
      if (!result) {
        std::cerr
            << "warning: No schema resources were imported from this file\n";
        std::cerr << "  at " << entry.first.string() << "\n";
        std::cerr << "Are you sure this schema sets any identifiers?\n";
      }
    }
  }

  if (options.contains("r")) {
    for (const auto &entry :
         for_each_json(options.at("r"), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Detecting schema resources from file: "
                           << entry.first.string() << "\n";
      const auto result = dynamic_resolver.add(
          entry.second, default_dialect, std::nullopt,
          [&options](const auto &identifier) {
            log_verbose(options)
                << "Importing schema into the resolution context: "
                << identifier << "\n";
          });
      if (!result) {
        std::cerr
            << "warning: No schema resources were imported from this file\n";
        std::cerr << "  at " << entry.first.string() << "\n";
        std::cerr << "Are you sure this schema sets any identifiers?\n";
      }
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

auto parse_extensions(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::set<std::string> {
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
    result.insert({".yaml"});
    result.insert({".yml"});
  }

  return result;
}

auto parse_ignore(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::set<std::filesystem::path> {
  std::set<std::filesystem::path> result;

  if (options.contains("ignore")) {
    for (const auto &ignore : options.at("ignore")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      log_verbose(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  if (options.contains("i")) {
    for (const auto &ignore : options.at("i")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      log_verbose(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  return result;
}

auto safe_weakly_canonical(const std::filesystem::path &input)
    -> std::filesystem::path {
  return std::filesystem::is_fifo(input)
             ? input
             : std::filesystem::weakly_canonical(input);
}

auto default_dialect(
    const std::map<std::string, std::vector<std::string>> &options)
    -> std::optional<std::string> {

  if (options.contains("default-dialect")) {
    return options.at("default-dialect").front();
  }

  if (options.contains("d")) {
    return options.at("d").front();
  }

  return std::nullopt;
}

} // namespace sourcemeta::jsonschema::cli
