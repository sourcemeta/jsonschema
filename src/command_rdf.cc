#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cassert>       // assert
#include <cstddef>       // std::size_t
#include <filesystem>    // std::filesystem
#include <iostream>      // std::cout, std::cerr
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <variant>       // std::get, std::holds_alternative
#include <vector>        // std::vector

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto assert_annotations_support(
    const sourcemeta::core::SchemaFrame &frame,
    const std::string &entrypoint_uri,
    const std::filesystem::path &schema_resolution_base) -> void {
  // An entry point naming nothing is not ours to report on, as compiling it
  // says so in the terms the caller asked in
  const auto location{frame.traverse(entrypoint_uri)};
  if (!location.has_value()) {
    return;
  }

  switch (location.value().get().base_dialect) {
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2020_12_HYPER:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09:
    case sourcemeta::core::SchemaBaseDialect::JSON_SCHEMA_2019_09_HYPER:
      return;
    default:
      throw sourcemeta::jsonschema::UnsupportedDialectRdfError{
          schema_resolution_base, std::string{location.value().get().dialect}};
  }
}

// The plain path makes a whole schema of its own before compiling, which a
// description does not need, as reading one already draws in every document it
// spans
auto compile_plain_schema(
    const sourcemeta::core::JSON &schema,
    const sourcemeta::core::Options &options,
    const sourcemeta::core::SchemaResolver &resolver,
    const std::string &dialect, const std::string &schema_default_id,
    const std::filesystem::path &schema_resolution_base,
    const sourcemeta::core::PointerPositionTracker &positions,
    const bool fast_mode, const sourcemeta::blaze::Tweaks &tweaks)
    -> sourcemeta::blaze::Template {
  const auto bundled{sourcemeta::jsonschema::bundle_for_evaluation(
      schema, resolver, dialect, schema_default_id, schema_resolution_base,
      positions)};

  const auto frame{sourcemeta::jsonschema::frame_for_evaluation(
      bundled, resolver, dialect, schema_default_id, schema_resolution_base,
      positions)};

  // A schema has a root to fall back on, unlike a description, but a caller
  // may still single out one of the subschemas it holds
  std::string entrypoint_uri{frame.root()};
  if (options.contains("entrypoint") && !options.at("entrypoint").empty()) {
    try {
      entrypoint_uri = sourcemeta::jsonschema::resolve_entrypoint(
          frame, options.at("entrypoint").front());
    } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidEntryPoint>(schema_resolution_base,
                                                        error);
    }
  }

  assert_annotations_support(frame, entrypoint_uri, schema_resolution_base);

  return sourcemeta::jsonschema::compile_for_evaluation(
      bundled, resolver, frame, entrypoint_uri,
      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                : sourcemeta::blaze::Mode::Exhaustive,
      tweaks, schema_resolution_base, positions);
}

// Only these inputs can hold more than one document. Every other instance
// keeps the single document reader, so that an empty or malformed file reports
// the parse error it names rather than being passed over as an input
auto is_multidocument_input(const std::filesystem::path &path) -> bool {
  return path.extension() == ".jsonl" || path.string().ends_with(".jsonl.gz") ||
         path.extension() == ".yaml" || path.extension() == ".yml";
}

// TODO: This materialises every document of the input in memory before the
// first one is evaluated, which defeats streaming a large dataset. Every
// command that reads more than one document shares that shape, so move them
// all over to a streaming reader at once rather than giving this command an
// input path of its own
auto read_instances(const std::string_view instance_path_view,
                    const std::filesystem::path &instance_path,
                    const bool instance_from_stdin,
                    const sourcemeta::core::Options &options)
    -> std::vector<sourcemeta::jsonschema::InputJSON> {
  if (instance_from_stdin || is_multidocument_input(instance_path)) {
    return sourcemeta::jsonschema::for_each_json(
        {instance_path_view}, options,
        sourcemeta::jsonschema::InputRequirement::NonEmpty);
  }

  const auto canonical{sourcemeta::core::weakly_canonical(instance_path)};
  auto parsed{sourcemeta::jsonschema::read_file(instance_path)};
  std::vector<sourcemeta::jsonschema::InputJSON> result;
  result.push_back({.first = canonical.generic_string(),
                    .resolution_base = canonical,
                    .second = std::move(parsed.document),
                    .positions = std::move(parsed.positions),
                    .yaml = parsed.yaml,
                    .property_storage = std::move(parsed.property_storage)});
  return result;
}

template <typename Error>
[[noreturn]] auto throw_with_entry(const std::optional<std::size_t> &entry,
                                   Error error) -> void {
  if (entry.has_value()) {
    throw sourcemeta::jsonschema::EntryError<Error>{entry.value(),
                                                    std::move(error)};
  }

  throw std::move(error);
}

auto promote_entry(const sourcemeta::jsonschema::InputJSON &entry,
                   sourcemeta::blaze::Evaluator &evaluator,
                   const sourcemeta::blaze::Template &schema_template,
                   const std::optional<sourcemeta::core::JSON> &context,
                   const bool flatten, const bool fast_mode,
                   const bool json_output, const bool multidocument,
                   const std::filesystem::path &schema_resolution_base,
                   const sourcemeta::core::Options &options) -> void {
  const auto instance_display_path{
      sourcemeta::jsonschema::stdin_path_string(entry.resolution_base)};
  const std::optional<std::size_t> entry_index{
      multidocument ? std::optional<std::size_t>{entry.index + 1}
                    : std::nullopt};
  auto outcome{
      sourcemeta::blaze::jsonld(evaluator, schema_template, entry.second)};

  if (std::holds_alternative<sourcemeta::blaze::JSONLDInvalid>(outcome)) {
    if (json_output) {
      const auto suboutput{sourcemeta::blaze::standard(
          evaluator, schema_template, entry.second,
          fast_mode ? sourcemeta::blaze::StandardOutput::Flag
                    : sourcemeta::blaze::StandardOutput::Basic,
          entry.positions)};
      sourcemeta::core::prettify(suboutput, std::cout);
      std::cout << "\n";
    } else {
      std::cerr << "fail: " << instance_display_path;
      if (multidocument) {
        std::cerr << " (entry #" << entry.index + 1 << ")\n\n";
        sourcemeta::core::prettify(entry.second, std::cerr);
        std::cerr << "\n\n";
      } else {
        std::cerr << "\n";
      }

      sourcemeta::jsonschema::print(
          std::get<sourcemeta::blaze::JSONLDInvalid>(outcome), entry.positions,
          std::cerr);
    }

    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_EXPECTED_FAILURE};
  }

  if (std::holds_alternative<sourcemeta::blaze::JSONLDResolutionError>(
          outcome)) {
    auto &error{std::get<sourcemeta::blaze::JSONLDResolutionError>(outcome)};
    const auto position{entry.positions.get(error.instance_location)};
    if (position.has_value()) {
      throw_with_entry(
          entry_index,
          sourcemeta::jsonschema::PositionError<
              sourcemeta::jsonschema::RdfResolutionError>{
              std::get<0>(position.value()), std::get<1>(position.value()),
              error.message,
              std::string{sourcemeta::jsonschema::facet_name(error.facet)},
              std::move(error.instance_location),
              std::move(error.schema_location),
              std::move(error.conflicting_schema_location),
              std::move(error.inert_override_location), entry.resolution_base});
    }

    throw_with_entry(
        entry_index,
        sourcemeta::jsonschema::RdfResolutionError{
            error.message,
            std::string{sourcemeta::jsonschema::facet_name(error.facet)},
            std::move(error.instance_location),
            std::move(error.schema_location),
            std::move(error.conflicting_schema_location),
            std::move(error.inert_override_location), entry.resolution_base});
  }

  auto document{std::get<sourcemeta::core::JSON>(std::move(outcome))};

  if (context.has_value()) {
    try {
      document =
          flatten ? sourcemeta::core::jsonld_flatten(document, context.value())
                  : sourcemeta::core::jsonld_compact(document, context.value());
    } catch (const sourcemeta::core::JSONLDError &error) {
      throw_with_entry(
          entry_index,
          sourcemeta::core::FileError<sourcemeta::core::JSONLDError>(
              entry.resolution_base, error));
    }
  } else if (flatten) {
    try {
      document = sourcemeta::core::jsonld_flatten(document);
    } catch (const sourcemeta::core::JSONLDError &error) {
      throw_with_entry(
          entry_index,
          sourcemeta::core::FileError<sourcemeta::core::JSONLDError>(
              entry.resolution_base, error));
    }
  }

  sourcemeta::jsonschema::LOG_VERBOSE(options)
      << "ok: " << instance_display_path;
  if (multidocument) {
    sourcemeta::jsonschema::LOG_VERBOSE(options)
        << " (entry #" << entry.index + 1 << ")";
  }
  sourcemeta::jsonschema::LOG_VERBOSE(options)
      << "\n  matches "
      << sourcemeta::jsonschema::stdin_path_string(schema_resolution_base)
      << "\n";

  // A dataset promotes to one JSON-LD document per line, as JSON-LD has no
  // YAML serialisation to fall back on when the input was YAML
  if (multidocument) {
    sourcemeta::core::stringify(document, std::cout);
  } else {
    sourcemeta::core::prettify(document, std::cout);
  }

  std::cout << "\n";
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

  // An OpenAPI description is not a schema, so what it goes by wherever we
  // report on it is an identity of its own rather than the one a schema from
  // the same place would take
  const auto is_openapi{is_openapi_document(parsed_schema.document)};
  const auto display_path{
      schema_from_stdin ? (is_openapi ? openapi_stdin_path() : stdin_path())
                        : schema_resolution_base};

  reject_unsupported_openapi(parsed_schema.document, display_path);

  if (!is_openapi && !parsed_schema.document.is_object() &&
      !parsed_schema.document.is_boolean()) {
    throw NotSchemaError{display_path};
  }

  const auto &schema{parsed_schema.document};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};
  const auto fast_mode{options.contains("fast")};
  const auto schema_default_id{sourcemeta::jsonschema::default_id(
      schema_resolution_base, schema_from_stdin)};

  auto tweaks{
      format_assertion_tweaks(options).value_or(sourcemeta::blaze::Tweaks{})};
  tweaks.annotations = std::unordered_set<sourcemeta::core::JSON::StringView>(
      sourcemeta::blaze::JSONLD_KEYWORDS.begin(),
      sourcemeta::blaze::JSONLD_KEYWORDS.end());

  const auto schema_template{
      is_openapi
          ? with_openapi_entrypoint(
                parsed_schema.document, options, custom_resolver,
                sourcemeta::jsonschema::openapi_resolver(
                    options, options.contains("http"), dialect, configuration),
                sourcemeta::jsonschema::openapi_default_id(
                    schema_resolution_base, schema_from_stdin),
                display_path, parsed_schema.positions,
                [&](const sourcemeta::core::SchemaFrame &frame,
                    const std::string &entrypoint_uri) {
                  assert_annotations_support(frame, entrypoint_uri,
                                             display_path);
                  return compile_for_evaluation(
                      parsed_schema.document, custom_resolver, frame,
                      entrypoint_uri,
                      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                                : sourcemeta::blaze::Mode::Exhaustive,
                      tweaks, display_path, parsed_schema.positions);
                })
          : compile_plain_schema(schema, options, custom_resolver, dialect,
                                 schema_default_id, schema_resolution_base,
                                 parsed_schema.positions, fast_mode, tweaks)};

  const auto entries{read_instances(instance_path_view, instance_path,
                                    instance_from_stdin, options)};
  assert(!entries.empty());
  const auto multidocument{entries.front().multidocument};

  const auto json_output{options.contains("json")};
  const auto flatten{options.contains("flatten")};
  const auto compact{options.contains("compact") &&
                     !options.at("compact").empty()};

  std::optional<sourcemeta::core::JSON> context{std::nullopt};
  if (compact) {
    const std::filesystem::path context_path{options.at("compact").front()};
    auto parsed_context{read_file(context_path)};

    // Compacting an empty document exercises context processing on its own,
    // so context errors are attributed to the context file while errors on
    // the real runs below are attributed to the instance that produced the
    // offending document
    try {
      [[maybe_unused]] const auto probe{sourcemeta::core::jsonld_compact(
          sourcemeta::core::JSON::make_array(), parsed_context.document)};
    } catch (const sourcemeta::core::JSONLDError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::JSONLDError>(
          context_path, error);
    }

    context = std::move(parsed_context.document);
  }

  sourcemeta::blaze::Evaluator evaluator;

  for (const auto &entry : entries) {
    promote_entry(entry, evaluator, schema_template, context, flatten,
                  fast_mode, json_output, multidocument, schema_resolution_base,
                  options);
  }
}
