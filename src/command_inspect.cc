#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::inspect(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http"})};
  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema inspect path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const sourcemeta::core::JSON schema{
      read_yaml_or_json(options.at("").front())};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::Instances};

  const auto dialect{infer_default_dialect(options)};
  frame.analyse(schema, sourcemeta::core::schema_official_walker,
                infer_resolver(options, dialect), dialect);

  if (options.contains("json") || options.contains("j")) {
    sourcemeta::core::prettify(frame.to_json(), std::cout);
    std::cout << "\n";
  } else {
    std::cout << frame;
  }

  return EXIT_SUCCESS;
}
