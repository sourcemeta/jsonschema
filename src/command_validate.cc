#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cerr, std::cout, std::endl

#include "command.h"
#include "utils.h"

static auto
callback(bool result,
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

auto intelligence::jsonschema::cli::validate(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};

  if (options.at("").size() < 2) {
    std::cerr << "You must pass a schema followed by an instance\n";
    return EXIT_FAILURE;
  }

  const auto &schema_path{options.at("").at(0)};
  const auto &instance_path{options.at("").at(1)};

  const auto schema{sourcemeta::jsontoolkit::from_file(schema_path)};

  const auto schema_template{sourcemeta::jsontoolkit::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker,
      sourcemeta::jsontoolkit::official_resolver,
      sourcemeta::jsontoolkit::default_schema_compiler)};

  const auto instance{sourcemeta::jsontoolkit::from_file(instance_path)};

  const auto result{sourcemeta::jsontoolkit::evaluate(
      schema_template, instance,
      sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast, callback)};

  if (result) {
    std::cerr << "Valid\n";
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
