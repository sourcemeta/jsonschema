#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <cstdlib>    // EXIT_FAILURE
#include <filesystem> // std::filesystem
#include <iostream>   // std::cerr, std::cout

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

static auto get_data(const sourcemeta::core::JSON &test_case,
                     const std::filesystem::path &base,
                     const sourcemeta::core::Options &options,
                     sourcemeta::core::PointerPositionTracker &tracker)
    -> sourcemeta::core::JSON {
  assert(base.is_absolute());
  assert(test_case.is_object());
  assert(test_case.defines("data") || test_case.defines("dataPath"));
  if (test_case.defines("data")) {
    return test_case.at("data");
  }

  assert(test_case.defines("dataPath"));
  assert(test_case.at("dataPath").is_string());

  const std::filesystem::path data_path{sourcemeta::core::weakly_canonical(
      base / test_case.at("dataPath").to_string())};
  sourcemeta::jsonschema::LOG_VERBOSE(options)
      << "Reading test instance file: " << data_path.string() << "\n";

  try {
    return sourcemeta::core::read_yaml_or_json(data_path, std::ref(tracker));
  } catch (...) {
    std::cout << "\n";
    throw;
  }
}

auto sourcemeta::jsonschema::test(const sourcemeta::core::Options &options)
    -> void {
  bool result{true};

  const auto verbose{options.contains("verbose")};
  sourcemeta::blaze::Evaluator evaluator;

  for (const auto &entry : for_each_json(options)) {
    const auto configuration_path{find_configuration(entry.first)};
    const auto &configuration{read_configuration(options, configuration_path)};
    const auto dialect{default_dialect(options, configuration)};
    const auto &test_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};

    const sourcemeta::core::JSON test{
        sourcemeta::core::read_yaml_or_json(entry.first)};

    if (!test.is_object()) {
      std::cout << entry.first.string() << ":\n";
      throw TestError{"The test document must be an object", std::nullopt};
    }

    if (!test.defines("target")) {
      std::cout << entry.first.string() << ":\n";
      throw TestError{"The test document must contain a `target` property",
                      std::nullopt};
    }

    if (!test.at("target").is_string()) {
      std::cout << entry.first.string() << ":\n";
      throw TestError{"The test document `target` property must be a URI",
                      std::nullopt};
    }

    if (!test.defines("tests")) {
      std::cout << entry.first.string() << ":\n";
      throw TestError{"The test document must contain a `tests` property",
                      std::nullopt};
    }

    if (!test.at("tests").is_array()) {
      std::cout << entry.first.string() << ":\n";
      throw TestError{"The test document `tests` property must be an array",
                      std::nullopt};
    }

    const auto test_path_uri{sourcemeta::core::URI::from_path(entry.first)};
    sourcemeta::core::URI schema_uri{test.at("target").to_string()};
    schema_uri.resolve_from(test_path_uri);
    schema_uri.canonicalize();

    LOG_VERBOSE(options) << "Looking for target: " << schema_uri.recompose()
                         << "\n";

    const auto schema{sourcemeta::core::wrap(schema_uri.recompose())};

    unsigned int pass_count{0};
    unsigned int index{0};
    const auto total{test.at("tests").size()};

    if (test.at("tests").empty()) {
      std::cout << entry.first.string() << ":";
      std::cout << " NO TESTS\n";
      continue;
    }

    sourcemeta::blaze::Template schema_template;

    try {
      schema_template = sourcemeta::blaze::compile(
          schema, sourcemeta::core::schema_official_walker, test_resolver,
          sourcemeta::blaze::default_schema_compiler,
          sourcemeta::blaze::Mode::FastValidation, dialect);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      if (error.location() == sourcemeta::core::Pointer{"$ref"} &&
          error.id() == schema_uri.recompose()) {
        std::cout << entry.first.string() << ":";
        std::cout << "\n";
        throw sourcemeta::core::SchemaResolutionError(
            test.at("target").to_string(),
            "Could not resolve schema under test");
      }

      throw;
    } catch (...) {
      std::cout << entry.first.string() << ":";
      std::cout << "\n";
      throw;
    }

    std::cout << entry.first.string() << ":";
    if (verbose) {
      std::cout << "\n";
    }

    for (const auto &test_case : test.at("tests").as_array()) {
      index += 1;

      if (!test_case.is_object()) {
        std::cout << "\n";
        throw TestError{"Test case documents must be objects", index};
      }

      if (!test_case.defines("data") && !test_case.defines("dataPath")) {
        std::cout << "\n";
        throw TestError{
            "Test case documents must contain a `data` or `dataPath` property",
            index};
      }

      if (test_case.defines("data") && test_case.defines("dataPath")) {
        std::cout << "\n";
        throw TestError{"Test case documents must contain either a `data` or "
                        "`dataPath` property, but not both",
                        index};
      }

      if (test_case.defines("dataPath") &&
          !test_case.at("dataPath").is_string()) {
        std::cout << "\n";
        throw TestError{
            "Test case documents must set the `dataPath` property to a string",
            index};
      }

      if (test_case.defines("description") &&
          !test_case.at("description").is_string()) {
        std::cout << "\n";
        throw TestError{
            "If you set a test case description, it must be a string", index};
      }

      if (!test_case.defines("valid")) {
        std::cout << "\n";
        throw TestError{"Test case documents must contain a `valid` property",
                        index};
      }

      if (!test_case.at("valid").is_boolean()) {
        std::cout << "\n";
        throw TestError{
            "The test case document `valid` property must be a boolean", index};
      }

      sourcemeta::core::PointerPositionTracker tracker;
      const auto instance{
          get_data(test_case, entry.first.parent_path(), options, tracker)};
      const std::string ref{"$ref"};
      sourcemeta::blaze::SimpleOutput output{instance, {std::cref(ref)}};
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
        print(output, tracker, std::cout);

        if (index != total && verbose) {
          std::cout << "\n";
        }

        result = false;
      }
    }

    if (!verbose && pass_count == total) {
      std::cout << " PASS " << pass_count << "/" << total << "\n";
    }
  }

  if (!result) {
    throw Fail{EXIT_FAILURE};
  }
}
