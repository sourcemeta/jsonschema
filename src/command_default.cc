#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr

#include "command.h"
#include "utils.h"

auto sourcemeta::jsonschema::cli::defaults(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http"})};
  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema and a path to an\n"
        << "instance to apply defaults against the schema. For example:\n\n"
        << "  jsonschema default path/to/schema.json path/to/instance.json\n";
    return EXIT_FAILURE;
  }

  if (options.at("").size() < 2) {
    std::cerr
        << "error: In addition to the schema, you must also pass an argument\n"
        << "that represents the instance to add defaults to. For example:\n\n"
        << "  jsonschema default path/to/schema.json path/to/instance.json\n";
    return EXIT_FAILURE;
  }

  const auto &schema_path{options.at("").at(0)};
  const auto schema{sourcemeta::jsonschema::cli::read_file(schema_path)};
  const auto &instance_path{options.at("").at(1)};
  auto instance{sourcemeta::jsonschema::cli::read_file(instance_path)};
  if (!sourcemeta::core::is_schema(schema)) {
    std::cerr << "error: The schema file you provided does not represent a "
                 "valid JSON Schema\n  "
              << sourcemeta::jsonschema::cli::safe_weakly_canonical(schema_path)
                     .string()
              << "\n";
    return EXIT_FAILURE;
  }

  const auto dialect{default_dialect(options)};
  const auto custom_resolver{resolver(
      options, options.contains("h") || options.contains("http"), dialect)};

  const sourcemeta::core::JSON bundled_schema{
      sourcemeta::core::bundle(schema, sourcemeta::core::schema_official_walker,
                               custom_resolver, dialect)};
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(bundled_schema, sourcemeta::core::schema_official_walker,
                custom_resolver, dialect);

  const auto schema_template{sourcemeta::blaze::compile(
      bundled_schema, sourcemeta::core::schema_official_walker, custom_resolver,
      sourcemeta::blaze::default_schema_compiler, frame,
      sourcemeta::blaze::Mode::Exhaustive, dialect)};

  sourcemeta::blaze::Evaluator evaluator;
  sourcemeta::blaze::SimpleOutput output{instance};

  if (!evaluator.validate(schema_template, instance, std::ref(output))) {
    std::cerr << "fail: " << safe_weakly_canonical(instance_path).string()
              << "\n";
    std::cerr << "The instance is not valid against the schema";
    print(output, std::cerr);
    return EXIT_FAILURE;
  }

  log_verbose(options) << "ok: "
                       << safe_weakly_canonical(instance_path).string()
                       << "\n  matches "
                       << safe_weakly_canonical(schema_path).string() << "\n";

  for (const auto &annotation : output.annotations()) {
    assert(!annotation.first.evaluate_path.empty());
    const auto &keyword{annotation.first.evaluate_path.back()};
    if (!keyword.is_property() || keyword.to_property() != "default") {
      continue;
    }

    const auto frame_entry{frame.traverse(annotation.first.schema_location)};
    if (!frame_entry.has_value()) {
      continue;
    }

    const auto vocabularies{
        frame.vocabularies(frame_entry.value().get(), custom_resolver)};

    // TODO: Support everything down to Draft 4
    if (vocabularies.contains(
            "https://json-schema.org/draft/2020-12/vocab/meta-data")) {
      if (sourcemeta::core::try_get(instance,
                                    annotation.first.instance_location)) {
        continue;
      }

      sourcemeta::core::set(
          instance,
          // TODO: Support WeakPointer directly
          sourcemeta::core::to_pointer(annotation.first.instance_location),
          annotation.second.back());
    }
  }

  sourcemeta::core::prettify(instance, std::cout);
  std::cout << "\n";
  return EXIT_SUCCESS;
}
