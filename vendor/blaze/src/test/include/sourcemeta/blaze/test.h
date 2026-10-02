#ifndef SOURCEMETA_BLAZE_TEST_H_
#define SOURCEMETA_BLAZE_TEST_H_

#ifndef SOURCEMETA_BLAZE_TEST_EXPORT
#include <sourcemeta/blaze/test_export.h>
#endif

#include <sourcemeta/blaze/test_error.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

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
  /// The expected promotion of the test data to expanded-form JSON-LD
  std::optional<sourcemeta::core::JSON> rdf;
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
/// Represents the outcome of evaluating a single test case against a target
struct SOURCEMETA_BLAZE_TEST_EXPORT TestOutcome {
// See
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251?view=msvc-170
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  /// Whether the test case passed overall
  bool passed;
  /// The actual validity outcome of the test data against the target
  bool valid;
  /// The actual expansion, when RDF promotion ran and succeeded
  std::optional<sourcemeta::core::JSON> rdf;
  /// The resolution error, when RDF promotion failed
  std::optional<JSONLDResolutionError> rdf_error;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup test
/// Where a target of a test document lives, as the caller resolved it
///
/// The document and the frame are borrowed rather than copied, and the
/// exhaustive template of a target is compiled on the first request, so both
/// must outlive the test suite
struct SOURCEMETA_BLAZE_TEST_EXPORT TestTarget {
// See
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251?view=msvc-170
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  /// The document that holds the schema under test, which may be that schema
  /// or a wrapper that holds it, such as an OpenAPI description
  const sourcemeta::core::JSON &document;
  /// The frame of the schemas that the document holds. For a wrapper, this is
  /// what framing the places it keeps its schemas in reports, such as what
  /// sourcemeta::core::OpenAPIFrame::schemas returns
  const sourcemeta::core::SchemaFrame &frame;
  /// The URI of the schema under test, which the frame must locate. This is
  /// typically the target itself, as what a test document names a schema by
  /// and what framing the document it lives in keys that schema by are the
  /// same URI. An empty string names whatever the frame was rooted at
  sourcemeta::core::JSON::String entrypoint;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

/// @ingroup test
/// How to reach what a target names, invoked once per target of a test
/// document in the order that the document lists them
// TODO(C++23): Use std::move_only_function when available in libc++
using TestTargetResolver =
    std::function<TestTarget(const sourcemeta::core::JSON::String &target)>;

/// @ingroup test
/// Represents a test suite containing multiple test cases
///
/// A test suite does not resolve or frame anything. The caller hands over the
/// document that holds each target, a frame of the schemas in it, and where in
/// it the schema under test sits. That is what lets a target name a schema
/// inside a document that is not a schema itself, such as an OpenAPI
/// description, without this module knowing anything about wrappers
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
  /// The target schema URIs or file paths, resolved against the location of
  /// the test document
  std::vector<sourcemeta::core::JSON::String> targets;
  /// The list of test cases in the suite
  std::vector<TestCase> tests;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
  /// The evaluator instance used for validation
  Evaluator evaluator;

  /// A callback invoked for each test case during execution
  // TODO(C++23): Use std::move_only_function when available in libc++
  using Callback = std::function<void(
      const sourcemeta::core::JSON::String &target, std::size_t target_index,
      std::size_t index, std::size_t total, const TestCase &test_case,
      const TestOutcome &outcome, TestTimestamp start, TestTimestamp end)>;

  /// The compiled schema template for fast validation of the given target
  [[nodiscard]] auto fast(std::size_t target_index) const -> const Template &;

  /// The compiled schema template for exhaustive validation of the given
  /// target, compiled on the first request and cached from then on. A test
  /// suite must not be shared across threads
  auto exhaustive(std::size_t target_index) -> const Template &;

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
  /// #include <filesystem>
  /// #include <functional>
  /// #include <iostream>
  ///
  /// const auto input{R"JSON({
  ///   "target": "https://example.com/string",
  ///   "tests": [
  ///     { "data": "foo", "valid": true, "description": "a string" }
  ///   ]
  /// })JSON"};
  ///
  /// sourcemeta::core::PointerPositionTracker tracker;
  /// sourcemeta::core::JSON document{nullptr};
  /// sourcemeta::core::parse_json(input, document, std::ref(tracker));
  ///
  /// const auto schema{sourcemeta::core::parse_json(R"JSON({
  ///   "$schema": "https://json-schema.org/draft/2020-12/schema",
  ///   "$id": "https://example.com/string",
  ///   "type": "string"
  /// })JSON")};
  ///
  /// const sourcemeta::core::SchemaFrame frame{
  ///     sourcemeta::core::SchemaFrame::Mode::References, schema,
  ///     sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver};
  ///
  /// auto suite{sourcemeta::blaze::TestSuite::parse(
  ///     document, tracker, std::filesystem::current_path(),
  ///     [&schema, &frame](const sourcemeta::core::JSON::String &target)
  ///         -> sourcemeta::blaze::TestTarget {
  ///       return {.document = schema, .frame = frame, .entrypoint = target};
  ///     },
  ///     sourcemeta::core::schema_resolver, sourcemeta::core::schema_walker,
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// const auto result{suite.run(
  ///     [](const sourcemeta::core::JSON::String &target,
  ///        std::size_t, std::size_t index, std::size_t total,
  ///        const sourcemeta::blaze::TestCase &test_case,
  ///        const sourcemeta::blaze::TestOutcome &outcome,
  ///        sourcemeta::blaze::TestTimestamp start,
  ///        sourcemeta::blaze::TestTimestamp end) {
  ///       std::cout << target << " " << index << "/" << total << ": "
  ///                 << test_case.description << " - "
  ///                 << (outcome.passed ? "PASS" : "FAIL")
  ///                 << "\n";
  ///     })};
  ///
  /// std::cout << result.passed << "/" << result.total << " passed\n";
  /// ```
  ///
  /// A target that names a schema inside a document that is not a schema
  /// itself is reached the same way, by handing over that document and a frame
  /// of the schemas it holds. Nothing else about it differs, as framing a
  /// document under the base it was read from keys a schema within it by that
  /// base and the place it sits in, which is what the target spells:
  ///
  /// ```cpp
  /// // A frame can neither be copied nor moved, and this one has to outlive
  /// // the suite, so the caller keeps it somewhere of its own and hands over
  /// // a reference to it
  /// const auto &frame{*frames.emplace_back(
  ///     std::make_unique<sourcemeta::core::OpenAPIFrame>(
  ///         description, walker, resolver, base))};
  /// return {.document = description, .frame = frame.schemas(),
  ///         .entrypoint = target};
  /// ```
  auto run(const Callback &callback) -> Result;

  /// Parse a test suite from a JSON object, resolving and compiling every
  /// target that it names. For example:
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
  /// #include <filesystem>
  /// #include <functional>
  ///
  /// const auto input{R"JSON({
  ///   "target": "https://example.com/string",
  ///   "tests": [
  ///     { "data": "foo", "valid": true },
  ///     { "data": [], "valid": false, "description": "Not a string" }
  ///   ]
  /// })JSON"};
  ///
  /// sourcemeta::core::PointerPositionTracker tracker;
  /// sourcemeta::core::JSON document{nullptr};
  /// sourcemeta::core::parse_json(input, document, std::ref(tracker));
  ///
  /// const auto schema{sourcemeta::core::parse_json(R"JSON({
  ///   "$schema": "https://json-schema.org/draft/2020-12/schema",
  ///   "$id": "https://example.com/string",
  ///   "type": "string"
  /// })JSON")};
  ///
  /// const sourcemeta::core::SchemaFrame frame{
  ///     sourcemeta::core::SchemaFrame::Mode::References, schema,
  ///     sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver};
  ///
  /// const auto suite{sourcemeta::blaze::TestSuite::parse(
  ///     document, tracker, std::filesystem::current_path(),
  ///     [&schema, &frame](const sourcemeta::core::JSON::String &target)
  ///         -> sourcemeta::blaze::TestTarget {
  ///       return {.document = schema, .frame = frame, .entrypoint = target};
  ///     },
  ///     sourcemeta::core::schema_resolver, sourcemeta::core::schema_walker,
  ///     sourcemeta::blaze::default_schema_compiler)};
  ///
  /// assert(suite.targets.size() == 1);
  /// assert(suite.targets.front() == "https://example.com/string");
  /// assert(suite.tests.size() == 2);
  /// ```
  ///
  /// The frame that a target resolves to must contain reference information
  /// for the document it frames, and that document must be bundled, which are
  /// the same pre-conditions that the overload of sourcemeta::blaze::compile
  /// taking a frame states. The frame must also locate the entry point, so a
  /// document is framed under the base that the target was resolved against
  /// rather than under no base at all
  static auto parse(const sourcemeta::core::JSON &document,
                    const sourcemeta::core::PointerPositionTracker &tracker,
                    const std::filesystem::path &base_path,
                    const TestTargetResolver &target_resolver,
                    const sourcemeta::core::SchemaResolver &schema_resolver,
                    const sourcemeta::core::SchemaWalker &walker,
                    const Compiler &compiler,
                    const std::optional<Tweaks> &tweaks = std::nullopt)
      -> TestSuite;

private:
// See
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4251?view=msvc-170
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  // What a target resolved to, held the way a container can hold it, as an
  // entry with a reference member cannot be assigned
  struct ResolvedTarget {
    const sourcemeta::core::JSON *document;
    const sourcemeta::core::SchemaFrame *frame;
    sourcemeta::core::JSON::String entrypoint;
  };

  [[nodiscard]] auto compile_target(std::size_t target_index, Mode mode) const
      -> Template;

  std::vector<ResolvedTarget> resolved_targets_;
  std::vector<Template> schemas_fast_;
  std::vector<std::optional<Template>> schemas_exhaustive_;
  sourcemeta::core::SchemaResolver schema_resolver_;
  sourcemeta::core::SchemaWalker walker_;
  Compiler compiler_;
  std::optional<Tweaks> tweaks_fast_;
  std::optional<Tweaks> tweaks_exhaustive_;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::blaze

#endif
