#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <set>      // std::set
#include <string>   // std::string

#include "command.h"
#include "utils.h"

// TODO: Add a flag to emit output using the standard JSON Schema output format
// TODO: Add a flag to collect annotations
auto intelligence::jsonschema::cli::validate(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"h", "http", "m", "metaschema"})};
  CLI_ENSURE(options.at("").size() >= 1, "You must pass a schema")
  const auto &schema_path{options.at("").at(0)};
  const auto custom_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};

  const auto schema{sourcemeta::jsontoolkit::from_file(schema_path)};

  if (options.contains("m") || options.contains("metaschema") ||
      options.at("").size() < 2) {
    const auto metaschema_template{sourcemeta::jsontoolkit::compile(
        sourcemeta::jsontoolkit::metaschema(schema, custom_resolver),
        sourcemeta::jsontoolkit::default_schema_walker, custom_resolver,
        sourcemeta::jsontoolkit::default_schema_compiler)};
    std::ostringstream error;
    if (sourcemeta::jsontoolkit::evaluate(
            metaschema_template, schema,
            sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
            pretty_evaluate_callback(error))) {
      log_verbose(options)
          << schema_path
          << ": The schema is valid with respect to its metaschema\n";
    } else {
      std::cerr << error.str();
      std::cerr << schema_path
                << ": The schema is NOT valid with respect to its metaschema\n";
      return EXIT_FAILURE;
    }
  }

  bool result{true};
  if (options.at("").size() >= 2) {
    const auto &instance_path{options.at("").at(1)};
    const auto schema_template{sourcemeta::jsontoolkit::compile(
        schema, sourcemeta::jsontoolkit::default_schema_walker, custom_resolver,
        sourcemeta::jsontoolkit::default_schema_compiler)};

    const auto instance{sourcemeta::jsontoolkit::from_file(instance_path)};

    std::ostringstream error;
    result = sourcemeta::jsontoolkit::evaluate(
        schema_template, instance,
        sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
        pretty_evaluate_callback(error));

    if (result) {
      log_verbose(options) << "Valid\n";
    } else {
      std::cerr << error.str();
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
