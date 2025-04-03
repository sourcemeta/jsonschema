#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::fmt(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"c", "check", "k", "keep-ordering"})};

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    if (entry.first.extension() == ".yaml" ||
        entry.first.extension() == ".yml") {
      std::cerr << "This command does not support YAML input files yet\n";
      return EXIT_FAILURE;
    }

    if (options.contains("c") || options.contains("check")) {
      log_verbose(options) << "Checking: " << entry.first.string() << "\n";
      std::ifstream input{entry.first};
      std::ostringstream buffer;
      buffer << input.rdbuf();
      std::ostringstream expected;

      if (options.contains("k") || options.contains("keep-ordering")) {
        sourcemeta::core::prettify(entry.second, expected);
      } else {
        sourcemeta::core::prettify(entry.second, expected,
                                   sourcemeta::core::schema_format_compare);
      }

      expected << "\n";

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
      log_verbose(options) << "Formatting: " << entry.first.string() << "\n";
      std::ofstream output{entry.first};

      if (options.contains("k") || options.contains("keep-ordering")) {
        sourcemeta::core::prettify(entry.second, output);
      } else {
        sourcemeta::core::prettify(entry.second, output,
                                   sourcemeta::core::schema_format_compare);
      }

      output << "\n";
    }
  }

  return EXIT_SUCCESS;
}
