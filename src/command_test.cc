#include <sourcemeta/blaze/output.h>

#include <cstdlib>  // EXIT_FAILURE
#include <iostream> // std::cerr, std::cout
#include <string>   // std::string

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "test.h"
#include "utils.h"

auto sourcemeta::jsonschema::test(const sourcemeta::core::Options &options)
    -> void {
  bool result{true};

  const auto verbose{options.contains("verbose")};

  for (const auto &entry : for_each_json(options)) {
    const auto configuration_path{find_configuration(entry.first)};
    const auto &configuration{read_configuration(options, configuration_path)};
    const auto dialect{default_dialect(options, configuration)};

    const auto &schema_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};

    std::optional<sourcemeta::jsonschema::TestSuite> test_suite;
    try {
      test_suite.emplace(sourcemeta::jsonschema::TestSuite::parse(
          entry.second, entry.positions, entry.first.parent_path(),
          schema_resolver, sourcemeta::core::schema_walker,
          sourcemeta::blaze::default_schema_compiler, dialect));
    } catch (const sourcemeta::jsonschema::TestParseError &error) {
      std::cout << entry.first.string() << ":\n";
      throw FileError<sourcemeta::jsonschema::TestParseError>{
          entry.first, error.what(), error.location(), error.line(),
          error.column()};
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      std::cout << entry.first.string() << ":\n";
      throw FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>{
          entry.first, error};
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      std::cout << entry.first.string() << ":\n";
      throw FileError<sourcemeta::core::SchemaResolutionError>{entry.first,
                                                               error};
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      std::cout << entry.first.string() << ":\n";
      throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>{
          entry.first};
    } catch (...) {
      std::cout << entry.first.string() << ":\n";
      throw;
    }

    std::cout << entry.first.string() << ":";

    const auto suite_result{test_suite->run(
        [&](std::size_t index, std::size_t total,
            const sourcemeta::jsonschema::TestCase &test_case, bool actual) {
          if (index == 1 && verbose) {
            std::cout << "\n";
          }

          const auto &description{test_case.description.empty()
                                      ? "<no description>"
                                      : test_case.description};

          if (test_case.valid == actual) {
            if (verbose) {
              std::cout << "  " << index << "/" << total << " PASS "
                        << description << "\n";
            }
          } else if (!test_case.valid && actual) {
            if (!verbose) {
              std::cout << "\n";
            }

            std::cout << "  " << index << "/" << total << " FAIL "
                      << description << "\n\n"
                      << "error: Passed but was expected to fail\n";

            if (index != total && verbose) {
              std::cout << "\n";
            }
          } else {
            // Re-run with exhaustive mode to get detailed error output
            const std::string ref{"$ref"};
            sourcemeta::blaze::SimpleOutput output{test_case.data,
                                                   {std::cref(ref)}};
            test_suite->evaluator.validate(test_suite->schema_exhaustive,
                                           test_case.data, std::ref(output));

            if (!verbose) {
              std::cout << "\n";
            }

            std::cout << "  " << index << "/" << total << " FAIL "
                      << description << "\n\n";
            print(output, test_case.tracker, std::cout);

            if (index != total && verbose) {
              std::cout << "\n";
            }
          }
        })};

    if (suite_result.passed != suite_result.total) {
      result = false;
    }

    if (suite_result.total == 0) {
      std::cout << " NO TESTS\n";
    } else if (!verbose && suite_result.passed == suite_result.total) {
      std::cout << " PASS " << suite_result.passed << "/" << suite_result.total
                << "\n";
    }
  }

  if (!result) {
    // Report a different exit code for test failures, to
    // distinguish them from other errors
    throw Fail{2};
  }
}
