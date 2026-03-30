#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include "command.h"
#include "error.h"
#include <filesystem> // std::filesystem
#include <iostream>   // std::cout
#include <string>     // std::string

namespace {

struct LoadedSchema {
  std::filesystem::path path;
  sourcemeta::core::JSON document;
};

auto read_schema(const std::string &source) -> LoadedSchema {
  if (source == "-") {
    throw sourcemeta::jsonschema::StdinError{
        "This command does not support reading schemas from standard input "
        "yet"};
  }

  const std::filesystem::path path{source};
  if (std::filesystem::is_directory(path)) {
    throw std::filesystem::filesystem_error{
        "The input was supposed to be a file but it is a directory", path,
        std::make_error_code(std::errc::is_a_directory)};
  }

  auto document{sourcemeta::core::read_yaml_or_json(path)};
  const auto display_path{path};

  if (!sourcemeta::core::is_schema(document)) {
    throw sourcemeta::jsonschema::NotSchemaError{display_path};
  }

  return {display_path, std::move(document)};
}

auto option_value_or_default(const sourcemeta::core::Options &options,
                             const std::string &name,
                             const std::string &fallback) -> std::string {
  const auto &option_name{name};
  if (!options.contains(option_name) || options.at(option_name).empty()) {
    return fallback;
  }

  return std::string{options.at(option_name).front()};
}

} // namespace

auto sourcemeta::jsonschema::compat(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() != 2) {
    throw PositionalArgumentError{
        "This command expects a path to a base schema and a path to a\n"
        "candidate schema",
        "jsonschema compat path/to/base.schema.json "
        "path/to/candidate.schema.json"};
  }

  auto mode{option_value_or_default(options, "mode", "backward")};
  if (mode != "backward" && mode != "forward" && mode != "full") {
    throw InvalidOptionEnumerationValueError{
        "Unknown compatibility mode", "mode", {"backward", "forward", "full"}};
  }

  auto format{option_value_or_default(options, "format", "text")};
  if (format != "text" && format != "json") {
    throw InvalidOptionEnumerationValueError{
        "Unknown compatibility report format", "format", {"text", "json"}};
  }

  auto fail_on{option_value_or_default(options, "fail-on", "breaking")};
  if (fail_on != "none" && fail_on != "warning" && fail_on != "breaking") {
    throw InvalidOptionEnumerationValueError{
        "Unknown compatibility failure threshold",
        "fail-on",
        {"none", "warning", "breaking"}};
  }

  if (options.contains("json")) {
    if (options.contains("format") && format != "json") {
      throw OptionConflictError{
          "The --json option cannot be combined with --format text"};
    }

    format = "json";
  }

  const auto base{read_schema(options.positional().at(0))};
  const auto candidate{read_schema(options.positional().at(1))};

  constexpr auto message =
      "This prototype validates inputs and reserves the CLI contract while "
      "the semantic compatibility engine is integrated.";

  if (format == "json") {
    auto output{sourcemeta::core::JSON::make_object()};
    output.assign("status", sourcemeta::core::JSON{"prototype"});
    output.assign("message", sourcemeta::core::JSON{message});
    output.assign("base", sourcemeta::core::JSON{base.path.string()});
    output.assign("candidate", sourcemeta::core::JSON{candidate.path.string()});
    output.assign("mode", sourcemeta::core::JSON{mode});
    output.assign("failOn", sourcemeta::core::JSON{fail_on});
    sourcemeta::core::prettify(output, std::cout);
    std::cout << "\n";
    return;
  }

  std::cout << "Compatibility analysis prototype\n"
            << "================================\n"
            << "Base schema: " << base.path.string() << "\n"
            << "Candidate schema: " << candidate.path.string() << "\n"
            << "Mode: " << mode << "\n"
            << "Fail on: " << fail_on << "\n\n"
            << message << "\n";
}
