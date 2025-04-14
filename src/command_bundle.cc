#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::bundle(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"h", "http", "w", "without-id"})};
  const auto dialect{default_dialect(options)};

  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema bundle path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  auto schema{sourcemeta::jsonschema::cli::read_file(options.at("").front())};

  sourcemeta::core::bundle(
      schema, sourcemeta::core::schema_official_walker,
      resolver(options, options.contains("h") || options.contains("http"),
               dialect),
      dialect);

  if (options.contains("w") || options.contains("without-id")) {
    log_verbose(options) << "Removing schema identifiers\n";
    sourcemeta::core::unidentify(
        schema, sourcemeta::core::schema_official_walker,
        resolver(options, options.contains("h") || options.contains("http"),
                 dialect),
        dialect);
  }

  sourcemeta::core::prettify(schema, std::cout,
                             sourcemeta::core::schema_format_compare);
  std::cout << "\n";
  return EXIT_SUCCESS;
}
