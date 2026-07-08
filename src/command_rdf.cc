#include <sourcemeta/blaze/foundation.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include <filesystem>    // std::filesystem
#include <iostream>      // std::cout, std::cerr
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::unreachable, std::move
#include <variant>       // std::get, std::holds_alternative

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto facet_name(const sourcemeta::blaze::JSONLDFacet facet)
    -> std::string_view {
  switch (facet) {
    case sourcemeta::blaze::JSONLDFacet::Type:
      return "type";
    case sourcemeta::blaze::JSONLDFacet::Predicate:
      return "predicate";
    case sourcemeta::blaze::JSONLDFacet::Datatype:
      return "datatype";
    case sourcemeta::blaze::JSONLDFacet::Language:
      return "language";
    case sourcemeta::blaze::JSONLDFacet::Direction:
      return "direction";
    case sourcemeta::blaze::JSONLDFacet::Graph:
      return "graph";
    case sourcemeta::blaze::JSONLDFacet::JSON:
      return "json";
    case sourcemeta::blaze::JSONLDFacet::Container:
      return "container";
    case sourcemeta::blaze::JSONLDFacet::Self:
      return "self";
    default:
      std::unreachable();
  }
}

auto assert_annotations_support(
    const sourcemeta::blaze::SchemaFrame &frame,
    const std::filesystem::path &schema_resolution_base) -> void {
  const auto &root_location{frame.locations().at(
      {sourcemeta::blaze::SchemaReferenceType::Static, frame.root()})};
  switch (root_location.base_dialect) {
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12:
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2020_12_Hyper:
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09:
    case sourcemeta::blaze::SchemaBaseDialect::JSON_Schema_2019_09_Hyper:
      return;
    default:
      throw sourcemeta::jsonschema::UnsupportedDialectRdfError{
          schema_resolution_base, std::string{root_location.dialect}};
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

  const auto configuration_path{find_configuration(schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  auto parsed_schema{schema_from_stdin ? read_from_stdin()
                                       : read_file(schema_path)};

  if (!sourcemeta::blaze::is_schema(parsed_schema.document)) {
    throw NotSchemaError{schema_from_stdin ? stdin_path()
                                           : schema_resolution_base};
  }

  const auto &schema{parsed_schema.document};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};
  const auto fast_mode{options.contains("fast")};
  const auto schema_default_id{
      sourcemeta::jsonschema::default_id(schema_resolution_base)};

  const auto bundled{
      bundle_for_evaluation(schema, custom_resolver, dialect, schema_default_id,
                            schema_resolution_base, parsed_schema.positions)};

  sourcemeta::blaze::SchemaFrame frame{
      sourcemeta::blaze::SchemaFrame::Mode::References};
  frame_for_evaluation(frame, bundled, custom_resolver, dialect,
                       schema_default_id, schema_resolution_base,
                       parsed_schema.positions);

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
          ? stdin_path().string()
          : sourcemeta::core::weakly_canonical(instance_path).string()};

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
          std::move(error.message),
          std::string{facet_name(error.facet)},
          std::move(error.instance_location),
          instance_from_stdin ? stdin_path() : instance_path};
    }

    throw RdfResolutionError{
        std::move(error.message), std::string{facet_name(error.facet)},
        std::move(error.instance_location),
        instance_from_stdin ? stdin_path() : instance_path};
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
                       << (schema_from_stdin
                               ? stdin_path().string()
                               : sourcemeta::core::weakly_canonical(
                                     schema_resolution_base)
                                     .string())
                       << "\n";
  sourcemeta::core::prettify(document, std::cout);
  std::cout << "\n";
}
