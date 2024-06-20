#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr, std::cout

#include "command.h"
#include "utils.h"

auto intelligence::jsonschema::cli::test(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http"})};
  bool result{true};
  const auto test_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};
  const auto verbose{options.contains("verbose") || options.contains("v")};

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    const sourcemeta::jsontoolkit::JSON test{
        sourcemeta::jsontoolkit::from_file(entry.first)};
    std::cout << entry.first.string() << ":";

    if (!test.is_object()) {
      std::cout << "\nerror: The test document must be an object\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.defines("$schema")) {
      std::cout
          << "\nerror: The test document must contain a `$schema` property\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.at("$schema").is_string()) {
      std::cout
          << "\nerror: The test document `$schema` property must be a URI\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.defines("tests")) {
      std::cout
          << "\nerror: The test document must contain a `tests` property\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.at("tests").is_array()) {
      std::cout
          << "\nerror: The test document `tests` property must be an array\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    const auto schema{test_resolver(test.at("$schema").to_string()).get()};
    if (!schema.has_value()) {
      if (verbose) {
        std::cout << "\n";
      }

      throw sourcemeta::jsontoolkit::SchemaResolutionError(
          test.at("$schema").to_string(),
          "Could not resolve schema under test");
    }

    unsigned int pass_count{0};
    unsigned int index{0};
    const auto total{test.at("tests").size()};

    if (test.at("tests").empty()) {
      std::cout << " NO TESTS\n";
      continue;
    }

    sourcemeta::jsontoolkit::SchemaCompilerTemplate schema_template;

    try {
      schema_template = sourcemeta::jsontoolkit::compile(
          schema.value(), sourcemeta::jsontoolkit::default_schema_walker,
          test_resolver, sourcemeta::jsontoolkit::default_schema_compiler);
    } catch (...) {
      std::cout << "\n";
      throw;
    }

    if (verbose) {
      std::cout << "\n";
    }

    for (const auto &test_case : test.at("tests").as_array()) {
      index += 1;

      if (!test_case.is_object()) {
        std::cout
            << "\nerror: Test case documents must be objects\n  at test case #"
            << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (!test_case.defines("data")) {
        std::cout << "\nerror: Test case documents must contain a `data` "
                     "property\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (test_case.defines("description") &&
          !test_case.at("description").is_string()) {
        std::cout << "\nerror: If you set a test case description, it must be "
                     "a string\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (!test_case.defines("valid")) {
        std::cout << "\nerror: Test case documents must contain a `valid` "
                     "property\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (!test_case.at("valid").is_boolean()) {
        std::cout << "\nerror: The test case document `valid` property must be "
                     "a boolean\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/Intelligence-AI/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      std::ostringstream error;
      const auto case_result{sourcemeta::jsontoolkit::evaluate(
          schema_template, test_case.at("data"),
          sourcemeta::jsontoolkit::SchemaCompilerEvaluationMode::Fast,
          pretty_evaluate_callback(error))};

      std::ostringstream test_case_description;
      if (test_case.defines("description")) {
        test_case_description << test_case.at("description").to_string();
      } else {
        test_case_description << "<no description>";
      }

      if (test_case.at("valid").to_boolean() == case_result) {
        pass_count += 1;
        if (verbose) {
          std::cout << "  " << index << "/" << total << " PASS "
                    << test_case_description.str() << "\n";
        }
      } else if (!test_case.at("valid").to_boolean() && case_result) {
        if (!verbose) {
          std::cout << "\n";
        }

        std::cout << "  " << index << "/" << total << " FAIL "
                  << test_case_description.str() << "\n\n"
                  << "error: passed but was expected to fail\n";

        if (index != total && verbose) {
          std::cout << "\n";
        }

        result = false;
      } else {
        if (!verbose) {
          std::cout << "\n";
        }

        std::cout << "  " << index << "/" << total << " FAIL "
                  << test_case_description.str() << "\n\n";
        std::cout << error.str();

        if (index != total && verbose) {
          std::cout << "\n";
        }

        result = false;
      }
    }

    if (!verbose && result) {
      std::cout << " PASS " << pass_count << "/" << total << "\n";
    }
  }

  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
