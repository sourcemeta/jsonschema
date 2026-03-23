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
#include <deque>         // std::deque
#include <filesystem>    // std::filesystem
#include <functional>    // std::ref, std::hash
#include <iostream>      // std::cin
#include <memory>        // std::shared_ptr, std::make_shared
#include <optional>      // std::optional
#include <set>           // std::set
#include <sstream>       // std::ostringstream, std::istringstream
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
  std::shared_ptr<std::deque<std::string>> property_storage;
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
  std::shared_ptr<std::deque<std::string>> property_storage;
  bool yaml{false};
};

inline auto
make_position_callback(sourcemeta::core::PointerPositionTracker &tracker,
                       std::shared_ptr<std::deque<std::string>> &storage)
    -> sourcemeta::core::JSON::ParseCallback {
  return
      [&tracker, &storage](const sourcemeta::core::JSON::ParsePhase phase,
                           const sourcemeta::core::JSON::Type type,
                           const std::uint64_t line, const std::uint64_t column,
                           const sourcemeta::core::JSON::ParseContext context,
                           const std::size_t index,
                           const sourcemeta::core::JSON::String &property) {
        storage->emplace_back(property);
        tracker(phase, type, line, column, context, index, storage->back());
      };
}

inline auto read_file(const std::filesystem::path &path) -> ParsedJSON {
  const auto extension{path.extension()};
  sourcemeta::core::PointerPositionTracker positions;
  auto property_storage = std::make_shared<std::deque<std::string>>();
  sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};

  if (extension == ".yaml" || extension == ".yml") {
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_yaml(path, document, callback);
    return {std::move(document), std::move(positions),
            std::move(property_storage), true};
  } else if (extension == ".json") {
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_json(path, document, callback);
    return {std::move(document), std::move(positions),
            std::move(property_storage), false};
  }

  try {
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_json(path, document, callback);
    return {std::move(document), std::move(positions),
            std::move(property_storage), false};
  } catch (const sourcemeta::core::JSONParseError &) {
    sourcemeta::core::PointerPositionTracker yaml_positions;
    auto yaml_property_storage = std::make_shared<std::deque<std::string>>();
    auto callback =
        make_position_callback(yaml_positions, yaml_property_storage);
    sourcemeta::core::read_yaml(path, document, callback);
    return {std::move(document), std::move(yaml_positions),
            std::move(yaml_property_storage), true};
  }
}

// Read stdin into a buffer and try JSON first, then YAML
inline auto read_from_stdin(std::string *raw_input = nullptr) -> ParsedJSON {
  std::ostringstream buffer;
  buffer << std::cin.rdbuf();
  const auto input{buffer.str()};
  if (raw_input != nullptr) {
    *raw_input = input;
  }

  try {
    std::istringstream json_stream{input};
    sourcemeta::core::PointerPositionTracker positions;
    auto property_storage = std::make_shared<std::deque<std::string>>();
    sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::parse_json(json_stream, document, callback);
    return {std::move(document), std::move(positions),
            std::move(property_storage), false};
  } catch (const sourcemeta::core::JSONParseError &json_error) {
    try {
      std::istringstream yaml_stream{input};
      sourcemeta::core::PointerPositionTracker positions;
      auto property_storage = std::make_shared<std::deque<std::string>>();
      sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
      auto callback = make_position_callback(positions, property_storage);
      sourcemeta::core::parse_yaml(yaml_stream, document, callback);
      return {std::move(document), std::move(positions),
              std::move(property_storage), true};
    } catch (...) {
      throw sourcemeta::core::JSONFileParseError(stdin_path(), json_error);
    }
  }
}

inline auto
handle_json_entry(const std::filesystem::path &entry_path,
                  const std::set<std::filesystem::path> &blacklist,
                  const std::set<std::string> &extensions,
                  std::vector<sourcemeta::jsonschema::InputJSON> &result,
                  const sourcemeta::core::Options &options) -> void {
  if (entry_path == "-") {
    auto parsed{read_from_stdin()};
    const auto path{stdin_path()};
    result.push_back({path.string(), path, std::move(parsed.document),
                      std::move(parsed.positions), 0, false, parsed.yaml, true,
                      std::move(parsed.property_storage)});
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
                          std::move(parsed.positions), 0, false, parsed.yaml,
                          false, std::move(parsed.property_storage)});
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
            result.push_back({canonical.string(),
                              canonical,
                              document,
                              std::move(positions),
                              index,
                              true,
                              false,
                              false,
                              {}});
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
        struct MultiDocEntry {
          sourcemeta::core::JSON document;
          sourcemeta::core::PointerPositionTracker positions;
          std::shared_ptr<std::deque<std::string>> property_storage;
        };
        std::vector<MultiDocEntry> documents;
        std::uint64_t line_offset{0};
        std::uint64_t max_line{0};
        while (stream.peek() != std::char_traits<char>::eof()) {
          sourcemeta::core::PointerPositionTracker positions;
          auto property_storage = std::make_shared<std::deque<std::string>>();
          const std::uint64_t current_offset{line_offset};
          max_line = 0;
          auto callback =
              [&positions, &property_storage, current_offset,
               &max_line](const sourcemeta::core::JSON::ParsePhase phase,
                          const sourcemeta::core::JSON::Type type,
                          const std::uint64_t line, const std::uint64_t column,
                          const sourcemeta::core::JSON::ParseContext context,
                          const std::size_t index,
                          const sourcemeta::core::JSON::String &property) {
                max_line = std::max(max_line, line);
                property_storage->emplace_back(property);
                positions(phase, type, line + current_offset, column, context,
                          index, property_storage->back());
              };
          sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
          sourcemeta::core::parse_yaml(stream, document, callback);
          documents.push_back({std::move(document), std::move(positions),
                               std::move(property_storage)});
          line_offset += max_line > 0 ? max_line - 1 : 0;
        }

        if (documents.size() > 1) {
          LOG_VERBOSE(options) << "Interpreting input as YAML multi-document: "
                               << canonical.string() << "\n";
          std::size_t index{0};
          for (auto &entry : documents) {
            result.push_back({canonical.string(), canonical,
                              std::move(entry.document),
                              std::move(entry.positions), index, true, true,
                              false, std::move(entry.property_storage)});
            index += 1;
          }
        } else if (documents.size() == 1) {
          result.push_back({canonical.string(), std::move(canonical),
                            std::move(documents.front().document),
                            std::move(documents.front().positions), 0, false,
                            true, false,
                            std::move(documents.front().property_storage)});
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
                          std::move(parsed.positions), 0, false, parsed.yaml,
                          false, std::move(parsed.property_storage)});
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
    std::sort(result.begin(), result.end(),
              [](const auto &left, const auto &right) { return left < right; });
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
      const auto before{result.size()};
      handle_json_entry(entry, blacklist, extensions, result, options);
      std::sort(
          result.begin() + static_cast<std::ptrdiff_t>(before), result.end(),
          [](const auto &left, const auto &right) { return left < right; });
    }
  }

  return result;
}

inline auto for_each_json(const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  return for_each_json(options.positional(), options);
}

} // namespace sourcemeta::jsonschema

#endif
