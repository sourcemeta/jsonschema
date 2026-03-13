#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <iostream> // std::cout

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "resolver.h"
#include "utils.h"

namespace {
auto transformer_callback_noop(
    const sourcemeta::core::Pointer &, const std::string_view,
    const std::string_view,
    const sourcemeta::core::SchemaTransformRule::Result &, const bool) -> void {
}
} // namespace

auto sourcemeta::jsonschema::canonicalize(
    const sourcemeta::core::Options &options) -> void {

  if (options.positional().size() < 1) {
    throw PositionalArgumentError{
        "This command expects a path to a schema",
        "jsonschema canonicalize path/to/schema.json"};
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
  auto schema{schema_from_stdin
                  ? read_from_stdin().document
                  : sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_from_stdin ? stdin_error_path()
                                           : schema_resolution_base};
  }

  try {
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};

    sourcemeta::core::SchemaTransformer canonicalizer;
    sourcemeta::core::add(canonicalizer,
                          sourcemeta::core::AlterSchemaMode::Canonicalizer);
    [[maybe_unused]] const auto result = canonicalizer.apply(
        schema, sourcemeta::core::schema_walker, custom_resolver,
        transformer_callback_noop, dialect);
    assert(result.first);

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
