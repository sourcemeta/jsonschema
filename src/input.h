#ifndef SOURCEMETA_JSONSCHEMA_CLI_INPUT_H_
#define SOURCEMETA_JSONSCHEMA_CLI_INPUT_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include "configuration.h"
#include "logger.h"

#include <algorithm>     // std::any_of, std::none_of, std::sort, std::count
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uintptr_t
#include <filesystem>    // std::filesystem
#include <functional>    // std::ref, std::hash
#include <iostream>      // std::cin
#include <set>           // std::set
#include <sstream>       // std::ostringstream
#include <stdexcept>     // std::runtime_error
#include <string>        // std::string
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

namespace sourcemeta::jsonschema {

struct InputJSON {
  std::string first;
  std::filesystem::path resolution_base;
  sourcemeta::core::JSON second;
  sourcemeta::core::PointerPositionTracker positions;
  std::size_t index{0};
  bool multidocument{false};
  bool yaml{false};
  bool from_stdin{false};
  auto operator<(const InputJSON &other) const noexcept -> bool {
    return this->first < other.first;
  }
};

inline auto parse_extensions(
    const sourcemeta::core::Options &options,
    const std::optional<sourcemeta::blaze::Configuration> &configuration)
    -> const std::set<std::string> & {
  using CacheKey =
      std::pair<std::uintptr_t, std::optional<std::filesystem::path>>;
  static std::map<CacheKey, std::set<std::string>> cache;

  CacheKey cache_key{reinterpret_cast<std::uintptr_t>(&options),
                     configuration.has_value()
                         ? std::optional{configuration.value().absolute_path}
                         : std::nullopt};

  const auto iterator{cache.find(cache_key)};
  if (iterator != cache.end()) {
    return iterator->second;
  }

  std::set<std::string> result;

  if (options.contains("extension")) {
    for (const auto &extension : options.at("extension")) {
      if (extension.empty() || extension.starts_with('.')) {
        result.emplace(extension);
      } else {
        std::ostringstream normalised_extension;
        normalised_extension << '.' << extension;
        result.emplace(normalised_extension.str());
      }
    }
  }

  if (configuration.has_value()) {
    for (const auto &extension : configuration.value().extension) {
      if (extension.empty() || extension.starts_with('.')) {
        result.emplace(extension);
      } else {
        std::ostringstream normalised_extension;
        normalised_extension << '.' << extension;
        result.emplace(normalised_extension.str());
      }
    }
  }

  for (const auto &extension : result) {
    if (extension.empty()) {
      LOG_WARNING() << "Matching files with no extension\n";
    } else {
      LOG_VERBOSE(options) << "Using extension: " << extension << "\n";
    }
  }

  if (result.empty()) {
    result.insert({".json"});
    result.insert({".yaml"});
    result.insert({".yml"});
  }

  return cache.emplace(std::move(cache_key), std::move(result)).first->second;
}

inline auto parse_ignore(const sourcemeta::core::Options &options)
    -> std::set<std::filesystem::path> {
  std::set<std::filesystem::path> result;

  if (options.contains("ignore")) {
    for (const auto &ignore : options.at("ignore")) {
      const auto canonical{std::filesystem::weakly_canonical(ignore)};
      LOG_VERBOSE(options) << "Ignoring path: " << canonical << "\n";
      result.insert(canonical);
    }
  }

  return result;
}

inline auto
merge_configuration_ignore(const std::filesystem::path &configuration_path,
                           std::set<std::filesystem::path> &blacklist,
                           const sourcemeta::core::Options &options) -> void {
  try {
    const auto configuration{sourcemeta::blaze::Configuration::read_json(
        configuration_path, configuration_reader)};
    for (const auto &ignore_path : configuration.ignore) {
      LOG_VERBOSE(options) << "Ignoring path from configuration: "
                           << ignore_path << "\n";
      blacklist.insert(ignore_path);
    }
  } catch (const sourcemeta::blaze::ConfigurationParseError &error) {
    throw FileError<sourcemeta::blaze::ConfigurationParseError>(
        configuration_path, error);
  }
}

namespace {

struct ParsedJSON {
  sourcemeta::core::JSON document;
  sourcemeta::core::PointerPositionTracker positions;
  bool yaml{false};
};

inline auto read_file(const std::filesystem::path &path) -> ParsedJSON {
  const auto extension{path.extension()};
  sourcemeta::core::PointerPositionTracker positions;

  if (extension == ".yaml" || extension == ".yml") {
    return {sourcemeta::core::read_yaml(path, std::ref(positions)),
            std::move(positions), true};
  } else if (extension == ".json") {
    return {sourcemeta::core::read_json(path, std::ref(positions)),
            std::move(positions), false};
  }

  try {
    return {sourcemeta::core::read_json(path, std::ref(positions)),
            std::move(positions), false};
  } catch (const sourcemeta::core::JSONParseError &) {
    sourcemeta::core::PointerPositionTracker yaml_positions;
    return {sourcemeta::core::read_yaml(path, std::ref(yaml_positions)),
            std::move(yaml_positions), true};
  }
}

// stdin is a non-seekable stream, so we cannot retry parsing after failure.
// Unlike file-based input where we detect format by extension or try JSON
// then YAML, stdin only supports JSON input.
inline auto read_from_stdin() -> ParsedJSON {
  sourcemeta::core::PointerPositionTracker positions;
  return {sourcemeta::core::parse_json(std::cin, std::ref(positions)),
          std::move(positions), false};
}

inline auto
handle_json_entry(const std::filesystem::path &entry_path,
                  const std::set<std::filesystem::path> &blacklist,
                  const std::set<std::string> &extensions,
                  std::vector<sourcemeta::jsonschema::InputJSON> &result,
                  const sourcemeta::core::Options &options) -> void {
  if (entry_path == "-") {
    // We treat stdin as a single file, and its "resolution base" is
    // the current working directory, so relative references are resolved
    // against where the command is being run
    auto parsed{read_from_stdin()};
    const auto current_path{std::filesystem::current_path()};
    result.push_back({current_path.string(), current_path,
                      std::move(parsed.document), std::move(parsed.positions),
                      0, false, parsed.yaml, true});
    return;
  }

  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      auto canonical{sourcemeta::core::weakly_canonical(entry.path())};
      if (!std::filesystem::is_directory(entry) &&
          std::any_of(extensions.cbegin(), extensions.cend(),
                      [&canonical](const auto &extension) {
                        return extension.empty()
                                   ? !canonical.has_extension()
                                   : canonical.string().ends_with(extension);
                      }) &&
          std::none_of(blacklist.cbegin(), blacklist.cend(),
                       [&canonical](const auto &prefix) {
                         return sourcemeta::core::starts_with(canonical,
                                                              prefix);
                       })) {
        if (std::filesystem::is_empty(canonical)) {
          continue;
        }

        // TODO: Print a verbose message for what is getting parsed
        auto parsed{read_file(canonical)};
        result.push_back({canonical.string(), std::move(canonical),
                          std::move(parsed.document),
                          std::move(parsed.positions), 0, false, parsed.yaml});
      }
    }
  } else {
    const auto canonical{sourcemeta::core::weakly_canonical(entry_path)};
    if (std::none_of(blacklist.cbegin(), blacklist.cend(),
                     [&canonical](const auto &prefix) {
                       return sourcemeta::core::starts_with(canonical, prefix);
                     })) {
      if (canonical.extension() == ".jsonl") {
        LOG_VERBOSE(options)
            << "Interpreting input as JSONL: " << canonical.string() << "\n";
        auto stream{sourcemeta::core::read_file(canonical)};
        std::size_t index{0};
        try {
          for (const auto &document : sourcemeta::core::JSONL{stream}) {
            // TODO: Get real positions for JSONL
            sourcemeta::core::PointerPositionTracker positions;
            result.push_back({canonical.string(), canonical, document,
                              std::move(positions), index, true});
            index += 1;
          }
        } catch (const sourcemeta::core::JSONParseError &error) {
          throw sourcemeta::core::JSONFileParseError(canonical, error);
        }

        if (index == 0) {
          LOG_WARNING() << "The JSONL file is empty\n";
        }
      } else if (canonical.extension() == ".yaml" ||
                 canonical.extension() == ".yml") {
        if (std::filesystem::is_empty(canonical)) {
          return;
        }
        auto stream{sourcemeta::core::read_file(canonical)};
        std::vector<std::pair<sourcemeta::core::JSON,
                              sourcemeta::core::PointerPositionTracker>>
            documents;
        std::uint64_t line_offset{0};
        std::uint64_t max_line{0};
        while (stream.peek() != std::char_traits<char>::eof()) {
          sourcemeta::core::PointerPositionTracker positions;
          const std::uint64_t current_offset{line_offset};
          max_line = 0;
          auto callback =
              [&positions, current_offset,
               &max_line](const sourcemeta::core::JSON::ParsePhase phase,
                          const sourcemeta::core::JSON::Type type,
                          const std::uint64_t line, const std::uint64_t column,
                          const sourcemeta::core::JSON::ParseContext context,
                          const std::size_t index,
                          const sourcemeta::core::JSON::StringView property) {
                max_line = std::max(max_line, line);
                positions(phase, type, line + current_offset, column, context,
                          index, property);
              };
          documents.emplace_back(sourcemeta::core::parse_yaml(stream, callback),
                                 std::move(positions));
          // The YAML parser reports the line of the next document separator,
          // so we subtract 1 to get the actual lines consumed by this document
          line_offset += max_line > 0 ? max_line - 1 : 0;
        }

        if (documents.size() > 1) {
          LOG_VERBOSE(options) << "Interpreting input as YAML multi-document: "
                               << canonical.string() << "\n";
          std::size_t index{0};
          for (auto &entry : documents) {
            result.push_back({canonical.string(), canonical,
                              std::move(entry.first), std::move(entry.second),
                              index, true, true});
            index += 1;
          }
        } else if (documents.size() == 1) {
          result.push_back({canonical.string(), std::move(canonical),
                            std::move(documents.front().first),
                            std::move(documents.front().second), 0, false,
                            true});
        }
      } else {
        if (std::filesystem::is_regular_file(canonical) &&
            std::filesystem::is_empty(canonical)) {
          return;
        }
        // TODO: Print a verbose message for what is getting parsed
        auto parsed{read_file(canonical)};
        result.push_back({canonical.string(), std::move(canonical),
                          std::move(parsed.document),
                          std::move(parsed.positions), 0, false, parsed.yaml});
      }
    }
  }
}

} // namespace

inline auto
check_no_duplicate_stdin(const std::vector<std::string_view> &arguments)
    -> void {
  if (std::count(arguments.cbegin(), arguments.cend(), "-") > 1) {
    throw StdinError("Cannot read from standard input more than once");
  }
}

inline auto for_each_json(const std::vector<std::string_view> &arguments,
                          const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  check_no_duplicate_stdin(arguments);

  auto blacklist{parse_ignore(options)};
  std::vector<InputJSON> result;

  if (arguments.empty()) {
    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(current_path)};
    const auto &configuration{read_configuration(options, configuration_path)};

    if (configuration_path.has_value()) {
      merge_configuration_ignore(configuration_path.value(), blacklist,
                                 options);
    }

    const auto extensions{parse_extensions(options, configuration)};

    handle_json_entry(configuration.has_value()
                          ? configuration.value().absolute_path
                          : current_path,
                      blacklist, extensions, result, options);
  } else {
    std::unordered_set<std::string> seen_configurations;
    for (const auto &entry : arguments) {
      // Skip stdin when looking for configurations
      if (entry == "-") {
        continue;
      }

      const auto entry_path{
          sourcemeta::core::weakly_canonical(std::filesystem::path{entry})};
      const auto configuration_path{
          find_configuration(std::filesystem::is_directory(entry_path)
                                 ? entry_path
                                 : entry_path.parent_path())};
      if (configuration_path.has_value() &&
          seen_configurations.insert(configuration_path.value().string())
              .second) {
        merge_configuration_ignore(configuration_path.value(), blacklist,
                                   options);
      }
    }

    const auto extensions{parse_extensions(options, std::nullopt)};
    for (const auto &entry : arguments) {
      handle_json_entry(entry, blacklist, extensions, result, options);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const auto &left, const auto &right) { return left < right; });

  return result;
}

inline auto for_each_json(const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  return for_each_json(options.positional(), options);
}

} // namespace sourcemeta::jsonschema

#endif
