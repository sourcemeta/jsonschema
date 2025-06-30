#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr, std::cout

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::compile(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http", "f", "fast"})};

  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema compile path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const auto &schema_path{options.at("").at(0)};
  const auto dialect{default_dialect(options)};
  const auto custom_resolver{resolver(
      options, options.contains("h") || options.contains("http"), dialect)};

  const auto schema{sourcemeta::jsonschema::cli::read_file(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    std::cerr << "error: The schema file you provided does not represent a "
                 "valid JSON Schema\n  "
              << sourcemeta::jsonschema::cli::safe_weakly_canonical(schema_path)
                     .string()
              << "\n";
    return EXIT_FAILURE;
  }

  const auto fast_mode{options.contains("f") || options.contains("fast")};
  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker, custom_resolver,
      sourcemeta::blaze::default_schema_compiler,
      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                : sourcemeta::blaze::Mode::Exhaustive,
      dialect,
      sourcemeta::core::URI::from_path(
          sourcemeta::jsonschema::cli::safe_weakly_canonical(schema_path))
          .recompose())};

  const auto template_json{sourcemeta::blaze::to_json(schema_template)};
  sourcemeta::core::prettify(template_json, std::cout);
  std::cout << "\n";

  return EXIT_SUCCESS;
}
