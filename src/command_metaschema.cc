#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <map>      // std::map
#include <set>      // std::set
#include <string>   // std::string

#include "command.h"
#include "utils.h"

// TODO: Add a flag to emit output using the standard JSON Schema output format
auto sourcemeta::jsonschema::cli::metaschema(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http", "t", "trace"})};
  const auto trace{options.contains("t") || options.contains("trace")};
  const auto default_dialect_option{default_dialect(options)};
  const auto custom_resolver{
      resolver(options, options.contains("h") || options.contains("http"),
               default_dialect_option)};
  bool result{true};
  sourcemeta::blaze::Evaluator evaluator;

  std::map<std::string, sourcemeta::blaze::Template> cache;

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    if (!sourcemeta::core::is_schema(entry.second)) {
      std::cerr << "error: The schema file you provided does not represent a "
                   "valid JSON Schema\n  "
                << sourcemeta::jsonschema::cli::safe_weakly_canonical(
                       entry.first)
                       .string()
                << "\n";
      return EXIT_FAILURE;
    }

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
      print(output, std::cout);
    } else {
      sourcemeta::blaze::SimpleOutput output{entry.second};
      if (evaluator.validate(cache.at(dialect.value()), entry.second,
                             std::ref(output))) {
        log_verbose(options)
            << "ok: " << safe_weakly_canonical(entry.first).string()
            << "\n  matches " << dialect.value() << "\n";
      } else {
        std::cerr << "fail: " << safe_weakly_canonical(entry.first).string()
                  << "\n";
        print(output, std::cerr);
        result = false;
      }
    }
  }

  return result ? EXIT_SUCCESS
                // Report a different exit code for validation failures, to
                // distinguish them from other errors
                : 2;
}
