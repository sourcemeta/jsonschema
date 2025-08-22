#include <sourcemeta/jsonschema/http.h>

#include <sourcemeta/core/io.h>
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

auto handle_json_entry(
    const std::filesystem::path &entry_path,
    const std::set<std::filesystem::path> &blacklist,
    const std::set<std::string> &extensions,
    std::vector<sourcemeta::jsonschema::cli::InputJSON> &result) -> void {
  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      auto canonical{sourcemeta::core::weakly_canonical(entry.path())};
      if (!std::filesystem::is_directory(entry) &&
          std::any_of(extensions.cbegin(), extensions.cend(),
                      [&canonical](const auto &extension) {
                        return canonical.string().ends_with(extension);
                      }) &&
          std::none_of(blacklist.cbegin(), blacklist.cend(),
                       [&canonical](const auto &prefix) {
                         return sourcemeta::core::starts_with(canonical,
                                                              prefix);
                       })) {
        if (std::filesystem::is_empty(canonical)) {
          continue;
        }

        sourcemeta::core::PointerPositionTracker positions;
        // TODO: Print a verbose message for what is getting parsed
        auto contents{sourcemeta::core::read_yaml_or_json(canonical,
                                                          std::ref(positions))};
        result.push_back(
            {std::move(canonical), std::move(contents), std::move(positions)});
      }
    }
  } else {
    const auto canonical{sourcemeta::core::weakly_canonical(entry_path)};
    if (!std::filesystem::exists(canonical)) {
      std::ostringstream error;
      error << "No such file or directory\n  " << canonical.string();
      throw std::runtime_error(error.str());
    }

    if (std::none_of(blacklist.cbegin(), blacklist.cend(),
                     [&canonical](const auto &prefix) {
                       return sourcemeta::core::starts_with(canonical, prefix);
                     })) {
      if (std::filesystem::is_empty(canonical)) {
        return;
      }

      sourcemeta::core::PointerPositionTracker positions;
      // TODO: Print a verbose message for what is getting parsed
      auto contents{
          sourcemeta::core::read_yaml_or_json(canonical, std::ref(positions))};
      result.push_back(
          {std::move(canonical), std::move(contents), std::move(positions)});
    }
  }
}

} // namespace

namespace sourcemeta::jsonschema::cli {

auto for_each_json(const std::vector<std::string_view> &arguments,
                   const std::set<std::filesystem::path> &blacklist,
                   const std::set<std::string> &extensions)
    -> std::vector<InputJSON> {
  std::vector<InputJSON> result;

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

auto print(const sourcemeta::blaze::SimpleOutput &output, std::ostream &stream)
    -> void {
  stream << "error: Schema validation failure\n";
  output.stacktrace(stream, "  ");
}

auto print_annotations(const sourcemeta::blaze::SimpleOutput &output,
                       const sourcemeta::core::Options &options,
                       std::ostream &stream) -> void {
  if (options.contains("verbose")) {
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

static auto fallback_resolver(const sourcemeta::core::Options &options,
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

auto resolver(const sourcemeta::core::Options &options, const bool remote,
              const std::optional<std::string> &default_dialect)
    -> sourcemeta::core::SchemaResolver {
  sourcemeta::core::SchemaMapResolver dynamic_resolver{
      [remote, &options](std::string_view identifier) {
        const sourcemeta::core::URI uri{std::string{identifier}};
        if (uri.is_file()) {
          const auto path{uri.to_path()};
          log_verbose(options)
              << "Attempting to read file reference from disk: "
              << path.string() << "\n";
          if (std::filesystem::exists(path)) {
            return std::optional<sourcemeta::core::JSON>{
                sourcemeta::core::read_yaml_or_json(path)};
          }
        }

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

  return dynamic_resolver;
}

auto log_verbose(const sourcemeta::core::Options &options) -> std::ostream & {
  if (options.contains("verbose")) {
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

auto parse_extensions(const sourcemeta::core::Options &options)
    -> std::set<std::string> {
  std::set<std::string> result;

  if (options.contains("extension")) {
    for (const auto &extension : options.at("extension")) {
      log_verbose(options) << "Using extension: " << extension << "\n";
      if (extension.starts_with('.')) {
        result.emplace(extension);
      } else {
        std::ostringstream normalised_extension;
        normalised_extension << '.' << extension;
        result.emplace(normalised_extension.str());
      }
    }
  }

  if (result.empty()) {
    result.insert({".json"});
    result.insert({".yaml"});
    result.insert({".yml"});
  }

  return result;
}

auto parse_ignore(const sourcemeta::core::Options &options)
    -> std::set<std::filesystem::path> {
  std::set<std::filesystem::path> result;

  if (options.contains("ignore")) {
    for (const auto &ignore : options.at("ignore")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      log_verbose(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  return result;
}

auto default_dialect(const sourcemeta::core::Options &options)
    -> std::optional<std::string> {
  if (options.contains("default-dialect")) {
    return std::string{options.at("default-dialect").front()};
  }

  return std::nullopt;
}

} // namespace sourcemeta::jsonschema::cli
