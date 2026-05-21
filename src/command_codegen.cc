#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
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

  validate_http_headers(options);

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
  auto parsed_schema{read_file(schema_path)};
  const auto &schema{parsed_schema.document};

  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  sourcemeta::blaze::CodegenIRResult result;
  try {
    result = sourcemeta::blaze::compile(
        schema, sourcemeta::blaze::schema_walker, custom_resolver,
        sourcemeta::blaze::default_compiler, dialect,
        sourcemeta::jsonschema::default_id(schema_path));
  } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
        schema_path, error);
  } catch (const sourcemeta::blaze::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
        schema_path, error);
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    const auto position{parsed_schema.positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_path, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>(schema_path, error);
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>(
        schema_path, error);
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>(schema_path);
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>(schema_path);
  } catch (const sourcemeta::blaze::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
        schema_path, error.what());
  } catch (const sourcemeta::blaze::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaVocabularyError>(
        schema_path, error.uri(), error.what());
  } catch (const sourcemeta::blaze::CodegenUnsupportedKeywordError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CodegenUnsupportedKeywordError>(
        schema_path, error.json(), error.pointer(),
        std::string{error.keyword()}, error.what());
  } catch (
      const sourcemeta::blaze::CodegenUnsupportedKeywordValueError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CodegenUnsupportedKeywordValueError>(
        schema_path, error.json(), error.pointer(),
        std::string{error.keyword()}, error.what());
  } catch (const sourcemeta::blaze::CodegenUnexpectedSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CodegenUnexpectedSchemaError>(
        schema_path, error.json(), error.pointer(), error.what());
  }

  std::ostringstream output;
  if (options.contains("name")) {
    sourcemeta::blaze::generate<sourcemeta::blaze::TypeScript>(
        output, result, options.at("name").front());
  } else {
    sourcemeta::blaze::generate<sourcemeta::blaze::TypeScript>(output, result);
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
