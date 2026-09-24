#ifndef SOURCEMETA_JSONSCHEMA_CLI_INPUT_H_
#define SOURCEMETA_JSONSCHEMA_CLI_INPUT_H_

#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/gzip.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/yaml.h>

#include "configuration.h"
#include "logger.h"

#include <algorithm>     // std::any_of, std::none_of, std::sort, std::count
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint8_t, std::uintptr_t
#include <deque>         // std::deque
#include <filesystem>    // std::filesystem
#include <functional>    // std::ref, std::hash
#include <iostream>      // std::cin
#include <memory>        // std::shared_ptr, std::make_shared
#include <optional>      // std::optional
#include <set>           // std::set
#include <sstream>       // std::ostringstream, std::istringstream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <vector>        // std::vector

namespace sourcemeta::jsonschema {

enum class InputRequirement : std::uint8_t { Optional, NonEmpty };

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
      LOG_VERBOSE(options) << "Ignoring path: \"" << canonical.generic_string()
                           << "\"\n";
      result.insert(canonical);
    }
  }

  return result;
}

inline auto
merge_configuration_ignore(const std::filesystem::path &configuration_path,
                           std::set<std::filesystem::path> &blacklist,
                           const sourcemeta::core::Options &options) -> void {
  const auto &configuration{load_configuration(options, configuration_path)};
  assert(configuration.has_value());
  for (const auto &ignore_path : configuration.value().ignore) {
    LOG_VERBOSE(options) << "Ignoring path from configuration: \""
                         << ignore_path.generic_string() << "\"\n";
    blacklist.insert(ignore_path);
  }
}

namespace {

struct ParsedJSON {
  sourcemeta::core::JSON document;
  sourcemeta::core::PointerPositionTracker positions;
  std::shared_ptr<std::deque<std::string>> property_storage;
  bool yaml{false};
};

struct MultiDocEntry {
  sourcemeta::core::JSON document;
  sourcemeta::core::PointerPositionTracker positions;
  std::shared_ptr<std::deque<std::string>> property_storage;
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

// RFC 8259 Section 2: "ws = *( %x20 / %x09 / %x0A / %x0D )", which is narrower
// than the ASCII whitespace that sourcemeta::core::trim strips by default
inline auto is_json_whitespace(const char character) noexcept -> bool {
  return character == ' ' || character == '\t' || character == '\n' ||
         character == '\r';
}

// Whether the buffer the stream reads from carries nothing but JSON whitespace
// past the point the parser stopped at
inline auto at_end_of_stream(const std::string &input, std::istream &stream)
    -> bool {
  const auto consumed{stream.tellg()};
  if (consumed < 0) {
    return false;
  }

  const auto offset{static_cast<std::size_t>(consumed)};
  return offset >= input.size() ||
         sourcemeta::core::strip_left(std::string_view{input}.substr(offset),
                                      is_json_whitespace)
             .empty();
}

// Parse every YAML document the stream holds, keeping line numbers running
// across the documents that follow the first
inline auto read_yaml_documents(std::istream &stream)
    -> std::vector<MultiDocEntry> {
  std::vector<MultiDocEntry> documents;
  std::uint64_t line_offset{0};
  std::uint64_t max_line{0};

  while (stream.peek() != std::char_traits<char>::eof()) {
    sourcemeta::core::PointerPositionTracker positions;
    auto property_storage = std::make_shared<std::deque<std::string>>();
    const std::uint64_t current_offset{line_offset};
    max_line = 0;
    auto callback = [&positions, &property_storage, current_offset, &max_line](
                        const sourcemeta::core::JSON::ParsePhase phase,
                        const sourcemeta::core::JSON::Type type,
                        const std::uint64_t line, const std::uint64_t column,
                        const sourcemeta::core::JSON::ParseContext context,
                        const std::size_t index,
                        const sourcemeta::core::JSON::String &property) {
      max_line = std::max(max_line, line);
      property_storage->emplace_back(property);
      positions(phase, type, line + current_offset, column, context, index,
                property_storage->back());
    };
    sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
    sourcemeta::core::parse_yaml(stream, document, callback);
    documents.push_back({.document = std::move(document),
                         .positions = std::move(positions),
                         .property_storage = std::move(property_storage)});
    line_offset += max_line > 0 ? max_line - 1 : 0;
  }

  return documents;
}

inline auto read_file(const std::filesystem::path &path) -> ParsedJSON {
  const auto extension{path.extension()};
  sourcemeta::core::PointerPositionTracker positions;
  auto property_storage = std::make_shared<std::deque<std::string>>();
  sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};

  if (extension == ".yaml" || extension == ".yml") {
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_yaml(path, document, callback);
    return {.document = std::move(document),
            .positions = std::move(positions),
            .property_storage = std::move(property_storage),
            .yaml = true};
  }

  if (extension == ".json") {
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_json(path, document, callback);
    return {.document = std::move(document),
            .positions = std::move(positions),
            .property_storage = std::move(property_storage)};
  }

  try {
    auto callback = make_position_callback(positions, property_storage);
    sourcemeta::core::read_json(path, document, callback);
    return {.document = std::move(document),
            .positions = std::move(positions),
            .property_storage = std::move(property_storage)};
  } catch (const sourcemeta::core::JSONParseError &) {
    sourcemeta::core::PointerPositionTracker yaml_positions;
    auto yaml_property_storage = std::make_shared<std::deque<std::string>>();
    auto callback =
        make_position_callback(yaml_positions, yaml_property_storage);
    sourcemeta::core::read_yaml(path, document, callback);
    return {.document = std::move(document),
            .positions = std::move(yaml_positions),
            .property_storage = std::move(yaml_property_storage),
            .yaml = true};
  }
}

// Read standard input as the one or more YAML documents it holds, reporting
// the JSON error that sent us here if it cannot be read that way either
inline auto read_stdin_yaml(const std::string &input,
                            const sourcemeta::core::JSONParseError &json_error)
    -> std::vector<ParsedJSON> {
  std::istringstream stream{input};
  std::vector<MultiDocEntry> documents;

  try {
    documents = read_yaml_documents(stream);
  } catch (...) {
    throw sourcemeta::core::JSONFileParseError(stdin_path(), json_error);
  }

  if (documents.empty()) {
    throw sourcemeta::core::JSONFileParseError(stdin_path(), json_error);
  }

  std::vector<ParsedJSON> result;
  result.reserve(documents.size());
  for (auto &entry : documents) {
    result.push_back({.document = std::move(entry.document),
                      .positions = std::move(entry.positions),
                      .property_storage = std::move(entry.property_storage),
                      .yaml = true});
  }

  return result;
}

// Read every document the given standard input buffer holds, trying JSON
// first, then JSONL for input that carries more than one JSON document, and
// finally YAML
inline auto read_stdin_documents(const std::string &input)
    -> std::vector<ParsedJSON> {
  std::istringstream json_stream{input};
  sourcemeta::core::PointerPositionTracker positions;
  auto property_storage = std::make_shared<std::deque<std::string>>();
  sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
  auto callback = make_position_callback(positions, property_storage);

  try {
    sourcemeta::core::parse_json(json_stream, document, callback);
  } catch (const sourcemeta::core::JSONParseError &json_error) {
    return read_stdin_yaml(input, json_error);
  }

  if (at_end_of_stream(input, json_stream)) {
    std::vector<ParsedJSON> result;
    result.push_back({.document = std::move(document),
                      .positions = std::move(positions),
                      .property_storage = std::move(property_storage)});
    return result;
  }

  // Standard input that carries more than one JSON document is JSONL, unless
  // what follows the first document only reads as YAML, as a YAML stream may
  // open with a document that is JSON and separate the rest with markers
  std::vector<ParsedJSON> result;
  std::istringstream jsonl_stream{input};
  try {
    for (const auto &entry : sourcemeta::core::JSONL{jsonl_stream}) {
      // TODO: Get real positions for JSONL
      sourcemeta::core::PointerPositionTracker jsonl_positions;
      result.push_back({.document = entry,
                        .positions = std::move(jsonl_positions),
                        .property_storage = {}});
    }
  } catch (const sourcemeta::core::JSONParseError &error) {
    return read_stdin_yaml(input, error);
  }

  return result;
}

// Read the single document standard input is expected to hold
inline auto read_from_stdin(std::string *raw_input = nullptr) -> ParsedJSON {
  const auto input{sourcemeta::core::read_stdin()};
  if (raw_input != nullptr) {
    *raw_input = input;
  }

  auto documents{read_stdin_documents(input)};
  assert(!documents.empty());
  if (documents.size() > 1) {
    throw MultiDocumentInputError{"This command does not support reading "
                                  "multiple documents from standard input",
                                  stdin_path()};
  }

  return std::move(documents.front());
}

inline auto
handle_input_file(const std::filesystem::path &canonical,
                  std::vector<sourcemeta::jsonschema::InputJSON> &result,
                  const sourcemeta::core::Options &options) -> void {
  const auto canonical_string{canonical.generic_string()};
  if (canonical_string.ends_with(".jsonl.gz")) {
    LOG_VERBOSE(options) << "Interpreting input as GZIP-compressed JSONL: "
                         << canonical_string << "\n";
    std::ifstream stream{sourcemeta::core::canonical(canonical),
                         std::ios::binary};
    stream.exceptions(std::ifstream::badbit);
    std::size_t index{0};
    try {
      for (const auto &document : sourcemeta::core::JSONL{
               stream, sourcemeta::core::JSONL::Mode::GZIP}) {
        // TODO: Get real positions for JSONL
        sourcemeta::core::PointerPositionTracker positions;
        result.push_back({.first = canonical_string,
                          .resolution_base = canonical,
                          .second = document,
                          .positions = std::move(positions),
                          .index = index,
                          .multidocument = true,
                          .property_storage = {}});
        index += 1;
      }
    } catch (const sourcemeta::core::GZIPError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::GZIPError>(
          canonical, error.what());
    } catch (const sourcemeta::core::JSONParseError &error) {
      throw sourcemeta::core::JSONFileParseError(canonical, error);
    }

    if (index == 0) {
      LOG_WARNING() << "The JSONL file is empty\n";
    }
  } else if (canonical.extension() == ".jsonl") {
    LOG_VERBOSE(options) << "Interpreting input as JSONL: " << canonical_string
                         << "\n";
    auto stream{sourcemeta::core::read_file(canonical)};
    std::size_t index{0};
    try {
      for (const auto &document : sourcemeta::core::JSONL{stream}) {
        // TODO: Get real positions for JSONL
        sourcemeta::core::PointerPositionTracker positions;
        result.push_back({.first = canonical_string,
                          .resolution_base = canonical,
                          .second = document,
                          .positions = std::move(positions),
                          .index = index,
                          .multidocument = true,
                          .property_storage = {}});
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
    std::vector<MultiDocEntry> documents;
    try {
      documents = read_yaml_documents(stream);
    } catch (const sourcemeta::core::YAMLParseError &error) {
      throw sourcemeta::core::YAMLFileParseError{canonical, error};
    }

    if (documents.size() > 1) {
      LOG_VERBOSE(options) << "Interpreting input as YAML multi-document: "
                           << canonical_string << "\n";
      std::size_t index{0};
      for (auto &entry : documents) {
        result.push_back(
            {.first = canonical_string,
             .resolution_base = canonical,
             .second = std::move(entry.document),
             .positions = std::move(entry.positions),
             .index = index,
             .multidocument = true,
             .yaml = true,
             .property_storage = std::move(entry.property_storage)});
        index += 1;
      }
    } else if (documents.size() == 1) {
      result.push_back(
          {.first = canonical_string,
           .resolution_base = canonical,
           .second = std::move(documents.front().document),
           .positions = std::move(documents.front().positions),
           .yaml = true,
           .property_storage = std::move(documents.front().property_storage)});
    }
  } else {
    if (std::filesystem::is_regular_file(canonical) &&
        std::filesystem::is_empty(canonical)) {
      return;
    }
    // TODO: Print a verbose message for what is getting parsed
    auto parsed{read_file(canonical)};
    result.push_back({.first = canonical_string,
                      .resolution_base = canonical,
                      .second = std::move(parsed.document),
                      .positions = std::move(parsed.positions),
                      .yaml = parsed.yaml,
                      .property_storage = std::move(parsed.property_storage)});
  }
}

inline auto
handle_json_entry(const std::filesystem::path &entry_path,
                  const std::set<std::filesystem::path> &blacklist,
                  const std::set<std::string> &extensions,
                  std::vector<sourcemeta::jsonschema::InputJSON> &result,
                  const sourcemeta::core::Options &options) -> void {
  if (entry_path == "-") {
    auto documents{read_stdin_documents(sourcemeta::core::read_stdin())};
    assert(!documents.empty());
    const auto path{stdin_path()};
    const auto multidocument{documents.size() > 1};

    if (multidocument) {
      LOG_VERBOSE(options)
          << (documents.front().yaml
                  ? "Interpreting standard input as YAML multi-document\n"
                  : "Interpreting standard input as JSONL\n");
    }

    std::size_t index{0};
    for (auto &document : documents) {
      result.push_back(
          {.first = std::string{STDIN_DEFAULT_ID},
           .resolution_base = path,
           .second = std::move(document.document),
           .positions = std::move(document.positions),
           .index = index,
           .multidocument = multidocument,
           .yaml = document.yaml,
           .from_stdin = true,
           .property_storage = std::move(document.property_storage)});
      index += 1;
    }

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
                         return sourcemeta::core::is_under_path(canonical,
                                                                prefix);
                       })) {
        if (std::filesystem::is_empty(canonical)) {
          continue;
        }

        handle_input_file(canonical, result, options);
      }
    }
  } else {
    const auto canonical{sourcemeta::core::weakly_canonical(entry_path)};
    if (std::none_of(blacklist.cbegin(), blacklist.cend(),
                     [&canonical](const auto &prefix) {
                       return sourcemeta::core::is_under_path(canonical,
                                                              prefix);
                     })) {
      handle_input_file(canonical, result, options);
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
                          const sourcemeta::core::Options &options,
                          const InputRequirement requirement)
    -> std::vector<InputJSON> {
  check_no_duplicate_stdin(arguments);

  auto blacklist{parse_ignore(options)};
  std::vector<InputJSON> result;

  if (arguments.empty()) {
    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(options, current_path)};
    const auto &configuration{read_configuration(options, configuration_path)};

    const auto &scan_path = configuration.has_value()
                                ? configuration.value().absolute_path
                                : current_path;

    if (!configuration_path.has_value()) {
      LOG_WARNING()
          << "Recursively processing every file in "
          << sourcemeta::core::weakly_canonical(current_path).generic_string()
          << " as no input was provided\n";
    } else if (configuration.has_value() &&
               !configuration.value().absolute_path_explicit) {
      LOG_WARNING()
          << "Recursively processing every file in "
          << sourcemeta::core::weakly_canonical(scan_path).generic_string()
          << " as the configuration file does not set an explicit path\n";
    }

    if (configuration_path.has_value()) {
      merge_configuration_ignore(configuration_path.value(), blacklist,
                                 options);
    }

    const auto extensions{parse_extensions(options, configuration)};

    handle_json_entry(scan_path, blacklist, extensions, result, options);
    if (result.empty() && requirement == InputRequirement::NonEmpty) {
      throw sourcemeta::core::FileError<NoInputFilesError>(scan_path);
    }

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
          find_configuration(options, std::filesystem::is_directory(entry_path)
                                          ? entry_path
                                          : entry_path.parent_path())};
      if (configuration_path.has_value() &&
          seen_configurations.insert(configuration_path.value().string())
              .second) {
        merge_configuration_ignore(configuration_path.value(), blacklist,
                                   options);
      }
    }

    for (const auto &entry : arguments) {
      std::optional<std::filesystem::path> entry_configuration_path{
          std::nullopt};
      if (entry != "-") {
        const auto entry_path{
            sourcemeta::core::weakly_canonical(std::filesystem::path{entry})};
        entry_configuration_path = find_configuration(
            options, std::filesystem::is_directory(entry_path)
                         ? entry_path
                         : entry_path.parent_path());
      }
      const auto &entry_configuration{
          load_configuration(options, entry_configuration_path)};
      const auto &extensions{parse_extensions(options, entry_configuration)};
      const auto before{result.size()};
      handle_json_entry(entry, blacklist, extensions, result, options);
      std::sort(
          result.begin() + static_cast<std::ptrdiff_t>(before), result.end(),
          [](const auto &left, const auto &right) { return left < right; });
    }

    if (result.empty() && requirement == InputRequirement::NonEmpty) {
      throw sourcemeta::core::FileError<NoInputFilesError>(
          std::filesystem::path{arguments.front()});
    }
  }

  return result;
}

inline auto for_each_json(const std::vector<std::string_view> &arguments,
                          const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  return for_each_json(arguments, options, InputRequirement::Optional);
}

inline auto for_each_json(const sourcemeta::core::Options &options,
                          const InputRequirement requirement)
    -> std::vector<InputJSON> {
  return for_each_json(options.positional(), options, requirement);
}

inline auto for_each_json(const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  return for_each_json(options.positional(), options,
                       InputRequirement::Optional);
}

} // namespace sourcemeta::jsonschema

#endif
