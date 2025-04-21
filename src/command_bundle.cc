#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::bundle(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"h", "http", "w", "without-id"})};
  if (options.at("").size() < 1) {
    log_error() << "This command expects a path to a schema. For example:\n\n"
                << "  jsonschema bundle path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const auto default_dialect{infer_default_dialect(options)};
  const auto resolver{infer_resolver(options, default_dialect)};
  auto schema{read_yaml_or_json(options.at("").front())};
  sourcemeta::core::bundle(schema, sourcemeta::core::schema_official_walker,
                           resolver, default_dialect);

  if (options.contains("w") || options.contains("without-id")) {
    log_warning()
        << "You are opting in to remove schema identifiers in "
           "the bundled schema.\nThe only legit use case of this "
           "advanced feature we know of it to workaround\nnon-compliant "
           "JSON Schema implementations such as Visual Studio Code.\nIn "
           "other case, this is not needed and may harm other use cases. "
           "For example,\nyou will be unable to reference the resulting "
           "schema from other schemas\nusing the --resolve/-r option.\n";
    sourcemeta::core::unidentify(schema,
                                 sourcemeta::core::schema_official_walker,
                                 resolver, default_dialect);
  }

  sourcemeta::core::prettify(schema, std::cout,
                             sourcemeta::core::schema_format_compare);
  std::cout << "\n";
  return EXIT_SUCCESS;
}
