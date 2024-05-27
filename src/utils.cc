#include "utils.h"

#include <cassert>   // assert
#include <fstream>   // std::ofstream
#include <iostream>  // std::cerr
#include <optional>  // std::optional, std::nullopt
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

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

auto resolver(const std::map<std::string, std::vector<std::string>> &options)
    -> sourcemeta::jsontoolkit::SchemaResolver {
  if (!options.contains("resolve") && !options.contains("r")) {
    return sourcemeta::jsontoolkit::official_resolver;
  }

  std::map<std::string, sourcemeta::jsontoolkit::JSON> schemas;
  const std::string option{options.contains("resolve") ? "resolve" : "r"};
  for (const auto &schema_path : options.at(option)) {
    const auto schema{sourcemeta::jsontoolkit::from_file(schema_path)};
    // TODO: Use the current resolver as its building up
    const auto id{sourcemeta::jsontoolkit::id(
                      schema, sourcemeta::jsontoolkit::official_resolver)
                      .get()};
    if (!id.has_value()) {
      std::ostringstream error;
      error << "Cannot determine the identifier of the schema: " << schema_path;
      throw std::runtime_error(error.str());
    }

    // TODO: Throw if we are overriding with a duplicate id
    // TODO: We need to frame to add subschemas too?
    schemas.insert({id.value(), schema});
    log_verbose(options) << "Loading schema: " << schema_path << "\n";
  }

  return [schemas](std::string_view identifier)
             -> std::future<std::optional<sourcemeta::jsontoolkit::JSON>> {
    const std::string string_identifier{identifier};
    if (schemas.contains(string_identifier)) {
      std::promise<std::optional<sourcemeta::jsontoolkit::JSON>> promise;
      promise.set_value(schemas.at(string_identifier));
      return promise.get_future();
    }

    return sourcemeta::jsontoolkit::official_resolver(identifier);
  };
}

auto log_verbose(const std::map<std::string, std::vector<std::string>> &options)
    -> std::ostream & {
  if (options.contains("verbose") || options.contains("v")) {
    return std::cerr;
  }

  static std::ofstream null_stream;
  return null_stream;
}

} // namespace intelligence::jsonschema::cli
