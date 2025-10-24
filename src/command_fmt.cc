#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <fstream>  // std::ofstream
#include <iostream> // std::cerr
#include <sstream>  // std::ostringstream

#include "command.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::fmt(const sourcemeta::core::Options &options)
    -> void {
  const auto indentation{parse_indentation(options)};
  for (const auto &entry : for_each_json(options)) {
    if (entry.first.extension() == ".yaml" ||
        entry.first.extension() == ".yml") {
      throw YAMLInputError{"This command does not support YAML input files yet",
                           entry.first};
    }

    if (options.contains("check")) {
      LOG_VERBOSE(options) << "Checking: " << entry.first.string() << "\n";
    } else {
      LOG_VERBOSE(options) << "Formatting: " << entry.first.string() << "\n";
    }

    const auto configuration_path{find_configuration(entry.first)};
    const auto &configuration{read_configuration(options, configuration_path)};
    const auto dialect{default_dialect(options, configuration)};
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};

    std::ostringstream expected;
    if (options.contains("keep-ordering")) {
      sourcemeta::core::prettify(entry.second, expected, indentation);
    } else {
      auto copy = entry.second;
      sourcemeta::core::format(copy, sourcemeta::core::schema_official_walker,
                               custom_resolver, dialect);
      sourcemeta::core::prettify(copy, expected, indentation);
    }
    expected << "\n";

    std::ifstream current_stream{entry.first};
    std::ostringstream current;
    current << current_stream.rdbuf();

    if (options.contains("check")) {
      if (current.str() == expected.str()) {
        LOG_VERBOSE(options) << "PASS: " << entry.first.string() << "\n";
      } else {
        std::cerr << "FAIL: " << entry.first.string() << "\n";
        std::cerr << "Got:\n"
                  << current.str() << "\nBut expected:\n"
                  << expected.str() << "\n";
        throw Fail{EXIT_FAILURE};
      }
    } else {
      if (current.str() != expected.str()) {
        std::ofstream output{entry.first};
        output << expected.str();
      }
    }
  }
}
