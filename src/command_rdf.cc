#include <sourcemeta/blaze/foundation.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cassert>       // assert
#include <filesystem>    // std::filesystem
#include <iostream>      // std::cout, std::cerr
#include <string>        // std::string
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <variant>       // std::get, std::holds_alternative

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto assert_annotations_support(
    const sourcemeta::blaze::SchemaFrame &frame,
    const std::filesystem::path &schema_resolution_base) -> void {
  const auto root_location{frame.root_location()};
  assert(root_location.has_value());
  switch (root_location.value().get().base_dialect) {
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12:
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09:
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      return;
    default:
      throw sourcemeta::jsonschema::UnsupportedDialectRdfError{
          schema_resolution_base,
          std::string{root_location.value().get().dialect}};
  }
}

} // namespace

auto sourcemeta::jsonschema::rdf(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() != 2) {
    throw PositionalArgumentError{
        "This command expects a path to a schema and a path to an instance "
        "to promote to JSON-LD",
        "jsonschema rdf path/to/schema.json path/to/instance.json"};
  }

  validate_http_headers(options);

  const auto &schema_path{options.positional().at(0)};
  const auto &instance_path_view{options.positional().at(1)};
  const std::filesystem::path instance_path{instance_path_view};
  const bool schema_from_stdin{schema_path == "-"};
  const bool instance_from_stdin{instance_path_view == "-"};

  check_no_duplicate_stdin(options.positional());

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw sourcemeta::core::IOIsADirectoryError{schema_path};
  }

  if (!instance_from_stdin && std::filesystem::is_directory(instance_path)) {
    throw sourcemeta::core::IOIsADirectoryError{instance_path};
  }

  const auto schema_config_base{schema_from_stdin
                                    ? std::filesystem::current_path()
                                    : std::filesystem::path(schema_path)};
  const auto schema_resolution_base{
      schema_from_stdin ? stdin_path() : std::filesystem::path(schema_path)};

  const auto configuration_path{
      find_configuration(options, schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  auto parsed_schema{schema_from_stdin ? read_from_stdin()
                                       : read_file(schema_path)};

  if (!parsed_schema.document.is_object() &&
      !parsed_schema.document.is_boolean()) {
    throw NotSchemaError{schema_from_stdin ? stdin_path()
                                           : schema_resolution_base};
  }

  const auto &schema{parsed_schema.document};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};
  const auto fast_mode{options.contains("fast")};
  const auto schema_default_id{sourcemeta::jsonschema::default_id(
      schema_resolution_base, schema_from_stdin)};

  const auto bundled{
      bundle_for_evaluation(schema, custom_resolver, dialect, schema_default_id,
                            schema_resolution_base, parsed_schema.positions)};

  const auto frame{
      frame_for_evaluation(bundled, custom_resolver, dialect, schema_default_id,
                           schema_resolution_base, parsed_schema.positions)};

  assert_annotations_support(frame, schema_resolution_base);

  auto tweaks{
      format_assertion_tweaks(options).value_or(sourcemeta::blaze::Tweaks{})};
  tweaks.annotations = std::unordered_set<sourcemeta::core::JSON::StringView>(
      sourcemeta::blaze::JSONLD_KEYWORDS.begin(),
      sourcemeta::blaze::JSONLD_KEYWORDS.end());

  const auto schema_template{compile_for_evaluation(
      bundled, custom_resolver, frame, std::string{frame.root()},
      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                : sourcemeta::blaze::Mode::Exhaustive,
      tweaks, schema_resolution_base, parsed_schema.positions)};

  const auto parsed_instance{instance_from_stdin ? read_from_stdin()
                                                 : read_file(instance_path)};
  const auto &instance{parsed_instance.document};
  const auto instance_display_path{
      instance_from_stdin
          ? std::string{STDIN_DEFAULT_ID}
          : sourcemeta::core::weakly_canonical(instance_path).generic_string()};

  sourcemeta::blaze::Evaluator evaluator;
  auto outcome{sourcemeta::blaze::jsonld(evaluator, schema_template, instance)};
  const auto json_output{options.contains("json")};

  if (std::holds_alternative<sourcemeta::blaze::JSONLDInvalid>(outcome)) {
    if (json_output) {
      const auto suboutput{sourcemeta::blaze::standard(
          evaluator, schema_template, instance,
          fast_mode ? sourcemeta::blaze::StandardOutput::Flag
                    : sourcemeta::blaze::StandardOutput::Basic,
          parsed_instance.positions)};
      sourcemeta::core::prettify(suboutput, std::cout);
      std::cout << "\n";
    } else {
      std::cerr << "fail: " << instance_display_path << "\n";
      print(std::get<sourcemeta::blaze::JSONLDInvalid>(outcome),
            parsed_instance.positions, std::cerr);
    }

    throw Fail{EXIT_EXPECTED_FAILURE};
  }

  if (std::holds_alternative<sourcemeta::blaze::JSONLDResolutionError>(
          outcome)) {
    auto &error{std::get<sourcemeta::blaze::JSONLDResolutionError>(outcome)};
    const auto position{parsed_instance.positions.get(error.instance_location)};
    if (position.has_value()) {
      throw PositionError<RdfResolutionError>{
          std::get<0>(position.value()),
          std::get<1>(position.value()),
          error.message,
          std::string{facet_name(error.facet)},
          std::move(error.instance_location),
          std::move(error.schema_location),
          std::move(error.conflicting_schema_location),
          std::move(error.inert_override_location),
          instance_from_stdin ? stdin_path() : instance_path};
    }

    throw RdfResolutionError{error.message,
                             std::string{facet_name(error.facet)},
                             std::move(error.instance_location),
                             std::move(error.schema_location),
                             std::move(error.conflicting_schema_location),
                             std::move(error.inert_override_location),
                             instance_from_stdin ? stdin_path()
                                                 : instance_path};
  }

  auto document{std::get<sourcemeta::core::JSON>(std::move(outcome))};
  const auto flatten{options.contains("flatten")};
  const auto compact{options.contains("compact") &&
                     !options.at("compact").empty()};

  if (compact) {
    const std::filesystem::path context_path{options.at("compact").front()};
    const auto parsed_context{read_file(context_path)};

    // Compacting an empty document exercises context processing on its own,
    // so context errors are attributed to the context file while errors on
    // the real run below are attributed to the instance that produced the
    // offending document
    try {
      [[maybe_unused]] const auto probe{sourcemeta::core::jsonld_compact(
          sourcemeta::core::JSON::make_array(), parsed_context.document)};
    } catch (const sourcemeta::core::JSONLDError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::JSONLDError>(
          context_path, error);
    }

    try {
      document = flatten ? sourcemeta::core::jsonld_flatten(
                               document, parsed_context.document)
                         : sourcemeta::core::jsonld_compact(
                               document, parsed_context.document);
    } catch (const sourcemeta::core::JSONLDError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::JSONLDError>(
          instance_from_stdin ? stdin_path() : instance_path, error);
    }
  } else if (flatten) {
    try {
      document = sourcemeta::core::jsonld_flatten(document);
    } catch (const sourcemeta::core::JSONLDError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::JSONLDError>(
          instance_from_stdin ? stdin_path() : instance_path, error);
    }
  }

  LOG_VERBOSE(options) << "ok: " << instance_display_path << "\n  matches "
                       << stdin_path_string(schema_resolution_base) << "\n";
  sourcemeta::core::prettify(document, std::cout);
  std::cout << "\n";
}
