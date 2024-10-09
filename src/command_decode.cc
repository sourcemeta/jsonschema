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

static auto has_data(std::ifstream &stream) -> bool {
  if (!stream.is_open()) {
    return false;
  }

  std::streampos current_pos = stream.tellg();
  stream.seekg(0, std::ios::end);
  std::streampos end_pos = stream.tellg();
  stream.seekg(current_pos);

  return (current_pos < end_pos) && stream.good();
}

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
  assert(!input_stream.fail());
  assert(input_stream.is_open());

  const std::filesystem::path output{options.at("").at(1)};
  std::ofstream output_stream(std::filesystem::weakly_canonical(output),
                              std::ios::binary);
  output_stream.exceptions(std::ios_base::badbit);
  sourcemeta::jsonbinpack::Decoder decoder{input_stream};

  if (output.extension() == ".jsonl") {
    log_verbose(options)
        << "Interpreting input as JSONL: "
        << std::filesystem::weakly_canonical(options.at("").front()).string()
        << "\n";

    std::size_t count{0};
    while (has_data(input_stream)) {
      log_verbose(options) << "Decoding entry #" << count << "\n";
      const auto document{decoder.read(encoding)};
      if (count > 0) {
        output_stream << "\n";
      }

      sourcemeta::jsontoolkit::prettify(
          document, output_stream,
          sourcemeta::jsontoolkit::schema_format_compare);
      count += 1;
    }
  } else {
    const auto document{decoder.read(encoding)};
    sourcemeta::jsontoolkit::prettify(
        document, output_stream,
        sourcemeta::jsontoolkit::schema_format_compare);
  }

  output_stream << "\n";
  output_stream.flush();
  output_stream.close();

  return EXIT_SUCCESS;
}
