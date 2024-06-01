#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr, std::cout

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::test(
    const std::span<const std::string> &arguments) -> int {
  const auto options{
      parse_options(arguments, {"h", "http", "m", "metaschema"})};
  bool result{true};
  const auto test_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};

  for (const auto &entry : for_each_json(options.at(""))) {
    const sourcemeta::jsontoolkit::JSON test{
        sourcemeta::jsontoolkit::from_file(entry.first)};
    CLI_ENSURE(test.is_object(), "The test document must be an object")
    CLI_ENSURE(test.defines("description"),
               "The test document must contain a `description` property")
    CLI_ENSURE(test.defines("schema"),
               "The test document must contain a `schema` property")
    CLI_ENSURE(test.defines("tests"),
               "The test document must contain a `tests` property")
    CLI_ENSURE(test.at("description").is_string(),
               "The test document `description` property must be a string")
    CLI_ENSURE(test.at("schema").is_string(),
               "The test document `schema` property must be a URI")
    CLI_ENSURE(test.at("tests").is_array(),
               "The test document `tests` property must be an array")

    std::cout << entry.first.string() << "\n";

    const auto schema{test_resolver(test.at("schema").to_string()).get()};
    if (!schema.has_value()) {
      std::cerr << "Could not resolve schema " << test.at("schema").to_string()
                << " at " << entry.first.string() << "\n";
      return EXIT_FAILURE;
    }

    if (options.contains("m") || options.contains("metaschema")) {
      const auto metaschema_template{sourcemeta::jsontoolkit::compile(
          sourcemeta::jsontoolkit::metaschema(schema.value(), test_resolver),
          sourcemeta::jsontoolkit::default_schema_walker, test_resolver,
          sourcemeta::jsontoolkit::default_schema_compiler)};
      if (sourcemeta::jsontoolkit::evaluate(
              metaschema_template, schema.value(),
              sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
              pretty_evaluate_callback)) {
        log_verbose(options)
            << "The schema is valid with respect to its metaschema\n";
      } else {
        std::cerr << "The schema is NOT valid with respect to its metaschema\n";
        return EXIT_FAILURE;
      }
    }

    const auto schema_template{sourcemeta::jsontoolkit::compile(
        schema.value(), sourcemeta::jsontoolkit::default_schema_walker,
        test_resolver, sourcemeta::jsontoolkit::default_schema_compiler)};

    for (const auto &test_case : test.at("tests").as_array()) {
      CLI_ENSURE(test_case.is_object(), "Test case documents must be objects")
      CLI_ENSURE(test_case.defines("description"),
                 "Test case documents must contain a `description` property")
      CLI_ENSURE(test_case.defines("data"),
                 "Test case documents must contain a `data` property")
      CLI_ENSURE(test_case.defines("valid"),
                 "Test case documents must contain a `valid` property")
      CLI_ENSURE(
          test_case.at("description").is_string(),
          "The test case document `description` property must be a string")
      CLI_ENSURE(test_case.at("valid").is_boolean(),
                 "The test case document `tests` property must be a boolean")

      std::cout << "    " << test.at("description").to_string() << " - "
                << test_case.at("description").to_string() << "\n";

      const auto case_result{sourcemeta::jsontoolkit::evaluate(
          schema_template, test_case.at("data"))};
      if (test_case.at("valid").to_boolean() == case_result) {
        std::cout << "        PASS\n";
      } else {
        std::cout << "        FAIL\n";
        result = false;
      }
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
