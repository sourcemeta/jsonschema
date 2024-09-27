#include "commands.h"
#include "defaults.h"

#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsonbinpack/runtime.h>

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>    // EXIT_SUCCESS
#include <filesystem> // std::filesystem
#include <fstream>    // std::ifstream
#include <ios>        // std::ios_base, std::ios::binary
#include <iostream>   // std::cin, std::cout
#include <istream>    // std::basic_istream

template <typename CharT, typename Traits>
static auto decode_from_stream(sourcemeta::jsontoolkit::JSON &schema,
                               std::basic_istream<CharT, Traits> &stream)
    -> int {
  sourcemeta::jsonbinpack::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker,
      sourcemeta::jsontoolkit::official_resolver,
      sourcemeta::jsonbinpack::DEFAULT_METASCHEMA);
  const sourcemeta::jsonbinpack::Plan plan{
      sourcemeta::jsonbinpack::parse(schema)};
  sourcemeta::jsonbinpack::Decoder decoder{stream};
  const sourcemeta::jsontoolkit::JSON result = decoder.decode(plan);
  sourcemeta::jsontoolkit::stringify(result, std::cout);
  return EXIT_SUCCESS;
}

auto sourcemeta::jsonbinpack::cli::decode(
    const std::filesystem::path &schema_path,
    const std::filesystem::path &data_path) -> int {
  std::ifstream data_stream{data_path, std::ios::binary};
  data_stream.exceptions(std::ios_base::badbit);
  auto schema = sourcemeta::jsontoolkit::from_file(schema_path);
  return decode_from_stream(schema, data_stream);
}

auto sourcemeta::jsonbinpack::cli::decode(
    const std::filesystem::path &schema_path) -> int {
  auto schema = sourcemeta::jsontoolkit::from_file(schema_path);
  return decode_from_stream(schema, std::cin);
}
