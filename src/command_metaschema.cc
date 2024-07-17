#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <map>      // std::map
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

  std::map<std::string, sourcemeta::jsontoolkit::SchemaCompilerTemplate> cache;

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    if (!sourcemeta::jsontoolkit::is_schema(entry.second)) {
      std::cerr << "Not a schema: " << entry.first.string() << "\n";
      return EXIT_FAILURE;
    }

    const auto dialect{sourcemeta::jsontoolkit::dialect(entry.second)};
    assert(dialect.has_value());

    if (!cache.contains(dialect.value())) {
      const auto metaschema{
          sourcemeta::jsontoolkit::metaschema(entry.second, custom_resolver)};
      const auto metaschema_template{sourcemeta::jsontoolkit::compile(
          metaschema, sourcemeta::jsontoolkit::default_schema_walker,
          custom_resolver, sourcemeta::jsontoolkit::default_schema_compiler)};
      cache.insert({dialect.value(), metaschema_template});
    }

    std::ostringstream error;
    if (sourcemeta::jsontoolkit::evaluate(
            cache.at(dialect.value()), entry.second,
            sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
            pretty_evaluate_callback(error,
                                     sourcemeta::jsontoolkit::empty_pointer))) {
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
