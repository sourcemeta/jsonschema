#ifndef SOURCEMETA_BLAZE_TEST_H_
#define SOURCEMETA_BLAZE_TEST_H_

#ifndef SOURCEMETA_BLAZE_TEST_EXPORT
#include <sourcemeta/blaze/test_export.h>
#endif

#include <sourcemeta/blaze/test_error.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <chrono>     // std::chrono::steady_clock
#include <cstddef>    // std::size_t
#include <filesystem> // std::filesystem
#include <functional> // std::function
#include <optional>   // std::optional
#include <string>     // std::string
#include <vector>     // std::vector

/// @defgroup test Test
/// @brief A JSON Schema test runner
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/blaze/test.h>
/// ```

namespace sourcemeta::blaze {

/// @ingroup test
/// The monotonic timestamp type used for timing measurements
using TestTimestamp = std::chrono::steady_clock::time_point;

/// @ingroup test
/// Represents a single test case in a test suite
struct SOURCEMETA_BLAZE_TEST_EXPORT TestCase {
// See
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251?view=msvc-170
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  /// The optional description of the test case
  sourcemeta::core::JSON::String description;
  /// Whether the test data is expected to be valid against the schema
  bool valid;
  /// The test data to validate
  sourcemeta::core::JSON data;
  /// The position tracker for error reporting on the data
  sourcemeta::core::PointerPositionTracker tracker;
  /// The position of this test case in the test suite file
  sourcemeta::core::PointerPositionTracker::Position position;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif

  /// Parse a single test case
  static auto
  parse(const sourcemeta::core::JSON &test_case_json,
        const sourcemeta::core::PointerPositionTracker &tracker,
        const std::filesystem::path &base_path,
        const sourcemeta::core::Pointer &location,
        const sourcemeta::core::PointerPositionTracker::Position &position)
      -> TestCase;
};

/// @ingroup test
/// Represents a test suite containing multiple test cases
struct SOURCEMETA_BLAZE_TEST_EXPORT TestSuite {

  /// The result of running a test suite
  struct Result {
    /// The total number of test cases
    std::size_t total;
    /// The number of test cases that passed
    std::size_t passed;
    /// The timestamp when the test suite started executing
    TestTimestamp start;
    /// The timestamp when the test suite finished executing
    TestTimestamp end;
  };

// See
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251?view=msvc-170
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  /// The target schema URIs or file paths
  std::vector<sourcemeta::core::JSON::String> targets;
  /// The list of test cases in the suite
  std::vector<TestCase> tests;
  /// The compiled schema templates for fast validation
  std::vector<Template> schemas_fast;
  /// The compiled schema templates for exhaustive validation
  std::vector<Template> schemas_exhaustive;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
  /// The evaluator instance used for validation
  Evaluator evaluator;

  /// A callback invoked for each test case during execution
  // TODO(C++23): Use std::move_only_function when available in libc++
  using Callback = std::function<void(
      const sourcemeta::core::JSON::String &target, std::size_t index,
      std::size_t total, const TestCase &test_case, bool actual,
      TestTimestamp start, TestTimestamp end)>;

  /// Run all test cases in the suite, invoking the callback for each.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/test.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <sourcemeta/blaze/foundation.h>
  ///
  /// #include <iostream>
  ///
  /// const auto input{R"JSON({
  ///   "target": "https://json-schema.org/draft/2020-12/schema",
  ///   "tests": [
  ///     {
  ///       "data": {
  ///         "$schema": "https://json-schema.org/draft/2020-12/schema"
  ///       },
  ///       "valid": true,
  ///       "description": "valid schema"
  ///     }
  ///   ]
  /// })JSON"};
  ///
  /// sourcemeta::core::PointerPositionTracker tracker;
  /// const auto document{
  ///     sourcemeta::core::parse_json(input, std::ref(tracker))};
  ///
  /// auto suite{sourcemeta::blaze::TestSuite::parse(
  ///     document, tracker, std::filesystem::current_path(),
  ///     sourcemeta::blaze::schema_resolver,
  ///     sourcemeta::blaze::schema_walker,
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// const auto result{suite.run(
  ///     [](const sourcemeta::core::JSON::String &target,
  ///        std::size_t index, std::size_t total,
  ///        const sourcemeta::blaze::TestCase &test_case, bool actual,
  ///        sourcemeta::blaze::TestTimestamp start,
  ///        sourcemeta::blaze::TestTimestamp end) {
  ///       std::cout << target << " " << index << "/" << total << ": "
  ///                 << test_case.description << " - "
  ///                 << (test_case.valid == actual ? "PASS" : "FAIL")
  ///                 << "\n";
  ///     })};
  ///
  /// std::cout << result.passed << "/" << result.total << " passed\n";
  /// ```
  auto run(const Callback &callback) -> Result;

  /// Parse a test suite from a JSON object. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/test.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <sourcemeta/blaze/foundation.h>
  ///
  /// #include <cassert>
  ///
  /// const auto input{R"JSON({
  ///   "target": "https://json-schema.org/draft/2020-12/schema",
  ///   "tests": [
  ///     { "data": {}, "valid": true },
  ///     { "data": [], "valid": false, "description": "Not an object" }
  ///   ]
  /// })JSON"};
  ///
  /// sourcemeta::core::PointerPositionTracker tracker;
  /// const auto document{
  ///     sourcemeta::core::parse_json(input, std::ref(tracker))};
  ///
  /// const auto suite{sourcemeta::blaze::TestSuite::parse(
  ///     document, tracker, std::filesystem::current_path(),
  ///     sourcemeta::blaze::schema_resolver,
  ///     sourcemeta::blaze::schema_walker,
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// assert(suite.targets.size() == 1);
  /// assert(suite.targets.front() ==
  ///   "https://json-schema.org/draft/2020-12/schema");
  /// assert(suite.tests.size() == 2);
  /// ```
  static auto
  parse(const sourcemeta::core::JSON &document,
        const sourcemeta::core::PointerPositionTracker &tracker,
        const std::filesystem::path &base_path,
        const sourcemeta::blaze::SchemaResolver &schema_resolver,
        const sourcemeta::blaze::SchemaWalker &walker, const Compiler &compiler,
        std::string_view default_dialect = "", std::string_view default_id = "",
        const std::optional<Tweaks> &tweaks = std::nullopt) -> TestSuite;
};

} // namespace sourcemeta::blaze

#endif
