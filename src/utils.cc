#include "utils.h"

#include <cassert>  // assert
#include <iostream> // std::cerr
#include <optional> // std::optional, std::nullopt
#include <sstream>  // std::ostringstream

namespace {

auto handle_json_entry(
    const std::filesystem::path &entry_path,
    std::vector<std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>
        &result) -> void {
  if (std::filesystem::is_directory(entry_path)) {
    for (auto const &entry :
         std::filesystem::recursive_directory_iterator{entry_path}) {
      if (!std::filesystem::is_directory(entry) &&
          entry.path().extension() == ".json") {
        result.emplace_back(entry.path(),
                            sourcemeta::jsontoolkit::from_file(entry.path()));
      }
    }
  } else {
    if (!std::filesystem::exists(entry_path)) {
      std::ostringstream error;
      error << "No such file or directory: " << entry_path.string();
      throw std::runtime_error(error.str());
    }

    result.emplace_back(entry_path,
                        sourcemeta::jsontoolkit::from_file(entry_path));
  }
}

} // namespace

namespace intelligence::jsonschema::cli {

auto for_each_json(const std::vector<std::string> &arguments)
    -> std::vector<
        std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>> {
  std::vector<std::pair<std::filesystem::path, sourcemeta::jsontoolkit::JSON>>
      result;

  if (arguments.empty()) {
    handle_json_entry(std::filesystem::current_path(), result);
  } else {
    for (const auto &entry : arguments) {
      handle_json_entry(entry, result);
    }
  }

  return result;
}

auto parse_options(const std::span<const std::string> &arguments,
                   const std::set<std::string> &flags)
    -> std::map<std::string, std::vector<std::string>> {
  std::map<std::string, std::vector<std::string>> options;
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
      if (flags.contains(current_option.value())) {
        current_option = std::nullopt;
      }

      // Short option
    } else if (argument.starts_with("-") && argument.size() == 2) {
      current_option = argument.substr(1);
      assert(current_option.has_value());
      assert(current_option.value().size() == 1);
      options.insert({current_option.value(), {}});
      assert(options.contains(current_option.value()));
      if (flags.contains(current_option.value())) {
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

auto pretty_evaluate_callback(
    bool result,
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &,
    const sourcemeta::jsontoolkit::Pointer &evaluate_path,
    const sourcemeta::jsontoolkit::Pointer &instance_location,
    const sourcemeta::jsontoolkit::JSON &) -> void {
  if (result) {
    return;
  }

  // TODO: Improve this pretty terrible output
  std::cerr << "✗ \"";
  sourcemeta::jsontoolkit::stringify(instance_location, std::cerr);
  std::cerr << "\" at evaluate path (\"";
  sourcemeta::jsontoolkit::stringify(evaluate_path, std::cerr);
  std::cerr << "\")\n";
}

// TODO: Use input options to get a custom-made resolver
auto resolver(const std::map<std::string, std::vector<std::string>> &)
    -> sourcemeta::jsontoolkit::SchemaResolver {
  return sourcemeta::jsontoolkit::official_resolver;
}

} // namespace intelligence::jsonschema::cli
