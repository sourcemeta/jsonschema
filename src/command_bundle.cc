#include <sourcemeta/core/editorschema.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::bundle(
    const sourcemeta::core::Options &options) -> int {

  if (options.positional().size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema bundle path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const std::filesystem::path schema_path{options.positional().front()};
  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{read_configuration(options, configuration_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};
  auto schema{sourcemeta::core::read_yaml_or_json(schema_path)};

  sourcemeta::core::bundle(schema, sourcemeta::core::schema_official_walker,
                           custom_resolver, dialect,
                           sourcemeta::core::URI::from_path(
                               sourcemeta::core::weakly_canonical(schema_path))
                               .recompose());

  if (options.contains("without-id")) {
    std::cerr << "warning: You are opting in to remove schema identifiers in "
                 "the bundled schema.\n";
    std::cerr << "The only legit use case of this advanced feature we know of "
                 "is to workaround\n";
    std::cerr << "non-compliant JSON Schema implementations such as Visual "
                 "Studio Code.\n";
    std::cerr << "Otherwise, this is not needed and may harm other use "
                 "cases. For example,\n";
    std::cerr << "you will be unable to reference the resulting schema from "
                 "other schemas\n";
    std::cerr << "using the --resolve/-r option.\n";
    sourcemeta::core::for_editor(schema,
                                 sourcemeta::core::schema_official_walker,
                                 custom_resolver, dialect);
  }

  sourcemeta::core::prettify(schema, std::cout,
                             sourcemeta::core::schema_format_compare);
  std::cout << "\n";
  return EXIT_SUCCESS;
}
