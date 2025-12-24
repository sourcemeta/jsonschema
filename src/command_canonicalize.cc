#include <sourcemeta/core/alterschema.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <iostream> // std::cout

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"
#include "utils.h"

namespace {
auto transformer_callback_noop(
    const sourcemeta::core::Pointer &, const std::string_view,
    const std::string_view,
    const sourcemeta::core::SchemaTransformRule::Result &) -> void {}
} // namespace

auto sourcemeta::jsonschema::canonicalize(
    const sourcemeta::core::Options &options) -> void {

  if (options.positional().size() < 1) {
    throw PositionalArgumentError{
        "This command expects a path to a schema",
        "jsonschema canonicalize path/to/schema.json"};
  }

  const std::filesystem::path schema_path{options.positional().front()};
  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_path)};
  const auto dialect{default_dialect(options, configuration)};
  auto schema{sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_path};
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
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw FileError<sourcemeta::core::SchemaReferenceError>(
        schema_path, std::string{error.identifier()}, error.location(),
        error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaResolutionError>(schema_path,
                                                             error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_path);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw FileError<sourcemeta::core::SchemaError>(schema_path, error.what());
  }

  sourcemeta::core::prettify(schema, std::cout);
  std::cout << "\n";
}
