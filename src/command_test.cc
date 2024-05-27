#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr, std::cout

#include "command.h"
#include "utils.h"

#define EXPECT_JSON(condition, message)                                        \
  if (!condition) {                                                            \
    std::cerr << message << "\n";                                              \
    return EXIT_FAILURE;                                                       \
  }

auto intelligence::jsonschema::cli::test(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};
  bool result{true};
  const auto test_resolver{resolver(options)};

  for (const auto &entry : for_each_json(options.at(""))) {
    const sourcemeta::jsontoolkit::JSON test{
        sourcemeta::jsontoolkit::from_file(entry.first)};
    EXPECT_JSON(test.is_object(), "The test document must be an object")

    EXPECT_JSON(test.defines("description"),
                "The test document must contain a `description` property")
    EXPECT_JSON(test.defines("schema"),
                "The test document must contain a `schema` property")
    EXPECT_JSON(test.defines("tests"),
                "The test document must contain a `tests` property")

    EXPECT_JSON(test.at("description").is_string(),
                "The test document `description` property must be a string")
    EXPECT_JSON(test.at("schema").is_string(),
                "The test document `schema` property must be a URI")
    EXPECT_JSON(test.at("tests").is_array(),
                "The test document `tests` property must be an array")

    std::cout << entry.first.string() << "\n";

    const auto schema{test_resolver(test.at("schema").to_string()).get()};
    if (!schema.has_value()) {
      std::cerr << "Could not resolve schema " << test.at("schema").to_string()
                << " at " << entry.first.string() << "\n";
      return EXIT_FAILURE;
    }

    const auto schema_template{sourcemeta::jsontoolkit::compile(
        schema.value(), sourcemeta::jsontoolkit::default_schema_walker,
        test_resolver, sourcemeta::jsontoolkit::default_schema_compiler)};

    for (const auto &test_case : test.at("tests").as_array()) {
      EXPECT_JSON(test_case.is_object(), "Test case documents must be objects")

      EXPECT_JSON(test_case.defines("description"),
                  "Test case documents must contain a `description` property")
      EXPECT_JSON(test_case.defines("data"),
                  "Test case documents must contain a `data` property")
      EXPECT_JSON(test_case.defines("valid"),
                  "Test case documents must contain a `valid` property")

      EXPECT_JSON(
          test_case.at("description").is_string(),
          "The test case document `description` property must be a string")
      EXPECT_JSON(test_case.at("valid").is_boolean(),
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
