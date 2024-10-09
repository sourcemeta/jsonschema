#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/runtime.h>
#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cassert>    // assert
#include <cstdlib>    // EXIT_SUCCESS
#include <filesystem> // std::filesystem
#include <fstream>    // std::ifstream
#include <iostream>   // std::cout, std::endl

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::decode(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};

  if (options.at("").size() < 2) {
    std::cerr
        << "error: This command expects a path to a binary file and an "
           "output path. For example:\n\n"
        << "  jsonschema decode path/to/output.binpack path/to/document.json\n";
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

  std::ifstream input_stream{std::filesystem::canonical(options.at("").front()),
                             std::ios::binary};
  input_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  assert(!input_stream.fail());
  assert(input_stream.is_open());
  sourcemeta::jsonbinpack::Decoder decoder{input_stream};
  const auto document{decoder.read(encoding)};

  std::ofstream output_stream(
      std::filesystem::weakly_canonical(options.at("").at(1)),
      std::ios::binary);
  output_stream.exceptions(std::ios_base::badbit);
  sourcemeta::jsontoolkit::prettify(
      document, output_stream, sourcemeta::jsontoolkit::schema_format_compare);
  output_stream << "\n";
  output_stream.flush();
  output_stream.close();
  return EXIT_SUCCESS;
}
