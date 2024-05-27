#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr

#include "command.h"
#include "utils.h"

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
      schema, sourcemeta::jsontoolkit::default_schema_walker, resolver(options),
      sourcemeta::jsontoolkit::default_schema_compiler)};

  const auto instance{sourcemeta::jsontoolkit::from_file(instance_path)};

  const auto result{sourcemeta::jsontoolkit::evaluate(
      schema_template, instance,
      sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
      pretty_evaluate_callback)};

  if (result) {
    log_verbose(options) << "Valid\n";
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
