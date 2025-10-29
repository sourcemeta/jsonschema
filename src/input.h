#ifndef SOURCEMETA_JSONSCHEMA_CLI_INPUT_H_
#define SOURCEMETA_JSONSCHEMA_CLI_INPUT_H_

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include "configuration.h"
#include "logger.h"

#include <algorithm>  // std::any_of, std::none_of, std::sort
#include <filesystem> // std::filesystem
#include <functional> // std::ref
#include <set>        // std::set
#include <sstream>    // std::ostringstream
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
#include <vector>     // std::vector

namespace sourcemeta::jsonschema {

struct InputJSON {
  std::filesystem::path first;
  sourcemeta::core::JSON second;
  sourcemeta::core::PointerPositionTracker positions;
  auto operator<(const InputJSON &other) const noexcept -> bool {
    return this->first < other.first;
  }
};

inline auto parse_extensions(const sourcemeta::core::Options &options)
    -> std::set<std::string> {
  std::set<std::string> result;

  if (options.contains("extension")) {
    for (const auto &extension : options.at("extension")) {
      LOG_VERBOSE(options) << "Using extension: " << extension << "\n";
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

namespace {

inline auto
handle_json_entry(const std::filesystem::path &entry_path,
                  const std::set<std::filesystem::path> &blacklist,
                  const std::set<std::string> &extensions,
                  std::vector<sourcemeta::jsonschema::InputJSON> &result)
    -> void {
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

inline auto for_each_json(const std::vector<std::string_view> &arguments,
                          const sourcemeta::core::Options &options)
    -> std::vector<InputJSON> {
  const auto blacklist{parse_ignore(options)};
  const auto extensions{parse_extensions(options)};
  std::vector<InputJSON> result;

  if (arguments.empty()) {
    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(current_path)};
    const auto &configuration{read_configuration(options, configuration_path)};
    handle_json_entry(configuration.has_value()
                          ? configuration.value().absolute_path
                          : current_path,
                      blacklist, extensions, result);
  } else {
    for (const auto &entry : arguments) {
      handle_json_entry(entry, blacklist, extensions, result);
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
