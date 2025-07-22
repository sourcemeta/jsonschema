#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/runtime.h>

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
  auto schema{sourcemeta::core::parse_json(R"JSON({
    "$schema": "https://json-schema.org/draft/2020-12/schema"
  })JSON")};

  const auto dialect{default_dialect(options)};
  sourcemeta::jsonbinpack::compile(
      schema, sourcemeta::core::schema_official_walker,
      resolver(options, options.contains("h") || options.contains("http"),
               dialect));
  const auto encoding{sourcemeta::jsonbinpack::load(schema)};

  const std::filesystem::path document{options.at("").front()};
  const auto original_size{std::filesystem::file_size(document)};
  std::cerr << "original file size: " << original_size << " bytes\n";

  if (document.extension() == ".jsonl") {
    log_verbose(options) << "Interpreting input as JSONL: "
                         << safe_weakly_canonical(document).string() << "\n";

    auto stream{sourcemeta::core::read_file(document)};
    std::ofstream output_stream(safe_weakly_canonical(options.at("").at(1)),
                                std::ios::binary);
    output_stream.exceptions(std::ios_base::badbit);
    sourcemeta::jsonbinpack::Encoder encoder{output_stream};
    std::size_t count{0};
    for (const auto &entry : sourcemeta::core::JSONL{stream}) {
      log_verbose(options) << "Encoding entry #" << count << "\n";
      encoder.write(entry, encoding);
      count += 1;
    }

    output_stream.flush();
    const auto total_size{output_stream.tellp()};
    output_stream.close();
    std::cerr << "encoded file size: " << total_size << " bytes\n";
    std::cerr << "compression ratio: "
              << (static_cast<std::uint64_t>(total_size) * 100 / original_size)
              << "%\n";
  } else {
    const auto entry{
        sourcemeta::jsonschema::cli::read_file(options.at("").front())};
    std::ofstream output_stream(safe_weakly_canonical(options.at("").at(1)),
                                std::ios::binary);
    output_stream.exceptions(std::ios_base::badbit);
    sourcemeta::jsonbinpack::Encoder encoder{output_stream};
    encoder.write(entry, encoding);
    output_stream.flush();
    const auto total_size{output_stream.tellp()};
    output_stream.close();
    std::cerr << "encoded file size: " << total_size << " bytes\n";
    std::cerr << "compression ratio: "
              << (static_cast<std::uint64_t>(total_size) * 100 / original_size)
              << "%\n";
  }

  return EXIT_SUCCESS;
}
