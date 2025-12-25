#ifndef SOURCEMETA_BLAZE_TEST_H_
#define SOURCEMETA_BLAZE_TEST_H_

#ifndef SOURCEMETA_BLAZE_TEST_EXPORT
#include <sourcemeta/blaze/test_export.h>
#endif

#include <sourcemeta/blaze/test_error.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

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
/// Represents a single test case in a test suite
struct SOURCEMETA_BLAZE_TEST_EXPORT TestCase {
  /// The optional description of the test case
  sourcemeta::core::JSON::String description;
  /// Whether the test data is expected to be valid against the schema
  bool valid;
  /// The test data to validate
  sourcemeta::core::JSON data;
  /// The position tracker for error reporting
  sourcemeta::core::PointerPositionTracker tracker;

  /// Parse a single test case
  static auto parse(const sourcemeta::core::JSON &test_case_json,
                    const sourcemeta::core::PointerPositionTracker &tracker,
                    const std::filesystem::path &base_path,
                    const sourcemeta::core::Pointer &location) -> TestCase;
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
  };

  /// The target schema URI or file path
  sourcemeta::core::JSON::String target;
  /// The list of test cases in the suite
  std::vector<TestCase> tests;
  /// The compiled schema template for fast validation
  Template schema_fast;
  /// The compiled schema template for exhaustive validation
  Template schema_exhaustive;
  /// The evaluator instance used for validation
  Evaluator evaluator;

  /// A callback invoked for each test case during execution
  using Callback = std::function<void(std::size_t index, std::size_t total,
                                      const TestCase &test_case, bool actual)>;

  /// Run all test cases in the suite, invoking the callback for each.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/blaze/test.h>
  /// #include <sourcemeta/blaze/compiler.h>
  ///
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/jsonpointer.h>
  /// #include <sourcemeta/core/jsonschema.h>
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
  ///     sourcemeta::core::schema_resolver,
  ///     sourcemeta::core::schema_walker,
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// const auto result{suite.run(
  ///     [](std::size_t index, std::size_t total,
  ///        const sourcemeta::blaze::TestCase &test_case, bool actual) {
  ///       std::cout << index << "/" << total << ": "
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
  /// #include <sourcemeta/core/jsonschema.h>
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
  ///     sourcemeta::core::schema_resolver,
  ///     sourcemeta::core::schema_walker,
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// assert(suite.target == "https://json-schema.org/draft/2020-12/schema");
  /// assert(suite.tests.size() == 2);
  /// ```
  static auto
  parse(const sourcemeta::core::JSON &document,
        const sourcemeta::core::PointerPositionTracker &tracker,
        const std::filesystem::path &base_path,
        const sourcemeta::core::SchemaResolver &schema_resolver,
        const sourcemeta::core::SchemaWalker &walker, const Compiler &compiler,
        const std::optional<std::string> &default_dialect = std::nullopt,
        const std::optional<std::string> &default_id = std::nullopt,
        const std::optional<Tweaks> &tweaks = std::nullopt) -> TestSuite;
};

} // namespace sourcemeta::blaze

#endif
