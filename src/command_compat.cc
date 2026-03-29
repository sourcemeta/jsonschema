#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <filesystem> // std::filesystem
#include <iostream>   // std::cout
#include <ostream>    // std::ostream
#include <string>     // std::string

#include "command.h"
#include "compatibility.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto print_section(
    std::ostream &stream, const std::string &title,
    const std::vector<sourcemeta::jsonschema::CompatibilityChange> &changes)
    -> void {
  if (changes.empty()) {
    return;
  }

  stream << title << "\n\n";
  for (const auto &change : changes) {
    stream << "- " << change.message << "\n";
  }
}

auto bundle_schema(const sourcemeta::core::Options &options,
                   const std::filesystem::path &schema_path)
    -> sourcemeta::core::JSON {
  if (std::filesystem::is_directory(schema_path)) {
    throw std::filesystem::filesystem_error{
        "The input was supposed to be a file but it is a directory",
        schema_path, std::make_error_code(std::errc::is_a_directory)};
  }

  const sourcemeta::core::JSON schema{
      sourcemeta::core::read_yaml_or_json(schema_path)};
  if (!sourcemeta::core::is_schema(schema)) {
    throw sourcemeta::jsonschema::NotSchemaError{schema_path};
  }

  const auto configuration_path{
      sourcemeta::jsonschema::find_configuration(schema_path)};
  const auto &configuration{sourcemeta::jsonschema::read_configuration(
      options, configuration_path, schema_path)};
  const auto dialect{
      sourcemeta::jsonschema::default_dialect(options, configuration)};
  const sourcemeta::jsonschema::CustomResolver custom_resolver{
      options, configuration, options.contains("http"), dialect};

  try {
    return sourcemeta::core::bundle(
        schema, sourcemeta::core::schema_walker, custom_resolver, dialect,
        sourcemeta::jsonschema::default_id(schema_path));
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
        schema_path, std::string{error.identifier()}, error.location(),
        error.what());
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(schema_path,
                                                                   error);
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
  } catch (const sourcemeta::core::SchemaReferenceObjectResourceError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaReferenceObjectResourceError>(
        schema_path, error.identifier());
  }
}

auto print_human_report(
    const sourcemeta::jsonschema::CompatibilityReport &report) -> void {
  if (report.empty()) {
    std::cout << "No compatibility changes detected\n";
    return;
  }

  bool printed{false};
  if (!report.breaking.empty()) {
    print_section(std::cout, "Breaking Changes:", report.breaking);
    printed = true;
  }

  if (!report.warnings.empty()) {
    if (printed) {
      std::cout << "\n";
    }

    print_section(std::cout, "Warnings:", report.warnings);
    printed = true;
  }

  if (!report.safe.empty()) {
    if (printed) {
      std::cout << "\n";
    }

    print_section(std::cout, "Safe Changes:", report.safe);
  }
}

} // namespace

auto sourcemeta::jsonschema::compat(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() != 2) {
    throw PositionalArgumentError{
        "This command expects a path to the old schema and a path to the new "
        "schema",
        "jsonschema compat path/to/old_schema.json path/to/new_schema.json"};
  }

  const std::filesystem::path old_schema_path{options.positional().at(0)};
  const std::filesystem::path new_schema_path{options.positional().at(1)};

  const auto old_schema{bundle_schema(options, old_schema_path)};
  const auto new_schema{bundle_schema(options, new_schema_path)};

  const CompatibilityChecker checker;
  const auto report{checker.compare(old_schema, new_schema)};

  if (options.contains("json")) {
    sourcemeta::core::prettify(report.to_json(), std::cout);
    std::cout << "\n";
  } else {
    print_human_report(report);
  }

  if (options.contains("fail-on-breaking") && report.has_breaking_changes()) {
    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
