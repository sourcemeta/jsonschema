#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>

#include <iostream> // std::cerr, std::cout

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::compile(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema compile path/to/schema.json"};
  }

  const auto &schema_path{options.positional().at(0)};
  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{read_configuration(options, configuration_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  const auto schema{sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_path};
  }

  const auto fast_mode{options.contains("fast")};
  const auto schema_template{sourcemeta::blaze::compile(
      schema, sourcemeta::core::schema_official_walker, custom_resolver,
      sourcemeta::blaze::default_schema_compiler,
      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                : sourcemeta::blaze::Mode::Exhaustive,
      dialect,
      sourcemeta::core::URI::from_path(
          sourcemeta::core::weakly_canonical(schema_path))
          .recompose())};

  const auto template_json{sourcemeta::blaze::to_json(schema_template)};
  if (options.contains("minify")) {
    sourcemeta::core::stringify(template_json, std::cout);
  } else {
    sourcemeta::core::prettify(template_json, std::cout);
  }

  std::cout << "\n";
}
