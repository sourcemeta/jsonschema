#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/blaze/test.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/parallel.h>

// The parallel module includes windows.h, which defines DELETE as a macro
// that would otherwise break parsing the HTTPMethod enumeration that the
// resolver transitively includes below
#if defined(_WIN32)
#undef DELETE
#endif

#include <algorithm> // std::find, std::distance, std::min, std::max
#include <atomic>    // std::atomic
#include <chrono>    // std::chrono
#include <cstddef>   // std::size_t
#include <exception> // std::exception_ptr, std::current_exception, std::rethrow_exception
#include <iostream>    // std::cout
#include <mutex>       // std::mutex, std::scoped_lock
#include <optional>    // std::optional
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread
#include <vector>      // std::vector

#include "command.h"
#include "configuration.h"
#include "configure.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto print_rdf_failure(const sourcemeta::jsonschema::InputJSON &entry,
                       const std::size_t test_index,
                       const sourcemeta::blaze::TestOutcome &outcome,
                       std::ostream &stream) -> void {
  if (outcome.rdf_error.has_value()) {
    const auto &error{outcome.rdf_error.value()};
    auto position{entry.positions.get(
        sourcemeta::core::Pointer{"tests", test_index, "data"}.concat(
            error.instance_location))};
    if (!position.has_value()) {
      position = entry.positions.get(
          sourcemeta::core::Pointer{"tests", test_index, "dataPath"});
    }

    stream << "error: " << error.message << "\n";
    if (position.has_value()) {
      stream << "  at line " << std::get<0>(position.value()) << "\n";
      stream << "  at column " << std::get<1>(position.value()) << "\n";
    }

    stream << "  at instance location \""
           << sourcemeta::core::to_string(error.instance_location) << "\"\n";
    stream << "  at facet \"" << sourcemeta::jsonschema::facet_name(error.facet)
           << "\"\n";
    stream << "  at schema location " << error.schema_location << "\n";

    if (error.conflicting_schema_location.has_value()) {
      stream << "  at conflicting schema location "
             << error.conflicting_schema_location.value() << "\n";
    }

    if (error.inert_override_location.has_value()) {
      stream << "  at inert override location "
             << error.inert_override_location.value() << "\n";
    }

    stream << "  at file path " << entry.resolution_base.generic_string()
           << "\n";

    if (error.inert_override_location.has_value()) {
      stream << "\nThe x-jsonld-override mark was ignored because it does not "
                "enclose the\n";
      stream << "conflicting annotation. Move the conflicting annotation, or "
                "the reference\n";
      stream << "that brings it in, inside the overriding object for the "
                "override to\n";
      stream << "take effect\n";
    }
  } else {
    auto location{sourcemeta::core::Pointer{"tests", test_index, "rdf"}};
    auto position{entry.positions.get(location)};
    if (!position.has_value()) {
      location = sourcemeta::core::Pointer{"tests", test_index, "rdfPath"};
      position = entry.positions.get(location);
    }

    stream << "error: RDF expansion mismatch\n";
    if (position.has_value()) {
      stream << "  at line " << std::get<0>(position.value()) << "\n";
      stream << "  at column " << std::get<1>(position.value()) << "\n";
    }

    stream << "  at file path " << entry.resolution_base.generic_string()
           << "\n";
    stream << "  at location \"" << sourcemeta::core::to_string(location)
           << "\"\n\n";
    sourcemeta::core::prettify(outcome.rdf.value(), stream);
    stream << "\n";
  }
}

auto parse_test_suite(const sourcemeta::jsonschema::InputJSON &entry,
                      const sourcemeta::blaze::SchemaResolver &schema_resolver,
                      const std::string_view dialect,
                      const std::optional<sourcemeta::blaze::Tweaks> &tweaks)
    -> sourcemeta::blaze::TestSuite {
  try {
    return sourcemeta::blaze::TestSuite::parse(
        entry.second, entry.positions,
        // A test document read from standard input has no directory of its
        // own, and the base path must remain a real directory, as relative
        // `dataPath` and `rdfPath` entries are opened from it
        entry.from_stdin ? std::filesystem::current_path()
                         : entry.resolution_base.parent_path(),
        schema_resolver, sourcemeta::blaze::schema_walker,
        sourcemeta::blaze::default_schema_compiler, dialect, "", tweaks);
  } catch (const sourcemeta::blaze::TestParseError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::TestParseError>{
        entry.resolution_base, error.what(), error.location(), error.line(),
        error.column()};
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::CompilerError &error) {
    // No position, as what compiles here is the schema the document targets
    // while the positions on hand describe the test document itself
    throw sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>{
        entry.resolution_base, error};
  } catch (
      const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownBaseDialectError>{
        entry.resolution_base};
  } catch (const sourcemeta::blaze::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaVocabularyError>{
        entry.resolution_base, error.uri(), error.what()};
  } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaUnknownDialectError>{entry.resolution_base};
  } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
    // No position, as what compiles here is the schema the document targets
    // while the positions on hand describe the test document itself
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::SchemaAnchorCollisionError>{entry.resolution_base,
                                                       error};
  }
}

auto warm_caches(const sourcemeta::core::Options &options,
                 const std::vector<sourcemeta::jsonschema::InputJSON> &entries)
    -> void {
  for (const auto &entry : entries) {
    const auto configuration_path{sourcemeta::jsonschema::find_configuration(
        options, entry.resolution_base)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    [[maybe_unused]] const auto &schema_resolver{
        sourcemeta::jsonschema::resolver(options, options.contains("http"),
                                         dialect, configuration)};
  }
}

auto emit_target_header(
    const bool multi_target, const sourcemeta::core::JSON::String &target,
    std::optional<sourcemeta::core::JSON::String> &last_target_header,
    std::ostream &stream) -> void {
  if (multi_target && last_target_header != target) {
    stream << "  " << target << ":\n";
    last_target_header = target;
  }
}

auto run_suite_as_text(const sourcemeta::core::Options &options,
                       const sourcemeta::jsonschema::InputJSON &entry,
                       const bool verbose, std::ostream &stream)
    -> sourcemeta::blaze::TestSuite::Result {
  const auto configuration_path{sourcemeta::jsonschema::find_configuration(
      options, entry.resolution_base)};
  const auto &configuration{
      sourcemeta::jsonschema::read_configuration(options, configuration_path)};
  const auto dialect{
      sourcemeta::jsonschema::default_dialect(options, configuration)};
  const auto &schema_resolver{sourcemeta::jsonschema::resolver(
      options, options.contains("http"), dialect, configuration)};

  auto test_suite{parse_test_suite(
      entry, schema_resolver, dialect,
      sourcemeta::jsonschema::format_assertion_tweaks(options))};

  stream << entry.first << ":";

  const auto multi_target{test_suite.targets.size() > 1};
  std::optional<sourcemeta::core::JSON::String> last_target_header;

  const auto suite_result{test_suite.run(
      [&](const sourcemeta::core::JSON::String &target, std::size_t index,
          std::size_t total, const sourcemeta::blaze::TestCase &test_case,
          const sourcemeta::blaze::TestOutcome &outcome,
          sourcemeta::blaze::TestTimestamp, sourcemeta::blaze::TestTimestamp) {
        if (verbose && index == 1) {
          stream << "\n";
        }

        const auto *const entry_indent{multi_target ? "    " : "  "};

        const auto &description{test_case.description.empty()
                                    ? "<no description>"
                                    : test_case.description};

        if (outcome.passed) {
          if (verbose) {
            emit_target_header(multi_target, target, last_target_header,
                               stream);
            stream << entry_indent << index << "/" << total << " PASS "
                   << description << "\n";
          }
        } else if (!test_case.valid && outcome.valid) {
          if (!verbose) {
            stream << "\n";
          }
          emit_target_header(multi_target, target, last_target_header, stream);
          stream << entry_indent << index << "/" << total << " FAIL "
                 << description << "\n\n"
                 << "error: Passed but was expected to fail\n";

          if (index != total && verbose) {
            stream << "\n";
          }
        } else if (!outcome.valid) {
          const std::string ref{"$ref"};
          sourcemeta::blaze::SimpleOutput output{test_case.data,
                                                 {std::cref(ref)}};
          const auto target_index{static_cast<std::size_t>(
              std::distance(test_suite.targets.cbegin(),
                            std::find(test_suite.targets.cbegin(),
                                      test_suite.targets.cend(), target)))};
          test_suite.evaluator.validate(test_suite.exhaustive(target_index),
                                        test_case.data, std::ref(output));

          if (!verbose) {
            stream << "\n";
          }
          emit_target_header(multi_target, target, last_target_header, stream);
          stream << entry_indent << index << "/" << total << " FAIL "
                 << description << "\n\n";
          sourcemeta::jsonschema::print(output, test_case.tracker, stream);

          if (index != total && verbose) {
            stream << "\n";
          }
        } else {
          if (!verbose) {
            stream << "\n";
          }
          emit_target_header(multi_target, target, last_target_header, stream);
          stream << entry_indent << index << "/" << total << " FAIL "
                 << description << "\n\n";
          print_rdf_failure(entry, (index - 1) % test_suite.tests.size(),
                            outcome, stream);

          if (index != total && verbose) {
            stream << "\n";
          }
        }
      })};

  if (suite_result.total == 0) {
    stream << " NO TESTS\n";
  } else if (!verbose && suite_result.passed == suite_result.total) {
    stream << " PASS " << suite_result.passed << "/" << suite_result.total
           << "\n";
  }

  return suite_result;
}

auto report_as_text(const sourcemeta::core::Options &options,
                    const std::size_t jobs) -> void {
  bool result{true};
  bool empty_test_suite{false};
  const auto verbose{options.contains("verbose") || options.contains("debug")};

  const auto entries{sourcemeta::jsonschema::for_each_json(options)};
  warm_caches(options, entries);

  std::mutex output_mutex;
  std::atomic<bool> skip_remaining{false};
  std::exception_ptr first_error{nullptr};
  std::string first_error_path;

  sourcemeta::core::parallel_for_each(
      entries.cbegin(), entries.cend(),
      [&](const sourcemeta::jsonschema::InputJSON &entry, const std::size_t,
          const std::size_t) {
        if (skip_remaining.load()) {
          return;
        }

        try {
          // Buffer the output of every suite, so that we only need to hold
          // the output lock while emitting it, letting suites actually
          // evaluate their test cases in parallel
          std::ostringstream buffer;
          const auto suite_result{
              run_suite_as_text(options, entry, verbose, buffer)};

          const std::scoped_lock<std::mutex> lock{output_mutex};
          std::cout << buffer.str();

          if (suite_result.passed != suite_result.total) {
            result = false;
          }

          if (suite_result.total == 0) {
            empty_test_suite = true;
          }
        } catch (...) {
          const std::scoped_lock<std::mutex> lock{output_mutex};
          if (!first_error) {
            first_error = std::current_exception();
            first_error_path = entry.first;
            skip_remaining.store(true);
          }
        }
      },
      jobs);

  if (first_error) {
    std::cout << first_error_path << ":\n";
    std::rethrow_exception(first_error);
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

struct CtrfSuiteReport {
  std::vector<sourcemeta::core::JSON> tests;
  std::size_t passed{0};
  std::size_t total{0};
  sourcemeta::blaze::TestTimestamp start{};
  sourcemeta::blaze::TestTimestamp end{};
};

auto run_suite_as_ctrf(const sourcemeta::core::Options &options,
                       const sourcemeta::jsonschema::InputJSON &entry,
                       CtrfSuiteReport &report) -> void {
  const auto configuration_path{sourcemeta::jsonschema::find_configuration(
      options, entry.resolution_base)};
  const auto &configuration{
      sourcemeta::jsonschema::read_configuration(options, configuration_path)};
  const auto dialect{
      sourcemeta::jsonschema::default_dialect(options, configuration)};
  const auto &schema_resolver{sourcemeta::jsonschema::resolver(
      options, options.contains("http"), dialect, configuration)};

  auto test_suite{parse_test_suite(
      entry, schema_resolver, dialect,
      sourcemeta::jsonschema::format_assertion_tweaks(options))};

  const auto file_path{entry.first};

  const auto suite_result{test_suite.run(
      [&](const sourcemeta::core::JSON::String &target, std::size_t index,
          std::size_t, const sourcemeta::blaze::TestCase &test_case,
          const sourcemeta::blaze::TestOutcome &outcome,
          sourcemeta::blaze::TestTimestamp start,
          sourcemeta::blaze::TestTimestamp end) {
        auto test_object{sourcemeta::core::JSON::make_object()};

        const auto &name{test_case.description.empty() ? "<no description>"
                                                       : test_case.description};
        test_object.assign("name", sourcemeta::core::JSON{name});

        test_object.assign("status", sourcemeta::core::JSON{
                                         outcome.passed ? "passed" : "failed"});

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

        if (!outcome.passed) {
          if (!test_case.valid && outcome.valid) {
            test_object.assign("message",
                               sourcemeta::core::JSON{"Passed but was "
                                                      "expected to fail"});
          } else if (!outcome.valid) {
            std::ostringstream trace_stream;
            const std::string ref{"$ref"};
            sourcemeta::blaze::SimpleOutput output{test_case.data,
                                                   {std::cref(ref)}};
            const auto target_index{static_cast<std::size_t>(
                std::distance(test_suite.targets.cbegin(),
                              std::find(test_suite.targets.cbegin(),
                                        test_suite.targets.cend(), target)))};
            test_suite.evaluator.validate(test_suite.exhaustive(target_index),
                                          test_case.data, std::ref(output));
            sourcemeta::jsonschema::print(output, test_case.tracker,
                                          trace_stream);
            test_object.assign("trace",
                               sourcemeta::core::JSON{trace_stream.str()});
          } else {
            std::ostringstream trace_stream;
            print_rdf_failure(entry, (index - 1) % test_suite.tests.size(),
                              outcome, trace_stream);
            test_object.assign("trace",
                               sourcemeta::core::JSON{trace_stream.str()});
          }
        }

        report.tests.push_back(std::move(test_object));
      })};

  report.passed = suite_result.passed;
  report.total = suite_result.total;
  report.start = suite_result.start;
  report.end = suite_result.end;
}

auto report_as_ctrf(const sourcemeta::core::Options &options,
                    const std::size_t jobs) -> void {
  bool result{true};
  bool empty_test_suite{false};

  const auto system_ref{std::chrono::system_clock::now()};
  const auto steady_ref{std::chrono::steady_clock::now()};

  const auto entries{sourcemeta::jsonschema::for_each_json(options)};
  warm_caches(options, entries);

  std::vector<CtrfSuiteReport> reports{entries.size()};
  std::mutex error_mutex;
  std::atomic<bool> skip_remaining{false};
  std::exception_ptr first_error{nullptr};

  sourcemeta::core::parallel_for_each(
      entries.cbegin(), entries.cend(),
      [&](const sourcemeta::jsonschema::InputJSON &entry, const std::size_t,
          const std::size_t) {
        if (skip_remaining.load()) {
          return;
        }

        try {
          run_suite_as_ctrf(
              options, entry,
              reports[static_cast<std::size_t>(&entry - entries.data())]);
        } catch (...) {
          const std::scoped_lock<std::mutex> lock{error_mutex};
          if (!first_error) {
            first_error = std::current_exception();
            skip_remaining.store(true);
          }
        }
      },
      jobs);

  if (first_error) {
    std::rethrow_exception(first_error);
  }

  auto ctrf_tests{sourcemeta::core::JSON::make_array()};
  std::size_t total_passed{0};
  std::size_t total_failed{0};
  sourcemeta::blaze::TestTimestamp global_start{};
  sourcemeta::blaze::TestTimestamp global_end{};
  bool first_suite{true};

  for (auto &report : reports) {
    if (first_suite) {
      global_start = report.start;
      global_end = report.end;
      first_suite = false;
    } else {
      global_start = std::min(global_start, report.start);
      global_end = std::max(global_end, report.end);
    }

    total_passed += report.passed;
    total_failed += report.total - report.passed;

    if (report.total == 0) {
      empty_test_suite = true;
    }

    if (report.passed != report.total) {
      result = false;
    }

    for (auto &test_object : report.tests) {
      ctrf_tests.push_back(std::move(test_object));
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
  const auto jobs{parse_jobs(options)};
  LOG_VERBOSE(options) << "Using parallelism: " << jobs << "\n";
  if (options.contains("json")) {
    report_as_ctrf(options, jobs);
  } else {
    report_as_text(options, jobs);
  }
}
