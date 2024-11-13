#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout, std::endl

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::compile(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};

  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema compile path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const auto schema{sourcemeta::jsontoolkit::from_file(options.at("").front())};

  const auto compiled_schema{sourcemeta::blaze::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker,
      resolver(options, options.contains("h") || options.contains("http")),
      sourcemeta::blaze::default_schema_compiler)};

  const sourcemeta::jsontoolkit::JSON result{
      sourcemeta::blaze::to_json(compiled_schema)};
  sourcemeta::jsontoolkit::prettify(result, std::cout,
                                    sourcemeta::blaze::template_format_compare);
  std::cout << std::endl;
  return EXIT_SUCCESS;
}
