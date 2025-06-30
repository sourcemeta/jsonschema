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

  const std::filesystem::path schema_path{options.at("").front()};
  const auto custom_resolver{resolver(
      options, options.contains("h") || options.contains("http"), dialect)};
  auto schema{sourcemeta::jsonschema::cli::read_file(schema_path)};

  sourcemeta::core::bundle(
      schema, sourcemeta::core::schema_official_walker, custom_resolver,
      dialect,
      sourcemeta::core::URI::from_path(
          sourcemeta::jsonschema::cli::safe_weakly_canonical(schema_path))
          .recompose());

  if (options.contains("w") || options.contains("without-id")) {
    std::cerr << "warning: You are opting in to remove schema identifiers in "
                 "the bundled schema.\n";
    std::cerr << "The only legit use case of this advanced feature we know of "
                 "it to workaround\n";
    std::cerr << "non-compliant JSON Schema implementations such as Visual "
                 "Studio Code.\n";
    std::cerr << "In other case, this is not needed and may harm other use "
                 "cases. For example,\n";
    std::cerr << "you will be unable to reference the resulting schema from "
                 "other schemas\n";
    std::cerr << "using the --resolve/-r option.\n";
    sourcemeta::core::unidentify(schema,
                                 sourcemeta::core::schema_official_walker,
                                 custom_resolver, dialect);
  }

  sourcemeta::core::prettify(schema, std::cout,
                             sourcemeta::core::schema_format_compare);
  std::cout << "\n";
  return EXIT_SUCCESS;
}
