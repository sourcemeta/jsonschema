#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::fmt(const sourcemeta::core::Options &options)
    -> int {
  const auto indentation{parse_indentation(options)};
  for (const auto &entry :
       for_each_json(options.positional(), parse_ignore(options),
                     parse_extensions(options))) {
    if (entry.first.extension() == ".yaml" ||
        entry.first.extension() == ".yml") {
      std::cerr << "This command does not support YAML input files yet\n";
      return EXIT_FAILURE;
    }

    std::ifstream input{entry.first};
    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (options.contains("check")) {
      log_verbose(options) << "Checking: " << entry.first.string() << "\n";
    } else {
      log_verbose(options) << "Formatting: " << entry.first.string() << "\n";
    }

    std::ostringstream expected;
    if (options.contains("keep-ordering")) {
      sourcemeta::core::prettify(entry.second, expected, indentation);
    } else {
      sourcemeta::core::prettify(entry.second, expected,
                                 sourcemeta::core::schema_format_compare,
                                 indentation);
    }
    expected << "\n";

    if (options.contains("check")) {
      if (buffer.str() == expected.str()) {
        log_verbose(options) << "PASS: " << entry.first.string() << "\n";
      } else {
        std::cerr << "FAIL: " << entry.first.string() << "\n";
        std::cerr << "Got:\n"
                  << buffer.str() << "\nBut expected:\n"
                  << expected.str() << "\n";
        return EXIT_FAILURE;
      }
    } else {
      if (buffer.str() != expected.str()) {
        std::ofstream output{entry.first};
        output << expected.str();
      }
    }
  }

  return EXIT_SUCCESS;
}
