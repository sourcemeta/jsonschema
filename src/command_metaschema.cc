#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <set>      // std::set
#include <string>   // std::string

#include "command.h"
#include "utils.h"

// TODO: Add a flag to emit output using the standard JSON Schema output format
auto intelligence::jsonschema::cli::metaschema(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http"})};
  const auto custom_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};
  bool result{true};

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    if (!sourcemeta::jsontoolkit::is_schema(entry.second)) {
      std::cerr << "Not a schema: " << entry.first.string() << "\n";
      return EXIT_FAILURE;
    }

    // TODO: Cache this somehow for performance reasons?
    const auto metaschema_template{sourcemeta::jsontoolkit::compile(
        sourcemeta::jsontoolkit::metaschema(entry.second, custom_resolver),
        sourcemeta::jsontoolkit::default_schema_walker, custom_resolver,
        sourcemeta::jsontoolkit::default_schema_compiler)};

    std::ostringstream error;
    if (sourcemeta::jsontoolkit::evaluate(
            metaschema_template, entry.second,
            sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
            pretty_evaluate_callback(error))) {
      log_verbose(options)
          << entry.first.string()
          << ": The schema is valid with respect to its metaschema\n";
    } else {
      std::cerr << error.str();
      std::cerr << entry.first.string()
                << ": The schema is NOT valid with respect to its metaschema\n";
      result = false;
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
