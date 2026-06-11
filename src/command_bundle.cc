#include <sourcemeta/blaze/bundle.h>
#include <sourcemeta/blaze/editor.h>
#include <sourcemeta/blaze/format.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/yaml.h>

#include <iostream> // std::cout

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

  validate_http_headers(options);

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw sourcemeta::core::IOIsADirectoryError{schema_path};
  }

  const auto schema_resolution_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};
  const auto schema_display_path{schema_from_stdin ? stdin_path()
                                                   : schema_path};

  const auto configuration_path{find_configuration(schema_resolution_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_resolution_base)};
  const auto dialect{default_dialect(options, configuration)};
  auto parsed_schema{schema_from_stdin ? read_from_stdin()
                                       : read_file(schema_path)};

  if (!sourcemeta::blaze::is_schema(parsed_schema.document)) {
    throw NotSchemaError{schema_display_path};
  }

  auto &schema{parsed_schema.document};

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  try {
    sourcemeta::blaze::bundle(
        schema, sourcemeta::blaze::schema_walker, custom_resolver,
        sourcemeta::blaze::BundleMode::NonOfficialMetaschemas, dialect,
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

      sourcemeta::blaze::for_editor(schema, sourcemeta::blaze::schema_walker,
                                    custom_resolver, dialect);
    }

    sourcemeta::blaze::format(schema, sourcemeta::blaze::schema_walker,
                              custom_resolver, dialect);
  } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
        schema_display_path, error);
  } catch (const sourcemeta::blaze::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
        schema_display_path, error);
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    const auto position{parsed_schema.positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_display_path, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>(schema_display_path,
                                                       error);
  } catch (const sourcemeta::blaze::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaReferenceError>(
        schema_display_path, error.identifier(), error.location(),
        error.what());
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
        schema_display_path, error);
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>(
        schema_display_path, error);
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>(schema_display_path);
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>(schema_display_path);
  } catch (const sourcemeta::blaze::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
        schema_display_path, error.what());
  }

  sourcemeta::core::prettify(schema, std::cout);
  std::cout << "\n";
}
