#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <cassert>   // assert
#include <iostream>  // std::cout
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <string>    // std::string

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto assert_dialect_support(const sourcemeta::core::JSON &schema,
                            const sourcemeta::core::SchemaResolver &resolver,
                            const std::string &dialect,
                            const std::string &default_id,
                            const std::filesystem::path &schema_path) -> void {
  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::Root,
      schema,
      sourcemeta::core::schema_walker,
      resolver,
      dialect,
      default_id};
  const auto root_location{frame.root_location()};
  assert(root_location.has_value());
  if (root_location.value().get().base_dialect ==
      sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12) {
    return;
  }

  throw sourcemeta::jsonschema::UnsupportedDialectCodegenError{
      schema_path, std::string{root_location.value().get().dialect}};
}

} // namespace

auto sourcemeta::jsonschema::codegen(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().empty()) {
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

  const auto configuration_path{find_configuration(options, schema_path)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};
  const auto schema_default_id{
      sourcemeta::jsonschema::default_id(schema_path, false)};

  sourcemeta::blaze::CodegenIRResult result;
  try {
    assert_dialect_support(schema, custom_resolver, dialect, schema_default_id,
                           schema_path);
    result = sourcemeta::blaze::compile(
        schema, sourcemeta::core::schema_walker, custom_resolver,
        sourcemeta::blaze::default_compiler, dialect, schema_default_id);
  } catch (const sourcemeta::blaze::CompilerError &error) {
    const auto position{parsed_schema.positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_path, error);
    }

    throw sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{parsed_schema.positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_path, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(schema_path, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(schema_path);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(schema_path);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        schema_path, error.what());
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaVocabularyError>(
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
