#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonl.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <set>      // std::set
#include <string>   // std::string

#include "command.h"
#include "utils.h"

// TODO: Add a flag to emit output using the standard JSON Schema output format
// TODO: Add a flag to collect annotations
// TODO: Add a flag to take a pre-compiled schema as input
auto intelligence::jsonschema::cli::validate(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http"})};

  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema and a path to an\n"
        << "instance to validate against the schema. For example:\n\n"
        << "  jsonschema validate path/to/schema.json path/to/instance.json\n";
    return EXIT_FAILURE;
  }

  if (options.at("").size() < 2) {
    std::cerr
        << "error: In addition to the schema, you must also pass an argument\n"
        << "that represents the instance to validate against. For example:\n\n"
        << "  jsonschema validate path/to/schema.json path/to/instance.json\n";
    return EXIT_FAILURE;
  }

  const auto &schema_path{options.at("").at(0)};
  const auto custom_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};

  const auto schema{sourcemeta::jsontoolkit::from_file(schema_path)};

  if (!sourcemeta::jsontoolkit::is_schema(schema)) {
    std::cerr << "error: The schema file you provided does not represent a "
                 "valid JSON Schema\n  "
              << std::filesystem::canonical(schema_path).string() << "\n";
    return EXIT_FAILURE;
  }

  bool result{true};
  const std::filesystem::path instance_path{options.at("").at(1)};
  const auto schema_template{sourcemeta::jsontoolkit::compile(
      schema, sourcemeta::jsontoolkit::default_schema_walker, custom_resolver,
      sourcemeta::jsontoolkit::default_schema_compiler)};

  if (instance_path.extension() == ".jsonl") {
    log_verbose(options) << "Interpreting input as JSONL\n";
    std::size_t index{0};

    auto stream{sourcemeta::jsontoolkit::read_file(instance_path)};
    try {
      for (const auto &instance : sourcemeta::jsontoolkit::JSONL{stream}) {
        std::ostringstream error;
        const auto subresult = sourcemeta::jsontoolkit::evaluate(
            schema_template, instance,
            sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
            pretty_evaluate_callback(error,
                                     sourcemeta::jsontoolkit::empty_pointer));

        if (subresult) {
          log_verbose(options)
              << "ok: "
              << std::filesystem::weakly_canonical(instance_path).string()
              << " (entry #" << index << ")"
              << "\n  matches "
              << std::filesystem::weakly_canonical(schema_path).string()
              << "\n";
        } else {
          std::cerr << "fail: "
                    << std::filesystem::weakly_canonical(instance_path).string()
                    << " (entry #" << index << ")\n\n";
          sourcemeta::jsontoolkit::prettify(instance, std::cerr);
          std::cerr << "\n\n";
          std::cerr << error.str();
          result = false;
          break;
        }

        index += 1;
      }
    } catch (const sourcemeta::jsontoolkit::ParseError &error) {
      // For producing better error messages
      throw sourcemeta::jsontoolkit::FileParseError(instance_path, error);
    }
  } else {
    const auto instance{sourcemeta::jsontoolkit::from_file(instance_path)};

    std::ostringstream error;
    result = sourcemeta::jsontoolkit::evaluate(
        schema_template, instance,
        sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
        pretty_evaluate_callback(error,
                                 sourcemeta::jsontoolkit::empty_pointer));

    if (result) {
      log_verbose(options)
          << "ok: " << std::filesystem::weakly_canonical(instance_path).string()
          << "\n  matches "
          << std::filesystem::weakly_canonical(schema_path).string() << "\n";
    } else {
      std::cerr << error.str();
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
