#include <sourcemeta/core/editorschema.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <iostream> // std::cin, std::cout
#include <sstream>  // std::ostringstream, std::istringstream

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::bundle(const sourcemeta::core::Options &options)
    -> void {

  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema bundle path/to/schema.json"};
  }

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw std::filesystem::filesystem_error{
        "The input was supposed to be a file but it is a directory",
        schema_path, std::make_error_code(std::errc::is_a_directory)};
  }

  const auto schema_resolution_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};

  const auto configuration_path{find_configuration(schema_resolution_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_resolution_base)};
  const auto dialect{default_dialect(options, configuration)};
  auto schema{
      schema_from_stdin ? [&]() {
        std::ostringstream buffer;
        buffer << std::cin.rdbuf();
        const auto input{buffer.str()};
        try {
          std::istringstream stream{input};
          return sourcemeta::core::parse_json(stream);
        } catch (const sourcemeta::core::JSONParseError &json_error) {
          try {
            std::istringstream stream{input};
            return sourcemeta::core::parse_yaml(stream);
          } catch (...) {
            throw json_error;
          }
        }
      }()
                        : sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_from_stdin ? stdin_error_path()
                                           : schema_resolution_base};
  }

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  try {
    sourcemeta::core::bundle(
        schema, sourcemeta::core::schema_walker, custom_resolver, dialect,
        sourcemeta::jsonschema::default_id(schema_resolution_base));

    if (options.contains("without-id")) {
      sourcemeta::jsonschema::LOG_WARNING()
          << "You are opting in to remove schema identifiers in "
             "the bundled schema.\n"
          << "The only legit use case of this advanced feature we know of "
             "is to workaround\n"
          << "non-compliant JSON Schema implementations such as Visual "
             "Studio Code.\n"
          << "Otherwise, this is not needed and may harm other use "
             "cases. For example,\n"
          << "you will be unable to reference the resulting schema from "
             "other schemas\n"
          << "using the --resolve/-r option.\n";

      sourcemeta::core::for_editor(schema, sourcemeta::core::schema_walker,
                                   custom_resolver, dialect);
    }

    sourcemeta::core::format(schema, sourcemeta::core::schema_walker,
                             custom_resolver, dialect);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw FileError<sourcemeta::core::SchemaKeywordError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw FileError<sourcemeta::core::SchemaFrameError>(schema_resolution_base,
                                                        error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw FileError<sourcemeta::core::SchemaReferenceError>(
        schema_resolution_base, std::string{error.identifier()},
        error.location(), error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw FileError<sourcemeta::core::SchemaError>(schema_resolution_base,
                                                   error.what());
  }

  sourcemeta::core::prettify(schema, std::cout);
  std::cout << "\n";
}
