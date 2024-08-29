#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

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
  const auto options{parse_options(arguments, {"h", "http"})};
  const auto custom_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};
  bool result{true};

  std::map<std::string, sourcemeta::jsontoolkit::SchemaCompilerTemplate> cache;

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    if (!sourcemeta::jsontoolkit::is_schema(entry.second)) {
      std::cerr << "error: The schema file you provided does not represent a "
                   "valid JSON Schema\n  "
                << std::filesystem::canonical(entry.first).string() << "\n";
      return EXIT_FAILURE;
    }

    const auto dialect{sourcemeta::jsontoolkit::dialect(entry.second)};
    assert(dialect.has_value());

    const auto metaschema{
        sourcemeta::jsontoolkit::metaschema(entry.second, custom_resolver)};
    if (!cache.contains(dialect.value())) {
      const auto metaschema_template{sourcemeta::jsontoolkit::compile(
          metaschema, sourcemeta::jsontoolkit::default_schema_walker,
          custom_resolver, sourcemeta::jsontoolkit::default_schema_compiler,
          sourcemeta::jsontoolkit::SchemaCompilerCompilationMode::Optimized)};
      cache.insert({dialect.value(), metaschema_template});
    }

    sourcemeta::jsontoolkit::SchemaCompilerErrorTraceOutput output{metaschema};
    if (sourcemeta::jsontoolkit::evaluate(
            cache.at(dialect.value()), entry.second,
            sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
            std::ref(output))) {
      log_verbose(options)
          << "ok: " << std::filesystem::weakly_canonical(entry.first).string()
          << "\n  matches " << dialect.value() << "\n";
    } else {
      std::cerr << "fail: "
                << std::filesystem::weakly_canonical(entry.first).string()
                << "\n";
      print(output, std::cerr);
      result = false;
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
