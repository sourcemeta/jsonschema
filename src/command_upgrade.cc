#include <sourcemeta/blaze/format.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/convert.h>

#include <filesystem>  // std::filesystem
#include <iostream>    // std::cout
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::get
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
    -> std::optional<sourcemeta::blaze::ConvertTarget> {
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

  throw sourcemeta::jsonschema::InvalidOptionEnumerationValueError{
      "The given target dialect is not supported",
      "to",
      {"draft4", "draft6", "draft7", "2019-09", "2020-12"}};
}

auto assert_upgradable(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::string &dialect, const std::string &default_id,
    const std::filesystem::path &schema_display_path,
    const sourcemeta::core::PointerPositionTracker &positions) -> void {
  std::optional<sourcemeta::core::SchemaFrame> frame;

  try {
    frame.emplace(sourcemeta::core::SchemaFrame::Mode::Locations, schema,
                  sourcemeta::core::schema_walker, resolver, dialect,
                  default_id);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        schema_display_path, error);
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    const auto position{positions.get(error.location())};
    if (position.has_value()) {
      throw sourcemeta::jsonschema::PositionError<sourcemeta::core::FileError<
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

  frame.value().for_each_location(
      [&schema_display_path, &positions](
          const sourcemeta::core::SchemaReferenceType, const std::string_view,
          const sourcemeta::core::SchemaFrame::Location &location) -> void {
        switch (location.base_dialect) {
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_7:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_7_HYPER:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_6:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_6_HYPER:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_4_HYPER:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3:
          case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_DRAFT_3_HYPER:
            return;
          default:
            break;
        }

        const auto unsupported_location_pointer{
            sourcemeta::core::to_pointer(location.pointer)};
        auto unsupported_dialect{std::string{location.dialect}};
        const auto unsupported_position{
            positions.get(unsupported_location_pointer)};
        if (unsupported_position.has_value()) {
          throw sourcemeta::jsonschema::PositionError<
              sourcemeta::jsonschema::UnsupportedDialectUpgradeError>{
              std::get<0>(unsupported_position.value()),
              std::get<1>(unsupported_position.value()), schema_display_path,
              unsupported_location_pointer, std::move(unsupported_dialect)};
        }

        throw sourcemeta::jsonschema::UnsupportedDialectUpgradeError{
            schema_display_path, unsupported_location_pointer,
            std::move(unsupported_dialect)};
      });

  frame.value().for_each_location(
      [&schema_display_path, &positions](
          const sourcemeta::core::SchemaReferenceType, const std::string_view,
          const sourcemeta::core::SchemaFrame::Location &location) -> void {
        if (sourcemeta::core::schema_is_known(location.dialect)) {
          return;
        }

        const auto custom_location_pointer{
            sourcemeta::core::to_pointer(location.pointer)};
        auto custom_dialect{std::string{location.dialect}};
        const auto position{positions.get(custom_location_pointer)};
        if (position.has_value()) {
          throw sourcemeta::jsonschema::PositionError<
              sourcemeta::jsonschema::CustomMetaschemaUpgradeError>{
              std::get<0>(position.value()), std::get<1>(position.value()),
              schema_display_path, custom_location_pointer,
              std::move(custom_dialect)};
        }

        throw sourcemeta::jsonschema::CustomMetaschemaUpgradeError{
            schema_display_path, custom_location_pointer,
            std::move(custom_dialect)};
      });
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
  const auto target_dialect_mode{parse_target_dialect(target_value)};

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
  auto parsed_schema{schema_from_stdin ? read_from_stdin()
                                       : read_file(schema_path)};

  if (!parsed_schema.document.is_object() &&
      !parsed_schema.document.is_boolean()) {
    throw NotSchemaError{schema_display_path};
  }

  auto &schema{parsed_schema.document};

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  assert_upgradable(
      schema, custom_resolver, dialect,
      sourcemeta::jsonschema::default_id(schema_path, schema_from_stdin),
      schema_display_path, parsed_schema.positions);

  if (target_dialect_mode.has_value()) {
    sourcemeta::blaze::convert(
        schema, sourcemeta::core::schema_walker, custom_resolver,
        target_dialect_mode.value(), dialect,
        sourcemeta::jsonschema::default_id(schema_path, schema_from_stdin),
        options.contains("meta"));
  }

  sourcemeta::blaze::format(schema, sourcemeta::core::schema_walker,
                            custom_resolver, dialect);

  sourcemeta::core::prettify(schema, std::cout);
  std::cout << "\n";
}
