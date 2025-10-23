#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <cassert>  // assert
#include <iostream> // std::cerr
#include <map>      // std::map
#include <string>   // std::string

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::metaschema(
    const sourcemeta::core::Options &options) -> void {
  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};

  bool result{true};
  sourcemeta::blaze::Evaluator evaluator;

  std::map<std::string, sourcemeta::blaze::Template> cache;

  for (const auto &entry : for_each_json(options)) {
    if (!sourcemeta::core::is_schema(entry.second)) {
      throw NotSchemaError{entry.first};
    }

    const auto configuration_path{find_configuration(entry.first)};
    const auto &configuration{read_configuration(options, configuration_path)};
    const auto default_dialect_option{default_dialect(options, configuration)};

    const auto &custom_resolver{resolver(options, options.contains("http"),
                                         default_dialect_option,
                                         configuration)};

    try {
      const auto dialect{
          sourcemeta::core::dialect(entry.second, default_dialect_option)};
      if (!dialect) {
        throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
            entry.first);
      }

      const auto metaschema{sourcemeta::core::metaschema(
          entry.second, custom_resolver, default_dialect_option)};
      const sourcemeta::core::JSON bundled{sourcemeta::core::bundle(
          metaschema, sourcemeta::core::schema_official_walker, custom_resolver,
          default_dialect_option)};
      sourcemeta::core::SchemaFrame frame{
          sourcemeta::core::SchemaFrame::Mode::References};
      frame.analyse(bundled, sourcemeta::core::schema_official_walker,
                    custom_resolver, default_dialect_option);

      if (!cache.contains(dialect.value())) {
        const auto metaschema_template{sourcemeta::blaze::compile(
            bundled, sourcemeta::core::schema_official_walker, custom_resolver,
            sourcemeta::blaze::default_schema_compiler, frame,
            sourcemeta::blaze::Mode::Exhaustive, default_dialect_option)};
        cache.insert({dialect.value(), metaschema_template});
      }

      if (trace) {
        sourcemeta::blaze::TraceOutput output{
            sourcemeta::core::schema_official_walker, custom_resolver,
            sourcemeta::core::empty_weak_pointer, frame};
        result = evaluator.validate(cache.at(dialect.value()), entry.second,
                                    std::ref(output));
        print(output, entry.positions, std::cout);
      } else if (json_output) {
        // Otherwise its impossible to correlate the output
        // when validating i.e. a directory of schemas
        std::cerr << entry.first.string() << "\n";
        const auto output{sourcemeta::blaze::standard(
            evaluator, cache.at(dialect.value()), entry.second,
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
        if (evaluator.validate(cache.at(dialect.value()), entry.second,
                               std::ref(output))) {
          LOG_VERBOSE(options)
              << "ok: "
              << sourcemeta::core::weakly_canonical(entry.first).string()
              << "\n  matches " << dialect.value() << "\n";
        } else {
          std::cerr << "fail: "
                    << sourcemeta::core::weakly_canonical(entry.first).string()
                    << "\n";
          print(output, entry.positions, std::cerr);
          result = false;
        }
      }
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          entry.first, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw FileError<sourcemeta::core::SchemaResolutionError>(entry.first,
                                                               error);
    }
  }

  if (!result) {
    // Report a different exit code for validation failures, to
    // distinguish them from other errors
    throw Fail{2};
  }
}
