#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS
#include <iostream> // std::cout, std::endl

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::bundle(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"h", "http", "w", "without-id"})};
  CLI_ENSURE(!options.at("").empty(), "You must pass a JSON Schema as input");
  auto schema{sourcemeta::jsontoolkit::from_file(options.at("").front())};

  if (options.contains("w") || options.contains("without-id")) {
    log_verbose(options) << "Bundling without using identifiers\n";
    sourcemeta::jsontoolkit::bundle(
        schema, sourcemeta::jsontoolkit::default_schema_walker,
        resolver(options, options.contains("h") || options.contains("http")),
        sourcemeta::jsontoolkit::BundleOptions::WithoutIdentifiers)
        .wait();
  } else {
    sourcemeta::jsontoolkit::bundle(
        schema, sourcemeta::jsontoolkit::default_schema_walker,
        resolver(options, options.contains("h") || options.contains("http")),
        sourcemeta::jsontoolkit::BundleOptions::Default)
        .wait();
  }

  sourcemeta::jsontoolkit::prettify(
      schema, std::cout, sourcemeta::jsontoolkit::schema_format_compare);
  std::cout << std::endl;
  return EXIT_SUCCESS;
}
