#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/blaze/test.h>

#include <sourcemeta/core/json.h>

#include <chrono>      // std::chrono
#include <cstdlib>     // EXIT_FAILURE
#include <iostream>    // std::cerr, std::cout
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread

#include "command.h"
#include "configuration.h"
#include "configure.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto parse_test_suite(const sourcemeta::jsonschema::InputJSON &entry,
                      const sourcemeta::core::SchemaResolver &schema_resolver,
                      const std::string_view dialect, const bool json_output)
    -> sourcemeta::blaze::TestSuite {
  try {
    return sourcemeta::blaze::TestSuite::parse(
        entry.second, entry.positions, entry.first.parent_path(),
        schema_resolver, sourcemeta::core::schema_walker,
        sourcemeta::blaze::default_schema_compiler, dialect);
  } catch (const sourcemeta::blaze::TestParseError &error) {
    if (!json_output) {
      std::cout << entry.first.string() << ":\n";
    }
    throw sourcemeta::jsonschema::FileError<sourcemeta::blaze::TestParseError>{
        entry.first, error.what(), error.location(), error.line(),
        error.column()};
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    if (!json_output) {
      std::cout << entry.first.string() << ":\n";
    }
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>{entry.first,
                                                                  error};
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    if (!json_output) {
      std::cout << entry.first.string() << ":\n";
    }
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>{entry.first,
                                                                   error};
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    if (!json_output) {
      std::cout << entry.first.string() << ":\n";
    }
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::core::SchemaResolutionError>{entry.first, error};
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    if (!json_output) {
      std::cout << entry.first.string() << ":\n";
    }
    throw sourcemeta::jsonschema::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>{entry.first};
  } catch (...) {
    if (!json_output) {
      std::cout << entry.first.string() << ":\n";
    }
    throw;
  }
}

auto report_as_text(const sourcemeta::core::Options &options) -> void {
  bool result{true};
  const auto verbose{options.contains("verbose") || options.contains("debug")};

  for (const auto &entry : sourcemeta::jsonschema::for_each_json(options)) {
    const auto configuration_path{
        sourcemeta::jsonschema::find_configuration(entry.first)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &schema_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};

    auto test_suite{parse_test_suite(entry, schema_resolver, dialect, false)};

    std::cout << entry.first.string() << ":";

    const auto suite_result{test_suite.run(
        [&](const sourcemeta::core::JSON::String &, std::size_t index,
            std::size_t total, const sourcemeta::blaze::TestCase &test_case,
            bool actual, sourcemeta::blaze::TestTimestamp,
            sourcemeta::blaze::TestTimestamp) {
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
            const std::string ref{"$ref"};
            sourcemeta::blaze::SimpleOutput output{test_case.data,
                                                   {std::cref(ref)}};
            test_suite.evaluator.validate(test_suite.schema_exhaustive,
                                          test_case.data, std::ref(output));

            if (!verbose) {
              std::cout << "\n";
            }

            std::cout << "  " << index << "/" << total << " FAIL "
                      << description << "\n\n";
            sourcemeta::jsonschema::print(output, test_case.tracker, std::cout);

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
    throw sourcemeta::jsonschema::Fail{2};
  }
}

auto timestamp_to_unix_ms(
    const sourcemeta::blaze::TestTimestamp &timestamp,
    const std::chrono::system_clock::time_point &system_ref,
    const sourcemeta::blaze::TestTimestamp &steady_ref) -> std::int64_t {
  const auto offset{timestamp - steady_ref};
  const auto unix_time{system_ref + offset};
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             unix_time.time_since_epoch())
      .count();
}

auto duration_ms(const sourcemeta::blaze::TestTimestamp &start,
                 const sourcemeta::blaze::TestTimestamp &end) -> std::int64_t {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
      .count();
}

auto report_as_ctrf(const sourcemeta::core::Options &options) -> void {
  bool result{true};

  const auto system_ref{std::chrono::system_clock::now()};
  const auto steady_ref{std::chrono::steady_clock::now()};

  auto ctrf_tests{sourcemeta::core::JSON::make_array()};
  std::size_t total_passed{0};
  std::size_t total_failed{0};
  sourcemeta::blaze::TestTimestamp global_start{};
  sourcemeta::blaze::TestTimestamp global_end{};
  bool first_suite{true};

  for (const auto &entry : sourcemeta::jsonschema::for_each_json(options)) {
    const auto configuration_path{
        sourcemeta::jsonschema::find_configuration(entry.first)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &schema_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};

    auto test_suite{parse_test_suite(entry, schema_resolver, dialect, true)};

    const auto file_path{
        sourcemeta::core::weakly_canonical(entry.first).string()};

    const auto suite_result{test_suite.run(
        [&](const sourcemeta::core::JSON::String &target, std::size_t,
            std::size_t, const sourcemeta::blaze::TestCase &test_case,
            bool actual, sourcemeta::blaze::TestTimestamp start,
            sourcemeta::blaze::TestTimestamp end) {
          auto test_object{sourcemeta::core::JSON::make_object()};

          const auto &name{test_case.description.empty()
                               ? "<no description>"
                               : test_case.description};
          test_object.assign("name", sourcemeta::core::JSON{name});

          const bool passed{test_case.valid == actual};
          test_object.assign(
              "status", sourcemeta::core::JSON{passed ? "passed" : "failed"});

          test_object.assign("duration",
                             sourcemeta::core::JSON{duration_ms(start, end)});
          auto suite{sourcemeta::core::JSON::make_array()};
          suite.push_back(sourcemeta::core::JSON{target});
          test_object.assign("suite", std::move(suite));
          test_object.assign("type", sourcemeta::core::JSON{"unit"});
          test_object.assign("filePath", sourcemeta::core::JSON{file_path});

          test_object.assign("line",
                             sourcemeta::core::JSON{static_cast<std::int64_t>(
                                 std::get<0>(test_case.position))});
          test_object.assign(
              "retries", sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
          test_object.assign("flaky", sourcemeta::core::JSON{false});
          std::ostringstream thread_id_stream;
          thread_id_stream << std::this_thread::get_id();
          test_object.assign("threadId",
                             sourcemeta::core::JSON{thread_id_stream.str()});

          if (!passed) {
            if (!test_case.valid && actual) {
              test_object.assign("message",
                                 sourcemeta::core::JSON{"Passed but was "
                                                        "expected to fail"});
            } else {
              std::ostringstream trace_stream;
              const std::string ref{"$ref"};
              sourcemeta::blaze::SimpleOutput output{test_case.data,
                                                     {std::cref(ref)}};
              test_suite.evaluator.validate(test_suite.schema_exhaustive,
                                            test_case.data, std::ref(output));
              sourcemeta::jsonschema::print(output, test_case.tracker,
                                            trace_stream);
              test_object.assign("trace",
                                 sourcemeta::core::JSON{trace_stream.str()});
            }
          }

          ctrf_tests.push_back(test_object);
        })};

    if (first_suite) {
      global_start = suite_result.start;
      first_suite = false;
    }
    global_end = suite_result.end;

    total_passed += suite_result.passed;
    total_failed += suite_result.total - suite_result.passed;

    if (suite_result.passed != suite_result.total) {
      result = false;
    }
  }

  // Build CTRF output
  auto summary{sourcemeta::core::JSON::make_object()};
  summary.assign("tests", sourcemeta::core::JSON{static_cast<std::int64_t>(
                              total_passed + total_failed)});
  summary.assign("passed", sourcemeta::core::JSON{
                               static_cast<std::int64_t>(total_passed)});
  summary.assign("failed", sourcemeta::core::JSON{
                               static_cast<std::int64_t>(total_failed)});
  summary.assign("pending",
                 sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
  summary.assign("skipped",
                 sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
  summary.assign("other", sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
  summary.assign("start", sourcemeta::core::JSON{timestamp_to_unix_ms(
                              global_start, system_ref, steady_ref)});
  summary.assign("stop", sourcemeta::core::JSON{timestamp_to_unix_ms(
                             global_end, system_ref, steady_ref)});

  auto tool{sourcemeta::core::JSON::make_object()};
  tool.assign("name", sourcemeta::core::JSON{"jsonschema"});
  tool.assign("version", sourcemeta::core::JSON{std::string{
                             sourcemeta::jsonschema::PROJECT_VERSION}});

  auto results{sourcemeta::core::JSON::make_object()};
  results.assign("tool", std::move(tool));
  results.assign("summary", std::move(summary));
  results.assign("tests", std::move(ctrf_tests));

  auto ctrf{sourcemeta::core::JSON::make_object()};
  ctrf.assign("reportFormat", sourcemeta::core::JSON{"CTRF"});
  ctrf.assign("specVersion", sourcemeta::core::JSON{"0.0.0"});
  ctrf.assign("results", std::move(results));

  sourcemeta::core::prettify(ctrf, std::cout);
  std::cout << "\n";

  if (!result) {
    throw sourcemeta::jsonschema::Fail{2};
  }
}

} // namespace

auto sourcemeta::jsonschema::test(const sourcemeta::core::Options &options)
    -> void {
  if (options.contains("json")) {
    report_as_ctrf(options);
  } else {
    report_as_text(options);
  }
}
