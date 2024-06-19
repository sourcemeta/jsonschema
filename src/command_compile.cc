#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout, std::endl

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::compile(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};
  CLI_ENSURE(!options.at("").empty(), "You must pass a JSON Schema as input");
  const auto schema{sourcemeta::jsontoolkit::from_file(options.at("").front())};

  const auto compiled_schema{sourcemeta::jsontoolkit::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker,
      resolver(options, options.contains("h") || options.contains("http")),
      sourcemeta::jsontoolkit::default_schema_compiler)};

  const sourcemeta::jsontoolkit::JSON result{
      sourcemeta::jsontoolkit::to_json(compiled_schema)};
  sourcemeta::jsontoolkit::prettify(result, std::cout);
  std::cout << std::endl;
  return EXIT_SUCCESS;
}
