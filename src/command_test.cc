#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/blaze/test.h>

#include <sourcemeta/core/json.h>

#include <algorithm>   // std::find, std::distance
#include <chrono>      // std::chrono
#include <iostream>    // std::cout
#include <optional>    // std::optional
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread

#include "command.h"
#include "configuration.h"
#include "configure.h"
#include "error.h"
#include "input.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto parse_test_suite(const sourcemeta::jsonschema::InputJSON &entry,
                      const sourcemeta::blaze::SchemaResolver &schema_resolver,
                      const std::string_view dialect, const bool json_output,
                      const std::optional<sourcemeta::blaze::Tweaks> &tweaks)
    -> sourcemeta::blaze::TestSuite {
  try {
    return sourcemeta::blaze::TestSuite::parse(
        entry.second, entry.positions, entry.resolution_base.parent_path(),
        schema_resolver, sourcemeta::blaze::schema_walker,
        sourcemeta::blaze::default_schema_compiler, dialect, "", tweaks);
  } catch (const sourcemeta::blaze::TestParseError &error) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<sourcemeta::blaze::TestParseError>{
        entry.resolution_base, error.what(), error.location(), error.line(),
        error.column()};
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>{
        entry.resolution_base, error};
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>{
        entry.resolution_base};
  } catch (const sourcemeta::blaze::SchemaVocabularyError &error) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaVocabularyError>{
        entry.resolution_base, error.uri(), error.what()};
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>{entry.resolution_base};
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }

    const auto position{entry.positions.get(error.location())};
    if (position.has_value()) {
      throw sourcemeta::jsonschema::PositionError<sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>>(
          std::get<0>(position.value()), std::get<1>(position.value()),
          entry.resolution_base, error);
    }

    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>{entry.resolution_base,
                                                       error};
  } catch (...) {
    if (!json_output) {
      std::cout << entry.first << ":\n";
    }
    throw;
  }
}

auto report_as_text(const sourcemeta::core::Options &options) -> void {
  bool result{true};
  bool empty_test_suite{false};
  const auto verbose{options.contains("verbose") || options.contains("debug")};

  for (const auto &entry : sourcemeta::jsonschema::for_each_json(options)) {
    const auto configuration_path{
        sourcemeta::jsonschema::find_configuration(entry.resolution_base)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &schema_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};

    auto test_suite{parse_test_suite(
        entry, schema_resolver, dialect, false,
        sourcemeta::jsonschema::format_assertion_tweaks(options))};

    std::cout << entry.first << ":";

    const auto multi_target{test_suite.targets.size() > 1};
    sourcemeta::core::JSON::String last_target_header;
    bool last_target_header_set{false};

    const auto suite_result{test_suite.run(
        [&](const sourcemeta::core::JSON::String &target, std::size_t index,
            std::size_t total, const sourcemeta::blaze::TestCase &test_case,
            bool actual, sourcemeta::blaze::TestTimestamp,
            sourcemeta::blaze::TestTimestamp) {
          if (verbose && index == 1) {
            std::cout << "\n";
          }

          const auto entry_indent{multi_target ? "    " : "  "};

          const auto emit_target_header{[&]() {
            if (multi_target &&
                (!last_target_header_set || last_target_header != target)) {
              std::cout << "  " << target << ":\n";
              last_target_header = target;
              last_target_header_set = true;
            }
          }};

          const auto &description{test_case.description.empty()
                                      ? "<no description>"
                                      : test_case.description};

          if (test_case.valid == actual) {
            if (verbose) {
              emit_target_header();
              std::cout << entry_indent << index << "/" << total << " PASS "
                        << description << "\n";
            }
          } else if (!test_case.valid && actual) {
            if (!verbose) {
              std::cout << "\n";
            }
            emit_target_header();
            std::cout << entry_indent << index << "/" << total << " FAIL "
                      << description << "\n\n"
                      << "error: Passed but was expected to fail\n";

            if (index != total && verbose) {
              std::cout << "\n";
            }
          } else {
            const std::string ref{"$ref"};
            sourcemeta::blaze::SimpleOutput output{test_case.data,
                                                   {std::cref(ref)}};
            const auto target_index{static_cast<std::size_t>(
                std::distance(test_suite.targets.cbegin(),
                              std::find(test_suite.targets.cbegin(),
                                        test_suite.targets.cend(), target)))};
            test_suite.evaluator.validate(
                test_suite.schemas_exhaustive[target_index], test_case.data,
                std::ref(output));

            if (!verbose) {
              std::cout << "\n";
            }
            emit_target_header();
            std::cout << entry_indent << index << "/" << total << " FAIL "
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
      empty_test_suite = true;
      std::cout << " NO TESTS\n";
    } else if (!verbose && suite_result.passed == suite_result.total) {
      std::cout << " PASS " << suite_result.passed << "/" << suite_result.total
                << "\n";
    }
  }

  if (!result) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_EXPECTED_FAILURE};
  }

  // An empty test suite likely means the author forgot to write the tests,
  // so don't let it silently succeed
  if (empty_test_suite) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_OTHER_INPUT_ERROR};
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
  bool empty_test_suite{false};

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
        sourcemeta::jsonschema::find_configuration(entry.resolution_base)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &schema_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};

    auto test_suite{parse_test_suite(
        entry, schema_resolver, dialect, true,
        sourcemeta::jsonschema::format_assertion_tweaks(options))};

    const auto file_path{entry.first};

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

          const auto [test_line, test_column, test_end_line, test_end_column] =
              test_case.position;
          test_object.assign("line", sourcemeta::core::JSON{
                                         static_cast<std::int64_t>(test_line)});
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
              const auto target_index{static_cast<std::size_t>(
                  std::distance(test_suite.targets.cbegin(),
                                std::find(test_suite.targets.cbegin(),
                                          test_suite.targets.cend(), target)))};
              test_suite.evaluator.validate(
                  test_suite.schemas_exhaustive[target_index], test_case.data,
                  std::ref(output));
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

    if (suite_result.total == 0) {
      empty_test_suite = true;
    }

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
  tool.assign("version",
              sourcemeta::core::JSON{sourcemeta::jsonschema::PROJECT_VERSION});

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
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_EXPECTED_FAILURE};
  }

  // An empty test suite likely means the author forgot to write the tests,
  // so don't let it silently succeed
  if (empty_test_suite) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_OTHER_INPUT_ERROR};
  }
}

} // namespace

auto sourcemeta::jsonschema::test(const sourcemeta::core::Options &options)
    -> void {
  validate_http_headers(options);
  if (options.contains("json")) {
    report_as_ctrf(options);
  } else {
    report_as_text(options);
  }
}
