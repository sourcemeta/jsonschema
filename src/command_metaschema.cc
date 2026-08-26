#include <sourcemeta/blaze/bundle.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <cassert>     // assert
#include <iostream>    // std::cout, std::cerr
#include <map>         // std::map
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

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
  if (!dialect) {
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

auto resolve_metaschema(const sourcemeta::core::JSON &schema,
                        const std::string_view dialect,
                        const sourcemeta::blaze::SchemaResolver &resolver)
    -> sourcemeta::core::JSON {
  // A meta-schema that is embedded in the schema itself takes precedence
  // over what the resolver knows about, as the schema pins the exact
  // meta-schema it is described by
  const auto *embedded{
      sourcemeta::blaze::metaschema_try_embedded(schema, dialect, resolver)};
  if (embedded) {
    return *embedded;
  }

  auto result{resolver(dialect)};
  if (result.has_value()) {
    return std::move(result.value());
  }

  if (sourcemeta::core::URI{dialect}.is_relative()) {
    throw sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError{dialect};
  }

  throw sourcemeta::blaze::SchemaResolutionError{
      dialect, "Could not resolve the metaschema of the schema"};
}

} // namespace

auto sourcemeta::jsonschema::metaschema(
    const sourcemeta::core::Options &options) -> void {
  validate_http_headers(options);
  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};

  bool result{true};
  sourcemeta::blaze::Evaluator evaluator;

  std::map<std::string, sourcemeta::blaze::Template> cache;

  for (const auto &entry : for_each_json(options)) {
    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw NotSchemaError{entry.from_stdin ? stdin_path()
                                            : entry.resolution_base};
    }

    const auto configuration_path{find_configuration(entry.resolution_base)};
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

      const auto metaschema{
          resolve_metaschema(entry.second, dialect, custom_resolver)};
      const sourcemeta::core::JSON bundled{sourcemeta::blaze::bundle(
          metaschema, sourcemeta::blaze::schema_walker, custom_resolver,
          sourcemeta::blaze::BundleMode::References, default_dialect_option)};
      sourcemeta::blaze::SchemaFrame frame{
          sourcemeta::blaze::SchemaFrame::Mode::References};
      frame.analyse(bundled, sourcemeta::blaze::schema_walker, custom_resolver,
                    default_dialect_option);

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
            sourcemeta::blaze::schema_walker, custom_resolver,
            trace_callback(entry.positions, std::cout),
            sourcemeta::core::EMPTY_WEAK_POINTER, frame};
        result = evaluator.validate(cache.at(std::string{dialect}),
                                    entry.second, std::ref(output));
      } else if (json_output) {
        // Otherwise its impossible to correlate the output
        // when validating i.e. a directory of schemas
        std::cerr << entry.first << "\n";
        const auto output{sourcemeta::blaze::standard(
            evaluator, cache.at(std::string{dialect}), entry.second,
            sourcemeta::blaze::StandardOutput::Basic, entry.positions)};
        assert(output.is_object());
        assert(output.defines("valid"));
        assert(output.at("valid").is_boolean());
        if (!output.at("valid").to_boolean()) {
          result = false;
        }

        sourcemeta::core::prettify(output, std::cout);
        std::cout << "\n";
      } else {
        sourcemeta::blaze::SimpleOutput output{entry.second};
        if (evaluator.validate(cache.at(std::string{dialect}), entry.second,
                               std::ref(output))) {
          LOG_VERBOSE(options)
              << "ok: " << entry.first << "\n  matches " << dialect << "\n";
        } else {
          std::cerr << "fail: " << entry.first << "\n";
          print(output, entry.positions, std::cerr);
          result = false;
        }
      }
    } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::CompilerInvalidRegexError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidRegexError>(entry.resolution_base,
                                                        error);
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
  }

  if (!result) {
    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
