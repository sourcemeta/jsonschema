#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <set>      // std::set
#include <string>   // std::string

#include "command.h"
#include "utils.h"

// TODO: Add a flag to emit output using the standard JSON Schema output format
auto intelligence::jsonschema::cli::validate(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"h", "http", "m", "metaschema"})};
  CLI_ENSURE(options.at("").size() >= 2,
             "You must pass a schema followed by an instance")
  const auto &schema_path{options.at("").at(0)};
  const auto &instance_path{options.at("").at(1)};
  const auto custom_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};

  const auto schema{sourcemeta::jsontoolkit::from_file(schema_path)};

  // TODO: If not instance is passed, just validate the schema against its
  // metaschema?
  if (options.contains("m") || options.contains("metaschema")) {
    const auto metaschema_result{
        validate_against_metaschema(schema, custom_resolver)};
    if (metaschema_result) {
      log_verbose(options)
          << "The schema is valid with respect to its metaschema\n";
      ;
    } else {
      std::cerr << "The schema is NOT valid with respect to its metaschema\n";
      return EXIT_FAILURE;
    }
  }

  const auto schema_template{sourcemeta::jsontoolkit::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker, custom_resolver,
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
