#include <sourcemeta/blaze/format.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/alterschema.h>

#include <filesystem>  // std::filesystem
#include <iostream>    // std::cout
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::ignore
#include <utility>     // std::move

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto parse_target_dialect(const std::string_view value)
    -> std::optional<sourcemeta::blaze::AlterSchemaMode> {
  if (value == "draft4") {
    return sourcemeta::blaze::AlterSchemaMode::UpgradeDraft4;
  } else if (value == "draft6") {
    return sourcemeta::blaze::AlterSchemaMode::UpgradeDraft6;
  } else if (value == "draft7") {
    return sourcemeta::blaze::AlterSchemaMode::UpgradeDraft7;
  } else if (value == "2019-09") {
    return sourcemeta::blaze::AlterSchemaMode::Upgrade201909;
  } else if (value == "2020-12") {
    return sourcemeta::blaze::AlterSchemaMode::Upgrade202012;
  }

  throw sourcemeta::jsonschema::InvalidOptionEnumerationValueError{
      "The given target dialect is not supported",
      "to",
      {"draft4", "draft6", "draft7", "2019-09", "2020-12"}};
}

} // namespace

auto sourcemeta::jsonschema::upgrade(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema upgrade path/to/schema.json"};
  }

  const auto target_value{options.contains("to") ? options.at("to").front()
                                                 : std::string_view{"2020-12"}};
  const auto target_dialect_mode{parse_target_dialect(target_value)};

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

  sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::Locations};

  try {
    frame.analyse(schema, sourcemeta::blaze::schema_walker, custom_resolver,
                  dialect,
                  sourcemeta::jsonschema::default_id(schema_resolution_base));
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

  for (const auto &entry : frame.locations()) {
    switch (entry.second.base_dialect) {
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_7:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_7_Hyper:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_6:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_6_Hyper:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_4:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_4_Hyper:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_3:
      case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_Draft_3_Hyper:
        continue;
      default:
        break;
    }

    const auto unsupported_location_pointer{
        sourcemeta::core::to_pointer(entry.second.pointer)};
    auto unsupported_dialect{std::string{entry.second.dialect}};
    const auto unsupported_position{
        parsed_schema.positions.get(unsupported_location_pointer)};
    if (unsupported_position.has_value()) {
      throw PositionError<UnsupportedDialectUpgradeError>{
          std::get<0>(unsupported_position.value()),
          std::get<1>(unsupported_position.value()), schema_display_path,
          unsupported_location_pointer, std::move(unsupported_dialect)};
    }

    throw UnsupportedDialectUpgradeError{schema_display_path,
                                         unsupported_location_pointer,
                                         std::move(unsupported_dialect)};
  }

  for (const auto &entry : frame.locations()) {
    if (sourcemeta::blaze::is_known_schema(entry.second.dialect)) {
      continue;
    }

    const auto custom_location_pointer{
        sourcemeta::core::to_pointer(entry.second.pointer)};
    auto custom_dialect{std::string{entry.second.dialect}};
    const auto position{parsed_schema.positions.get(custom_location_pointer)};
    if (position.has_value()) {
      throw PositionError<CustomMetaschemaUpgradeError>{
          std::get<0>(position.value()), std::get<1>(position.value()),
          schema_display_path, custom_location_pointer,
          std::move(custom_dialect)};
    }

    throw CustomMetaschemaUpgradeError{schema_display_path,
                                       custom_location_pointer,
                                       std::move(custom_dialect)};
  }

  if (target_dialect_mode.has_value()) {
    sourcemeta::blaze::SchemaTransformer transformer;
    sourcemeta::blaze::add(transformer, target_dialect_mode.value());
    std::ignore = transformer.apply(
        schema, sourcemeta::blaze::schema_walker, custom_resolver,
        [](const auto &, const auto, const auto, const auto &, const auto) {},
        dialect, sourcemeta::jsonschema::default_id(schema_resolution_base), "",
        options.contains("meta"));
  }

  sourcemeta::blaze::format(schema, sourcemeta::blaze::schema_walker,
                            custom_resolver, dialect);

  sourcemeta::core::prettify(schema, std::cout);
  std::cout << "\n";
}
