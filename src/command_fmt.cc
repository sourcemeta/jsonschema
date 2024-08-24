#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr
#include <sstream>  // std::ostringstream

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::fmt(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"c", "check"})};

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    if (options.contains("c") || options.contains("check")) {
      log_verbose(options) << "Checking: " << entry.first.string() << "\n";
      std::ifstream input{entry.first};
      std::ostringstream buffer;
      buffer << input.rdbuf();
      std::ostringstream expected;
      sourcemeta::jsontoolkit::prettify(
          entry.second, expected,
          sourcemeta::jsontoolkit::schema_format_compare);
      expected << "\n";

      if (buffer.str() == expected.str()) {
        log_verbose(options) << "PASS: " << entry.first.string() << "\n";
      } else {
        std::cerr << "FAIL: " << entry.first.string() << "\n";
        std::cerr << "Got: \n"
                  << buffer.str() << "\nBut expected:\n"
                  << expected.str() << "\n";
        return EXIT_FAILURE;
      }
    } else {
      log_verbose(options) << "Formatting: " << entry.first.string() << "\n";
      std::ofstream output{entry.first};
      sourcemeta::jsontoolkit::prettify(
          entry.second, output, sourcemeta::jsontoolkit::schema_format_compare);
      output << "\n";
    }
  }

  return EXIT_SUCCESS;
}
