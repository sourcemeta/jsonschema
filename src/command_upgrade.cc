#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/convert.h>

#include <filesystem>  // std::filesystem
#include <iostream>    // std::cout
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
    -> sourcemeta::blaze::ConvertTarget {
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

template <typename Error>
[[noreturn]] auto
throw_upgrade_error(const std::filesystem::path &schema_display_path,
                    const sourcemeta::core::PointerPositionTracker &positions,
                    sourcemeta::core::Pointer location, std::string uri)
    -> void {
  const auto position{positions.get(location)};
  if (position.has_value()) {
    throw sourcemeta::jsonschema::PositionError<Error>{
        std::get<0>(position.value()), std::get<1>(position.value()),
        schema_display_path, std::move(location), std::move(uri)};
  }

  throw Error{schema_display_path, std::move(location), std::move(uri)};
}

auto upgrade_schema(sourcemeta::core::JSON &schema,
                    const sourcemeta::core::SchemaResolver &resolver,
                    const sourcemeta::blaze::ConvertTarget target,
                    const std::string &dialect, const std::string &default_id,
                    const std::filesystem::path &schema_display_path,
                    const sourcemeta::core::PointerPositionTracker &positions)
    -> void {
  try {
    sourcemeta::blaze::convert(schema, sourcemeta::core::schema_walker,
                               resolver, target, dialect, default_id);
  } catch (const sourcemeta::blaze::ConvertUnsupportedMetaschemaError &error) {
    throw_upgrade_error<sourcemeta::jsonschema::MetaschemaUpgradeError>(
        schema_display_path, positions, error.location(),
        std::string{error.identifier()});
  } catch (const sourcemeta::blaze::ConvertUnsupportedDialectError &error) {
    // A dialect with no conversion rules is either one we have not taught
    // the CLI yet or one only the author of the schema knows about, which are
    // different problems to act on. Whether JSON Schema names the dialect is
    // what tells them apart
    if (sourcemeta::core::schema_is_known(error.identifier())) {
      throw_upgrade_error<
          sourcemeta::jsonschema::UnsupportedDialectUpgradeError>(
          schema_display_path, positions, error.location(),
          std::string{error.identifier()});
    }

    throw_upgrade_error<sourcemeta::jsonschema::CustomMetaschemaUpgradeError>(
        schema_display_path, positions, error.location(),
        std::string{error.identifier()});
  } catch (const sourcemeta::blaze::ConvertInvalidReferenceError &error) {
    throw_upgrade_error<sourcemeta::jsonschema::InvalidReferenceUpgradeError>(
        schema_display_path, positions, error.location(),
        std::string{error.identifier()});
  } catch (const sourcemeta::blaze::ConvertBrokenReferenceError &error) {
    throw_upgrade_error<sourcemeta::jsonschema::BrokenReferenceUpgradeError>(
        schema_display_path, positions, error.location(),
        std::string{error.identifier()});
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
  const auto target_dialect{parse_target_dialect(target_value)};

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

  upgrade_schema(
      schema, custom_resolver, target_dialect, dialect,
      sourcemeta::jsonschema::default_id(schema_path, schema_from_stdin),
      schema_display_path, parsed_schema.positions);

  sourcemeta::jsonschema::format_schema(schema, custom_resolver, dialect);

  sourcemeta::core::prettify(schema, std::cout);
  std::cout << "\n";
}
