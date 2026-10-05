#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/convert.h>

#include <filesystem>  // std::filesystem
#include <iostream>    // std::cout
#include <string>      // std::string
#include <string_view> // std::string_view

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto parse_target_dialect(const std::string_view value)
    -> sourcemeta::blaze::ConvertTarget {
  if (value == "draft4") {
    return sourcemeta::blaze::ConvertTarget::Draft4;
  }

  if (value == "draft6") {
    return sourcemeta::blaze::ConvertTarget::Draft6;
  }

  if (value == "draft7") {
    return sourcemeta::blaze::ConvertTarget::Draft7;
  }

  if (value == "2019-09") {
    return sourcemeta::blaze::ConvertTarget::Draft201909;
  }

  if (value == "2020-12") {
    return sourcemeta::blaze::ConvertTarget::Draft202012;
  }

  if (value == "openapi3.1") {
    return sourcemeta::blaze::ConvertTarget::OpenAPI31;
  }

  if (value == "openapi3.2") {
    return sourcemeta::blaze::ConvertTarget::OpenAPI32;
  }

  throw sourcemeta::jsonschema::InvalidOptionEnumerationValueError{
      "The given target dialect is not supported",
      "to",
      {"draft4", "draft6", "draft7", "2019-09", "2020-12", "openapi3.1",
       "openapi3.2"}};
}

} // namespace

auto sourcemeta::jsonschema::upgrade(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().empty()) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema upgrade path/to/schema.json"};
  }

  const auto target_value{options.contains("to") ? options.at("to").front()
                                                 : std::string_view{"2020-12"}};
  const auto target_dialect{parse_target_dialect(target_value)};
  const auto indentation{parse_optional_indentation(options)};

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw sourcemeta::core::IOIsADirectoryError{schema_path};
  }

  const auto schema_config_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};
  const auto schema_display_path{schema_from_stdin ? stdin_path()
                                                   : schema_path};

  const auto configuration_path{
      find_configuration(options, schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};
  auto parsed_schema{schema_from_stdin
                         ? read_from_stdin(nullptr, InputFormatting::Preserve)
                         : read_file(schema_path, InputFormatting::Preserve)};

  if (parsed_schema.multidocument) {
    throw MultiDocumentInputError{
        "This command does not support input with multiple documents",
        schema_display_path};
  }

  // An OpenAPI description is not a schema, so what it goes by wherever we
  // report on it is an identity of its own rather than the one a schema from
  // the same place would take
  if (is_openapi_document(parsed_schema.document)) {
    throw sourcemeta::core::FileError<UnsupportedOpenAPIUpgradeError>(
        schema_from_stdin ? openapi_stdin_path() : schema_path);
  }

  if (!parsed_schema.document.is_object() &&
      !parsed_schema.document.is_boolean()) {
    throw NotSchemaError{schema_display_path};
  }

  auto &schema{parsed_schema.document};

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  sourcemeta::jsonschema::upgrade_schema(
      schema, custom_resolver, target_dialect, dialect,
      sourcemeta::jsonschema::default_id(schema_path, schema_from_stdin),
      schema_display_path, parsed_schema.positions);

  sourcemeta::jsonschema::format_schema(schema, custom_resolver, dialect);

  sourcemeta::jsonschema::write_schema(schema, std::cout, indentation,
                                       parsed_schema.roundtrip);
}
