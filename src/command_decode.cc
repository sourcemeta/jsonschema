#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/runtime.h>

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
  auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema"
  })JSON")};

  const auto dialect{default_dialect(options)};
  sourcemeta::jsonbinpack::compile(
      schema, sourcemeta::core::schema_official_walker,
      resolver(options, options.contains("h") || options.contains("http"),
               dialect));
  const auto encoding{sourcemeta::jsonbinpack::load(schema)};

  std::ifstream input_stream{sourcemeta::jsonschema::cli::safe_weakly_canonical(
                                 options.at("").front()),
                             std::ios::binary};
  assert(!input_stream.fail());
  assert(input_stream.is_open());

  const std::filesystem::path output{options.at("").at(1)};
  std::ofstream output_stream(safe_weakly_canonical(output), std::ios::binary);
  output_stream.exceptions(std::ios_base::badbit);
  sourcemeta::jsonbinpack::Decoder decoder{input_stream};

  if (output.extension() == ".jsonl") {
    log_verbose(options)
        << "Interpreting input as JSONL: "
        << safe_weakly_canonical(options.at("").front()).string() << "\n";

    std::size_t count{0};
    while (has_data(input_stream)) {
      log_verbose(options) << "Decoding entry #" << count << "\n";
      const auto document{decoder.read(encoding)};
      if (count > 0) {
        output_stream << "\n";
      }

      sourcemeta::core::prettify(document, output_stream,
                                 sourcemeta::core::schema_format_compare);
      count += 1;
    }
  } else {
    const auto document{decoder.read(encoding)};
    sourcemeta::core::prettify(document, output_stream,
                               sourcemeta::core::schema_format_compare);
  }

  output_stream << "\n";
  output_stream.flush();
  output_stream.close();

  return EXIT_SUCCESS;
}
