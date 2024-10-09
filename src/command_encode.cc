#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/runtime.h>
#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>    // EXIT_SUCCESS
#include <filesystem> // std::filesystem
#include <fstream>    // std::ofstream
#include <iostream>   // std::cout, std::endl

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::encode(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};

  if (options.at("").size() < 2) {
    std::cerr
        << "error: This command expects a path to a JSON document and an "
           "output path. For example:\n\n"
        << "  jsonschema encode path/to/document.json path/to/output.binpack\n";
    return EXIT_FAILURE;
  }

  // TODO: Take a real schema as argument
  auto schema{sourcemeta::jsontoolkit::parse(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema"
  })JSON")};

  sourcemeta::jsonbinpack::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker,
      resolver(options, options.contains("h") || options.contains("http")));
  const auto encoding{sourcemeta::jsonbinpack::load(schema)};

  const auto document{
      sourcemeta::jsontoolkit::from_file(options.at("").front())};

  std::ofstream output_stream(
      std::filesystem::weakly_canonical(options.at("").at(1)),
      std::ios::binary);
  output_stream.exceptions(std::ios_base::badbit);
  sourcemeta::jsonbinpack::Encoder encoder{output_stream};
  encoder.write(document, encoding);
  output_stream.flush();
  const auto size{output_stream.tellp()};
  output_stream.close();
  std::cerr << "size: " << size << " bytes\n";
  return EXIT_SUCCESS;
}
