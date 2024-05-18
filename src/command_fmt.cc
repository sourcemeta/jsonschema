#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::endl

#include "command.h"
#include "utils.h"

// TODO: Support a `--check` option to do an assert like Deno does
auto intelligence::jsonschema::cli::fmt(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};
  for (const auto &entry : for_each_schema(options.at(""))) {
    std::cerr << "Formatting: " << entry.first.string() << "\n";
    std::ofstream output{entry.first};
    sourcemeta::jsontoolkit::prettify(
        entry.second, output, sourcemeta::jsontoolkit::schema_format_compare);
    output << std::endl;
  }

  return EXIT_SUCCESS;
}
