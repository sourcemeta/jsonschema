#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout, std::endl

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::lint(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"f", "fix"})};

  sourcemeta::jsontoolkit::SchemaTransformBundle bundle;
  bundle.add(
      sourcemeta::jsontoolkit::SchemaTransformBundle::Category::Modernize);
  bundle.add(
      sourcemeta::jsontoolkit::SchemaTransformBundle::Category::AntiPattern);

  bool result{true};

  if (options.contains("f") || options.contains("fix")) {
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      auto copy = entry.second;
      bundle.apply(copy, sourcemeta::jsontoolkit::default_schema_walker,
                   resolver(options));
      std::ofstream output{entry.first};
      sourcemeta::jsontoolkit::prettify(
          copy, output, sourcemeta::jsontoolkit::schema_format_compare);
      output << std::endl;
    }
  } else {
    for (const auto &entry :
         for_each_json(options.at(""), parse_ignore(options),
                       parse_extensions(options))) {
      log_verbose(options) << "Linting: " << entry.first.string() << "\n";
      const bool subresult = bundle.check(
          entry.second, sourcemeta::jsontoolkit::default_schema_walker,
          resolver(options),
          [&entry](const auto &pointer, const auto &name, const auto &message) {
            std::cout << entry.first.string() << "\n";
            std::cout << "    ";
            sourcemeta::jsontoolkit::stringify(pointer, std::cout);
            std::cout << " " << message << " (" << name << ")\n";
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
