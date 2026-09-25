#include <sourcemeta/blaze/editor.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
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
  if (options.positional().empty()) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema bundle path/to/schema.json"};
  }

  validate_http_headers(options);
  const auto indentation{parse_optional_indentation(options)};

  const std::filesystem::path schema_path{options.positional().front()};
  const bool schema_from_stdin = (schema_path == "-");

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw sourcemeta::core::IOIsADirectoryError{schema_path};
  }

  const auto schema_config_base{
      schema_from_stdin ? std::filesystem::current_path() : schema_path};

  const auto configuration_path{
      find_configuration(options, schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};
  auto parsed_schema{schema_from_stdin
                         ? read_from_stdin(nullptr, InputFormatting::Preserve)
                         : read_file(schema_path, InputFormatting::Preserve)};

  // An OpenAPI description is not a schema, so what it goes by wherever we
  // report on it is an identity of its own rather than the one a schema from
  // the same place would take
  const auto is_openapi{
      sourcemeta::core::openapi_version(parsed_schema.document).has_value()};
  const auto schema_display_path{
      schema_from_stdin ? (is_openapi ? openapi_stdin_path() : stdin_path())
                        : schema_path};

  if (parsed_schema.multidocument) {
    throw MultiDocumentInputError{
        "This command does not support input with multiple documents",
        schema_display_path};
  }

  if (!parsed_schema.document.is_object() &&
      !parsed_schema.document.is_boolean()) {
    throw NotSchemaError{schema_display_path};
  }

  reject_unsupported_openapi(parsed_schema.document, schema_display_path);

  // Removing identifiers is a schema operation, and there is no description to
  // read once the Schema Objects it holds can no longer say who they are
  if (is_openapi && options.contains("without-id")) {
    throw sourcemeta::core::FileError<UnsupportedOpenAPIWithoutIdError>(
        schema_display_path);
  }

  auto &schema{parsed_schema.document};

  const auto openapi_base{sourcemeta::jsonschema::openapi_default_id(
      schema_path, schema_from_stdin)};

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  try {
    if (is_openapi) {
      sourcemeta::core::openapi_bundle(
          schema, sourcemeta::core::schema_walker, custom_resolver,
          openapi_resolver(options, options.contains("http"), dialect,
                           configuration),
          {.default_base = openapi_base});
      // TODO: Order the keys of a bundled description once Core grows the
      // equivalent of `sourcemeta::core::schema_format` for one. The shell of
      // a description has no ordering of its own to apply here
      sourcemeta::jsonschema::write_schema(schema, std::cout, indentation,
                                           parsed_schema.roundtrip);
      return;
    }

    sourcemeta::core::schema_bundle(
        schema, sourcemeta::core::schema_walker, custom_resolver, dialect,
        sourcemeta::jsonschema::default_id(schema_path, schema_from_stdin));

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

      sourcemeta::blaze::for_editor(schema, sourcemeta::core::schema_walker,
                                    custom_resolver, dialect);
    }

    sourcemeta::jsonschema::format_schema(schema, custom_resolver, dialect);
  } catch (const sourcemeta::core::OpenAPIResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIResolutionError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::OpenAPIReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIReferenceError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::OpenAPIError &error) {
    const auto position{error.base() == openapi_base
                            ? parsed_schema.positions.get(error.location())
                            : std::nullopt};
    if (position.has_value()) {
      throw PositionError<
          sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_display_path, error);
    }

    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{parsed_schema.positions.get(error.location())};
    if (position.has_value()) {
      throw PositionError<sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_display_path, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>(schema_display_path,
                                                      error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        schema_display_path, error.identifier(), error.location(),
        error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(schema_display_path);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(schema_display_path);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        schema_display_path, error.what());
  }

  sourcemeta::jsonschema::write_schema(schema, std::cout, indentation,
                                       parsed_schema.roundtrip);
}
