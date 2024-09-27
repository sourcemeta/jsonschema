#include "commands.h"
#include "defaults.h"

#include <sourcemeta/jsonbinpack/compiler.h>
#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>    // EXIT_SUCCESS
#include <filesystem> // std::filesystem
#include <iostream>   // std::cin, std::cout, std::endl;

static auto canonicalize_from_json(sourcemeta::jsontoolkit::JSON &schema)
    -> int {
  sourcemeta::jsonbinpack::canonicalize(
      schema, sourcemeta::jsontoolkit::default_schema_walker,
      sourcemeta::jsontoolkit::official_resolver,
      sourcemeta::jsonbinpack::DEFAULT_METASCHEMA);
  sourcemeta::jsontoolkit::prettify(schema, std::cout);
  std::cout << std::endl;
  return EXIT_SUCCESS;
}

auto sourcemeta::jsonbinpack::cli::canonicalize(
    const std::filesystem::path &schema_path) -> int {
  auto schema = sourcemeta::jsontoolkit::from_file(schema_path);
  return canonicalize_from_json(schema);
}

auto sourcemeta::jsonbinpack::cli::canonicalize() -> int {
  sourcemeta::jsontoolkit::JSON schema =
      sourcemeta::jsontoolkit::parse(std::cin);
  return canonicalize_from_json(schema);
}
