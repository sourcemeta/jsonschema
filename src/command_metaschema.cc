#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <cassert>     // assert
#include <iostream>    // std::cout, std::cerr
#include <iterator>    // std::next
#include <map>         // std::map
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "print.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto effective_dialect(const sourcemeta::core::JSON &schema,
                       const std::string_view default_dialect)
    -> std::string_view {
  if (!schema.is_object()) {
    return default_dialect;
  }

  const auto *dialect{schema.try_at("$schema")};
  if (dialect == nullptr) {
    return default_dialect;
  }

  if (!dialect->is_string()) {
    std::ostringstream value;
    sourcemeta::core::stringify(*dialect, value);
    throw sourcemeta::core::SchemaKeywordError{"$schema", value.str(),
                                               "The dialect value is invalid"};
  }

  return dialect->to_string();
}

} // namespace

// Resolving, bundling and compiling a meta-schema is the expensive part of
// this, and one dialect usually governs everything being checked, so it
// happens once per dialect rather than once per schema
auto metaschema_template(
    std::map<std::string, sourcemeta::blaze::Template> &cache,
    const sourcemeta::core::JSON &schema, const std::string &dialect,
    const std::string_view framing_dialect,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::Options &options)
    -> const sourcemeta::blaze::Template & {
  const auto match{cache.find(dialect)};
  if (match != cache.cend()) {
    return match->second;
  }

  const sourcemeta::core::SchemaFrame schema_frame{
      sourcemeta::core::SchemaFrame::Mode::Root, schema,
      sourcemeta::core::schema_walker, resolver, framing_dialect};
  const sourcemeta::core::JSON bundled{sourcemeta::core::schema_bundle(
      schema_frame.metaschema(resolver), sourcemeta::core::schema_walker,
      resolver, framing_dialect, "",
      sourcemeta::jsonschema::bundle_references_options())};
  const sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References, bundled,
      sourcemeta::core::schema_walker, resolver, framing_dialect};

  return cache
      .insert({dialect,
               sourcemeta::blaze::compile(
                   bundled, sourcemeta::core::schema_walker, resolver,
                   sourcemeta::blaze::default_schema_compiler, frame,
                   frame.root(), sourcemeta::blaze::Mode::Exhaustive,
                   sourcemeta::jsonschema::format_assertion_tweaks(options))})
      .first->second;
}

// Checking one schema against its dialect and saying how it went, which is the
// same work whether the schema stands on its own or sits inside a description
// Blaze resolves instance locations against the tracker itself, and what it
// reports is relative to the schema that was validated rather than to the file
// it came out of, so for a Schema Object of a description the positions have to
// be put in afterwards against the place that schema sits
//
// TODO: Doing this here is wrong. It repeats a lookup Blaze already did, and
// the shape it writes back is Blaze's to define, so the two drift apart the
// moment that format changes. Validating a subtree is the general case and
// Blaze should take the base pointer and resolve against it, after which this
// whole function goes away. See `blaze-standard-subtree-positions`
auto rebase_positions(sourcemeta::core::JSON &output,
                      const sourcemeta::core::PointerPositionTracker &positions,
                      const sourcemeta::core::Pointer &base) -> void {
  for (const auto &property : {"errors", "annotations"}) {
    if (!output.defines(property)) {
      continue;
    }

    for (auto &unit : output.at(property).as_array()) {
      assert(unit.defines("instanceLocation"));
      const auto position{
          positions.get(base.concat(sourcemeta::core::to_pointer(
              unit.at("instanceLocation").to_string())))};
      if (position.has_value()) {
        unit.assign("instancePosition",
                    sourcemeta::core::to_json(position.value()));
      }
    }
  }
}

auto check_against_metaschema(
    sourcemeta::blaze::Evaluator &evaluator,
    const sourcemeta::blaze::Template &schema_template,
    const sourcemeta::core::JSON &schema, const std::string &subject,
    const std::string &dialect,
    const sourcemeta::core::PointerPositionTracker &positions,
    const sourcemeta::core::Options &options, const bool trace,
    const bool json_output, const sourcemeta::core::Pointer &base = {})
    -> bool {
  if (trace) {
    sourcemeta::blaze::TraceOutput output{
        schema_template,
        sourcemeta::jsonschema::trace_callback(positions, std::cout)};
    return evaluator.validate(schema_template, schema, std::ref(output));
  }

  if (json_output) {
    // Otherwise its impossible to correlate the output
    // when validating i.e. a directory of schemas
    std::cerr << subject << "\n";
    auto output{sourcemeta::blaze::standard(
        evaluator, schema_template, schema,
        sourcemeta::blaze::StandardOutput::Basic, positions)};
    if (!base.empty()) {
      rebase_positions(output, positions, base);
    }
    assert(output.is_object());
    assert(output.defines("valid"));
    assert(output.at("valid").is_boolean());
    sourcemeta::core::prettify(output, std::cout);
    std::cout << "\n";
    return output.at("valid").to_boolean();
  }

  sourcemeta::blaze::SimpleOutput output{schema};
  if (evaluator.validate(schema_template, schema, std::ref(output))) {
    sourcemeta::jsonschema::LOG_VERBOSE(options)
        << sourcemeta::jsonschema::format_validation_status(
               sourcemeta::jsonschema::ValidationStatus::Pass)
        << " " << subject << "\n  matches "
        << sourcemeta::jsonschema::paint(
               dialect, sourcemeta::core::TerminalStyle::Cyan,
               sourcemeta::core::TerminalStream::Stderr)
        << "\n";
    return true;
  }

  // Which meta-schema turned it down is not something the failure itself
  // says, and one description may hold schemas in several dialects, so there
  // is nothing else to read it off
  std::cerr << sourcemeta::jsonschema::format_validation_status(
                   sourcemeta::jsonschema::ValidationStatus::Fail)
            << " " << subject << "\n  against "
            << sourcemeta::jsonschema::paint(
                   dialect, sourcemeta::core::TerminalStyle::Cyan,
                   sourcemeta::core::TerminalStream::Stderr)
            << "\n";
  sourcemeta::jsonschema::print(output, positions, std::cerr, "error:", base);
  return false;
}

// Every Schema Object a description holds, checked against whatever dialect
// it is written in. Which positions are Schema Objects is the description's
// frame to say, while what dialect each is written in is the frame of the
// schemas it holds, as a Schema Object declaring its own `$schema` overrides
// whatever the description had in force
auto check_openapi_description(
    sourcemeta::blaze::Evaluator &evaluator,
    std::map<std::string, sourcemeta::blaze::Template> &cache,
    const sourcemeta::jsonschema::InputJSON &entry,
    const std::filesystem::path &display_path,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::core::Options &options,
    sourcemeta::jsonschema::ValidationSummary &summary, const bool trace,
    const bool json_output, const bool continue_on_error) -> void {
  const auto frame{sourcemeta::jsonschema::openapi_frame_for_evaluation(
      entry.second, resolver, sourcemeta::jsonschema::openapi_default_id(entry),
      display_path, entry.positions)};

  std::vector<std::reference_wrapper<
      const std::pair<const sourcemeta::core::JSON::String,
                      sourcemeta::core::OpenAPIFrame::Location>>>
      schemas;
  for (const auto &location : frame.locations()) {
    if (location.second.type ==
        sourcemeta::core::OpenAPIFrame::ObjectKind::Schema) {
      schemas.emplace_back(location);
    }
  }

  if (schemas.empty()) {
    sourcemeta::jsonschema::LOG_WARNING()
        << "No schema objects were found in "
        << sourcemeta::jsonschema::relative_path_string(display_path) << "\n";
    return;
  }

  for (auto iterator{schemas.cbegin()}; iterator != schemas.cend();
       ++iterator) {
    const auto &location{*iterator};
    const auto &uri{location.get().first};
    const auto &pointer{location.get().second.pointer};
    const auto effective{frame.schemas().traverse(uri)};
    assert(effective.has_value());
    const std::string dialect{effective.value().get().dialect};
    const auto &schema{sourcemeta::core::get(entry.second, pointer)};

    summary.validated += 1;
    const auto &schema_template{metaschema_template(
        cache, schema, dialect, dialect, resolver, options)};

    std::ostringstream subject;
    subject << sourcemeta::jsonschema::relative_path_string(display_path)
            << "#";
    sourcemeta::core::stringify(pointer, subject);
    if (!check_against_metaschema(evaluator, schema_template, schema,
                                  subject.str(), dialect, entry.positions,
                                  options, trace, json_output, pointer)) {
      summary.failed += 1;

      // One description holds many schemas, so stopping at the first failure
      // has to mean the first of those rather than the first file
      if (!continue_on_error) {
        summary.stopped = std::next(iterator) != schemas.cend();
        return;
      }
    }
  }
}

auto sourcemeta::jsonschema::metaschema(
    const sourcemeta::core::Options &options) -> void {
  validate_http_headers(options);
  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};
  const auto continue_on_error{options.contains("continue")};

  ValidationSummary summary;
  sourcemeta::blaze::Evaluator evaluator;

  std::map<std::string, sourcemeta::blaze::Template> cache;

  const auto entries{for_each_json(options, InputRequirement::NonEmpty)};

  // Trace output carries no per schema header, so more than one schema would
  // produce an unattributable stream of interleaved evaluation steps
  if (trace && entries.size() > 1) {
    throw OptionConflictError{
        "The `--trace/-t` option is only allowed given a single schema"};
  }

  for (auto iterator{entries.cbegin()}; iterator != entries.cend();
       ++iterator) {
    const auto &entry{*iterator};
    const auto failures_before{summary.failed};
    // An OpenAPI description is not a schema, so what it goes by wherever we
    // report on it is an identity of its own rather than the one a schema from
    // the same place would take
    const auto is_openapi{is_openapi_document(entry.second)};
    const auto display_path{
        entry.from_stdin ? (is_openapi ? openapi_stdin_path() : stdin_path())
                         : entry.resolution_base};

    reject_unsupported_openapi(entry.second, display_path);

    // A description is a collection of schemas rather than one, so whatever a
    // trace of it showed would have to be read against a schema the output
    // never names. Refused however many it holds, as a rule that depends on
    // the count is a rule nobody can predict
    if (is_openapi && trace) {
      throw OptionConflictError{
          "The `--trace/-t` option is not available when the input is an "
          "OpenAPI description"};
    }

    if (!is_openapi && !entry.second.is_object() &&
        !entry.second.is_boolean()) {
      throw NotSchemaError{display_path};
    }

    if (!is_openapi) {
      summary.validated += 1;
    }

    const auto configuration_path{
        find_configuration(options, entry.resolution_base)};
    const auto &configuration{
        read_configuration(options, configuration_path, entry.resolution_base)};
    const auto default_dialect_option{default_dialect(options, configuration)};

    const auto &custom_resolver{resolver(options, options.contains("http"),
                                         default_dialect_option,
                                         configuration)};

    try {
      // A description holds many schemas rather than being one, so each of
      // them is checked against whatever dialect it is written in
      if (is_openapi) {
        check_openapi_description(evaluator, cache, entry, display_path,
                                  custom_resolver, options, summary, trace,
                                  json_output, continue_on_error);
      } else {
        const auto dialect{
            effective_dialect(entry.second, default_dialect_option)};
        if (dialect.empty()) {
          throw sourcemeta::core::FileError<
              sourcemeta::core::SchemaUnknownBaseDialectError>(display_path);
        }

        const auto &schema_template{metaschema_template(
            cache, entry.second, std::string{dialect}, default_dialect_option,
            custom_resolver, options)};

        if (!check_against_metaschema(
                evaluator, schema_template, entry.second,
                relative_path_string(entry.resolution_base),
                std::string{dialect}, entry.positions, options, trace,
                json_output)) {
          summary.failed += 1;
        }
      }
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::CompilerInvalidRegexError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidRegexError>(entry.resolution_base,
                                                        error);
    } catch (const sourcemeta::blaze::CompilerError &error) {
      // No position, as what compiles here is the meta-schema while the
      // positions on hand describe the schema being validated against it
      throw sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>(
          entry.resolution_base, error);
    } catch (
        const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaResolutionError>(entry.resolution_base,
                                                   error);
    } catch (const sourcemeta::core::SchemaVocabularyError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaVocabularyError>(entry.resolution_base,
                                                   error.uri(), error.what());
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownBaseDialectError>(
          entry.resolution_base);
    } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownDialectError>(entry.resolution_base);
    } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::core::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>(entry.resolution_base,
                                                        error);
    }

    if (summary.failed > failures_before && !continue_on_error) {
      // A description may already have stopped partway through the schemas it
      // holds, which is just as much having stopped as leaving files unread
      summary.stopped =
          summary.stopped || std::next(iterator) != entries.cend();
      break;
    }
  }

  if (!json_output && !trace) {
    print_summary(summary, options, std::cerr);
  }

  if (summary.stopped) {
    LOG_WARNING()
        << "Stopped at first failure, pass --continue/-c to keep going\n";
  }

  if (summary.failed > 0) {
    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
