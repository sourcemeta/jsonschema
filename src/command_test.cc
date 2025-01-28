#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <cstdlib>    // EXIT_SUCCESS, EXIT_FAILURE
#include <filesystem> // std::filesystem
#include <iostream>   // std::cerr, std::cout

#include "command.h"
#include "utils.h"

static auto get_schema_object(const sourcemeta::core::URI &identifier,
                              const sourcemeta::core::SchemaResolver &resolver)
    -> std::optional<sourcemeta::core::JSON> {
  const auto schema{resolver(identifier.recompose())};
  if (schema.has_value()) {
    return schema;
  }

  // Resolving a schema identifier that contains a fragment (i.e. a JSON Pointer
  // one) can be tricky, as we might end up re-inventing JSON Schema referencing
  // all over again. To make it work without much hassle, we do exactly that:
  // create an artificial schema wrapper that uses `$ref`.
  if (identifier.fragment().has_value()) {
    auto result{sourcemeta::core::JSON::make_object()};
    result.assign("$schema", sourcemeta::core::JSON{
                                 "http://json-schema.org/draft-07/schema#"});
    result.assign("$ref", sourcemeta::core::JSON{identifier.recompose()});
    return result;
  }

  return std::nullopt;
}

static auto get_data(const sourcemeta::core::JSON &test_case,
                     const std::filesystem::path &base, const bool verbose)
    -> sourcemeta::core::JSON {
  assert(base.is_absolute());
  assert(test_case.is_object());
  assert(test_case.defines("data") || test_case.defines("dataPath"));
  if (test_case.defines("data")) {
    return test_case.at("data");
  }

  assert(test_case.defines("dataPath"));
  assert(test_case.at("dataPath").is_string());

  const std::filesystem::path data_path{std::filesystem::weakly_canonical(
      base / test_case.at("dataPath").to_string())};
  if (verbose) {
    std::cerr << "Reading test instance file: " << data_path.string() << "\n";
  }

  try {
    return sourcemeta::jsonschema::cli::read_file(data_path);
  } catch (...) {
    std::cout << "\n";
    throw;
  }
}

auto sourcemeta::jsonschema::cli::test(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {"h", "http"})};
  bool result{true};
  const auto test_resolver{
      resolver(options, options.contains("h") || options.contains("http"))};
  const auto verbose{options.contains("verbose") || options.contains("v")};
  sourcemeta::blaze::Evaluator evaluator;

  for (const auto &entry : for_each_json(options.at(""), parse_ignore(options),
                                         parse_extensions(options))) {
    const sourcemeta::core::JSON test{
        sourcemeta::jsonschema::cli::read_file(entry.first)};
    std::cout << entry.first.string() << ":";

    if (!test.is_object()) {
      std::cout << "\nerror: The test document must be an object\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.defines("target")) {
      std::cout
          << "\nerror: The test document must contain a `target` property\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.at("target").is_string()) {
      std::cout
          << "\nerror: The test document `target` property must be a URI\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.defines("tests")) {
      std::cout
          << "\nerror: The test document must contain a `tests` property\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    if (!test.at("tests").is_array()) {
      std::cout
          << "\nerror: The test document `tests` property must be an array\n\n";
      std::cout << "Learn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/test.markdown\n";
      return EXIT_FAILURE;
    }

    sourcemeta::core::URI schema_uri{test.at("target").to_string()};
    schema_uri.canonicalize();
    const auto schema{get_schema_object(schema_uri, test_resolver)};
    if (!schema.has_value()) {
      std::cout << "\n";
      throw sourcemeta::core::SchemaResolutionError(
          test.at("target").to_string(), "Could not resolve schema under test");
    }

    unsigned int pass_count{0};
    unsigned int index{0};
    const auto total{test.at("tests").size()};

    if (test.at("tests").empty()) {
      std::cout << " NO TESTS\n";
      continue;
    }

    sourcemeta::blaze::Template schema_template;

    try {
      schema_template = sourcemeta::blaze::compile(
          schema.value(), sourcemeta::core::default_schema_walker,
          test_resolver, sourcemeta::blaze::default_schema_compiler);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      if (error.location() == sourcemeta::core::Pointer{"$ref"} &&
          error.id() == schema_uri.recompose()) {
        std::cout << "\n";
        throw sourcemeta::core::SchemaResolutionError(
            test.at("target").to_string(),
            "Could not resolve schema under test");
      }

      throw;
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
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (!test_case.defines("data") && !test_case.defines("dataPath")) {
        std::cout << "\nerror: Test case documents must contain a `data` or "
                     "`dataPath` property\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (test_case.defines("data") && test_case.defines("dataPath")) {
        std::cout
            << "\nerror: Test case documents must contain either a `data` or "
               "`dataPath` property, but not both\n  at test case #"
            << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (test_case.defines("dataPath") &&
          !test_case.at("dataPath").is_string()) {
        std::cout << "\nerror: Test case documents must set the `dataPath` "
                     "property to a string\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (test_case.defines("description") &&
          !test_case.at("description").is_string()) {
        std::cout << "\nerror: If you set a test case description, it must be "
                     "a string\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (!test_case.defines("valid")) {
        std::cout << "\nerror: Test case documents must contain a `valid` "
                     "property\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      if (!test_case.at("valid").is_boolean()) {
        std::cout << "\nerror: The test case document `valid` property must be "
                     "a boolean\n  at test case #"
                  << index << "\n\n";
        std::cout << "Learn more here: "
                     "https://github.com/sourcemeta/jsonschema/blob/main/"
                     "docs/test.markdown\n";
        return EXIT_FAILURE;
      }

      const auto instance{
          get_data(test_case, entry.first.parent_path(), verbose)};
      const std::string ref{"$ref"};
      sourcemeta::blaze::ErrorOutput output{instance, {std::cref(ref)}};
      const auto case_result{
          evaluator.validate(schema_template, instance, std::ref(output))};

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
                  << "error: Passed but was expected to fail\n";

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
        print(output, std::cout);

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
