#include <sourcemeta/codegen/generator.h>
#include <sourcemeta/codegen/ir.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <iostream>  // std::cout
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::codegen(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema codegen path/to/schema.json "
                                  "--name MyType --target typescript"};
  }

  if (!options.contains("target")) {
    throw OptionConflictError{
        "You must pass a target using the `--target/-t` option"};
  }

  const auto &target{options.at("target").front()};
  if (target != "typescript") {
    throw InvalidOptionEnumerationValueError{
        "Unknown code generation target", "target", {"typescript"}};
  }

  const std::filesystem::path schema_path{options.positional().front()};
  const sourcemeta::core::JSON schema{
      sourcemeta::core::read_yaml_or_json(schema_path)};

  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  sourcemeta::codegen::IRResult result;
  try {
    result = sourcemeta::codegen::compile(
        schema, sourcemeta::core::schema_walker, custom_resolver,
        sourcemeta::codegen::default_compiler, dialect,
        sourcemeta::jsonschema::default_id(schema_path));
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw FileError<sourcemeta::core::SchemaKeywordError>(schema_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw FileError<sourcemeta::core::SchemaFrameError>(schema_path, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaResolutionError>(schema_path,
                                                             error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_path);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownDialectError>(schema_path);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw FileError<sourcemeta::core::SchemaError>(schema_path, error.what());
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    throw FileError<sourcemeta::core::SchemaVocabularyError>(
        schema_path, std::string{error.uri()}, error.what());
  } catch (const sourcemeta::codegen::UnsupportedKeywordError &error) {
    throw FileError<sourcemeta::codegen::UnsupportedKeywordError>(
        schema_path, error.json(), error.pointer(),
        std::string{error.keyword()}, error.what());
  } catch (const sourcemeta::codegen::UnsupportedKeywordValueError &error) {
    throw FileError<sourcemeta::codegen::UnsupportedKeywordValueError>(
        schema_path, error.json(), error.pointer(),
        std::string{error.keyword()}, error.what());
  } catch (const sourcemeta::codegen::UnexpectedSchemaError &error) {
    throw FileError<sourcemeta::codegen::UnexpectedSchemaError>(
        schema_path, error.json(), error.pointer(), error.what());
  }

  std::ostringstream output;
  if (options.contains("name")) {
    sourcemeta::codegen::generate<sourcemeta::codegen::TypeScript>(
        output, result, options.at("name").front());
  } else {
    sourcemeta::codegen::generate<sourcemeta::codegen::TypeScript>(output,
                                                                   result);
  }

  if (options.contains("json")) {
    auto json_output{sourcemeta::core::JSON::make_object()};
    json_output.assign("code", sourcemeta::core::JSON{output.str()});
    sourcemeta::core::prettify(json_output, std::cout);
    std::cout << "\n";
  } else {
    std::cout << output.str();
  }
}
