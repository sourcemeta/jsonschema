#include <sourcemeta/blaze/bundle.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

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
    throw sourcemeta::blaze::SchemaKeywordError{"$schema", value.str(),
                                                "The dialect value is invalid"};
  }

  return dialect->to_string();
}

} // namespace

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
    summary.validated += 1;
    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw NotSchemaError{entry.from_stdin ? stdin_path()
                                            : entry.resolution_base};
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
      const auto dialect{
          effective_dialect(entry.second, default_dialect_option)};
      if (dialect.empty()) {
        throw sourcemeta::core::FileError<
            sourcemeta::blaze::SchemaUnknownBaseDialectError>(
            entry.resolution_base);
      }

      const sourcemeta::blaze::SchemaFrame schema_frame{
          sourcemeta::blaze::SchemaFrame::Mode::Root, entry.second,
          sourcemeta::blaze::schema_walker, custom_resolver,
          default_dialect_option};
      const sourcemeta::core::JSON bundled{sourcemeta::blaze::bundle(
          schema_frame.metaschema(custom_resolver),
          sourcemeta::blaze::schema_walker, custom_resolver,
          sourcemeta::blaze::BundleMode::References, default_dialect_option)};
      const sourcemeta::blaze::SchemaFrame frame{
          sourcemeta::blaze::SchemaFrame::Mode::References, bundled,
          sourcemeta::blaze::schema_walker, custom_resolver,
          default_dialect_option};

      if (!cache.contains(std::string{dialect})) {
        const auto metaschema_template{sourcemeta::blaze::compile(
            bundled, sourcemeta::blaze::schema_walker, custom_resolver,
            sourcemeta::blaze::default_schema_compiler, frame, frame.root(),
            sourcemeta::blaze::Mode::Exhaustive,
            sourcemeta::jsonschema::format_assertion_tweaks(options))};
        cache.insert({std::string{dialect}, metaschema_template});
      }

      if (trace) {
        sourcemeta::blaze::TraceOutput output{
            cache.at(std::string{dialect}),
            trace_callback(entry.positions, std::cout)};
        if (!evaluator.validate(cache.at(std::string{dialect}), entry.second,
                                std::ref(output))) {
          summary.failed += 1;
        }
      } else if (json_output) {
        // Otherwise its impossible to correlate the output
        // when validating i.e. a directory of schemas
        std::cerr << relative_path_string(entry.resolution_base) << "\n";
        const auto output{sourcemeta::blaze::standard(
            evaluator, cache.at(std::string{dialect}), entry.second,
            sourcemeta::blaze::StandardOutput::Basic, entry.positions)};
        assert(output.is_object());
        assert(output.defines("valid"));
        assert(output.at("valid").is_boolean());
        if (!output.at("valid").to_boolean()) {
          summary.failed += 1;
        }

        sourcemeta::core::prettify(output, std::cout);
        std::cout << "\n";
      } else {
        sourcemeta::blaze::SimpleOutput output{entry.second};
        if (evaluator.validate(cache.at(std::string{dialect}), entry.second,
                               std::ref(output))) {
          LOG_VERBOSE(options)
              << "ok: " << relative_path_string(entry.resolution_base)
              << "\n  matches " << dialect << "\n";
        } else {
          std::cerr << "fail: " << relative_path_string(entry.resolution_base)
                    << "\n";
          print(output, entry.positions, std::cerr);
          summary.failed += 1;
        }
      }
    } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
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
    } catch (const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaResolutionError>(entry.resolution_base,
                                                    error);
    } catch (const sourcemeta::blaze::SchemaVocabularyError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaVocabularyError>(entry.resolution_base,
                                                    error.uri(), error.what());
    } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaUnknownBaseDialectError>(
          entry.resolution_base);
    } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaUnknownDialectError>(entry.resolution_base);
    } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::blaze::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>(entry.resolution_base,
                                                         error);
    }

    if (summary.failed > failures_before && !continue_on_error) {
      summary.stopped = std::next(iterator) != entries.cend();
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
