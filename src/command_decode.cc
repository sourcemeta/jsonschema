#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/runtime.h>

#include <cassert>    // assert
#include <filesystem> // std::filesystem
#include <fstream>    // std::ifstream
#include <iostream>   // std::cout, std::endl

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "logger.h"
#include "resolver.h"
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

auto sourcemeta::jsonschema::decode(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 2) {
    throw PositionalArgumentError{
        "This command expects a path to a binary file and an output path",
        "jsonschema decode path/to/output.binpack path/to/document.json"};
  }

  // TODO: Take a real schema as argument
  auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema"
  })JSON")};

  const auto configuration_path{
      find_configuration(options.positional().front())};
  const auto &configuration{read_configuration(options, configuration_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  sourcemeta::jsonbinpack::compile(
      schema, sourcemeta::core::schema_official_walker, custom_resolver);
  const auto encoding{sourcemeta::jsonbinpack::load(schema)};

  std::ifstream input_stream{
      sourcemeta::core::weakly_canonical(options.positional().front()),
      std::ios::binary};
  assert(!input_stream.fail());
  assert(input_stream.is_open());

  const std::filesystem::path output{options.positional().at(1)};
  std::ofstream output_stream(sourcemeta::core::weakly_canonical(output),
                              std::ios::binary);
  output_stream.exceptions(std::ios_base::badbit);
  sourcemeta::jsonbinpack::Decoder decoder{input_stream};

  if (output.extension() == ".jsonl") {
    LOG_VERBOSE(options) << "Interpreting input as JSONL: "
                         << sourcemeta::core::weakly_canonical(
                                options.positional().front())
                                .string()
                         << "\n";

    std::size_t count{0};
    while (has_data(input_stream)) {
      LOG_VERBOSE(options) << "Decoding entry #" << count << "\n";
      auto document{decoder.read(encoding)};
      if (count > 0) {
        output_stream << "\n";
      }

      sourcemeta::core::prettify(document, output_stream);
      count += 1;
    }
  } else {
    auto document{decoder.read(encoding)};
    sourcemeta::core::prettify(document, output_stream);
  }

  output_stream << "\n";
  output_stream.flush();
  output_stream.close();
}
