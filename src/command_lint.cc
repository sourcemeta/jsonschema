#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::lint(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"f", "fix"})};

  sourcemeta::core::SchemaTransformer bundle;
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::AntiPattern);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::Simplify);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::Superfluous);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::Redundant);
  sourcemeta::core::add(bundle,
                        sourcemeta::core::AlterSchemaCategory::SyntaxSugar);

  bool result{true};

  if (options.contains("f") || options.contains("fix")) {
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      if (entry.first.extension() == ".yaml" ||
          entry.first.extension() == ".yml") {
        std::cerr << "The --fix option is not supported for YAML input files\n";
        return EXIT_FAILURE;
      }

      auto copy = entry.second;
      bundle.apply(copy, sourcemeta::core::schema_official_walker,
                   resolver(options));
      std::ofstream output{entry.first};
      sourcemeta::core::prettify(copy, output,
                                 sourcemeta::core::schema_format_compare);
      output << "\n";
    }
  } else {
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      const bool subresult = bundle.check(
          entry.second, sourcemeta::core::schema_official_walker,
          resolver(options),
          [&entry](const auto &pointer, const auto &name, const auto &message) {
            std::cout << entry.first.string() << ":\n";
            std::cout << "  " << message << " (" << name << ")\n";
            std::cout << "    at schema location \"";
            sourcemeta::core::stringify(pointer, std::cout);
            std::cout << "\"\n";
          });

      if (subresult) {
        log_verbose(options) << "PASS: " << entry.first.string() << "\n";
      } else {
        result = false;
      }
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
