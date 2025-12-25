#ifndef SOURCEMETA_JSONSCHEMA_CLI_TEST_H_
#define SOURCEMETA_JSONSCHEMA_CLI_TEST_H_

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <cassert>    // assert
#include <cstdint>    // std::uint64_t
#include <filesystem> // std::filesystem
#include <functional> // std::function
#include <optional>   // std::optional
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string
#include <tuple>      // std::get
#include <vector>     // std::vector

namespace sourcemeta::jsonschema {

class TestParseError : public std::runtime_error {
public:
  TestParseError(std::string message, sourcemeta::core::Pointer location,
                 std::uint64_t line, std::uint64_t column)
      : std::runtime_error{std::move(message)}, location_{std::move(location)},
        line_{line}, column_{column} {}

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    return this->line_;
  }

  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  sourcemeta::core::Pointer location_;
  std::uint64_t line_;
  std::uint64_t column_;
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST_ERROR_IF(condition, tracker, pointer, message)                    \
  if (condition) {                                                             \
    const auto position{(tracker).get(pointer)};                               \
    assert(position.has_value());                                              \
    throw sourcemeta::jsonschema::TestParseError{                              \
        message, pointer, std::get<0>(position.value()),                       \
        std::get<1>(position.value())};                                        \
  }

struct TestCase {
  std::string description;
  bool valid;
  sourcemeta::core::JSON data;
  sourcemeta::core::PointerPositionTracker tracker;

  static auto parse(const sourcemeta::core::JSON &test_case_json,
                    const sourcemeta::core::PointerPositionTracker &tracker,
                    const std::filesystem::path &base_path,
                    const sourcemeta::core::Pointer &location) -> TestCase {
    TEST_ERROR_IF(!test_case_json.is_object(), tracker, location,
                  "Test case documents must be objects");
    TEST_ERROR_IF(!test_case_json.defines("data") &&
                      !test_case_json.defines("dataPath"),
                  tracker, location,
                  "Test case documents must contain a `data` or `dataPath` "
                  "property");
    TEST_ERROR_IF(test_case_json.defines("data") &&
                      test_case_json.defines("dataPath"),
                  tracker, location,
                  "Test case documents must contain either a `data` or "
                  "`dataPath` property, but not both");
    TEST_ERROR_IF(test_case_json.defines("dataPath") &&
                      !test_case_json.at("dataPath").is_string(),
                  tracker, location.concat({"dataPath"}),
                  "Test case documents must set the `dataPath` property to a "
                  "string");
    TEST_ERROR_IF(test_case_json.defines("description") &&
                      !test_case_json.at("description").is_string(),
                  tracker, location.concat({"description"}),
                  "If you set a test case description, it must be a string");
    TEST_ERROR_IF(!test_case_json.defines("valid"), tracker, location,
                  "Test case documents must contain a `valid` property");
    TEST_ERROR_IF(!test_case_json.at("valid").is_boolean(), tracker,
                  location.concat({"valid"}),
                  "The test case document `valid` property must be a boolean");

    std::string description;
    if (test_case_json.defines("description")) {
      description = test_case_json.at("description").to_string();
    }

    sourcemeta::core::PointerPositionTracker data_tracker;

    if (test_case_json.defines("data")) {
      return TestCase{std::move(description),
                      test_case_json.at("valid").to_boolean(),
                      test_case_json.at("data"), std::move(data_tracker)};
    } else {
      const std::filesystem::path data_path{sourcemeta::core::weakly_canonical(
          base_path / test_case_json.at("dataPath").to_string())};
      auto data{sourcemeta::core::read_yaml_or_json(data_path,
                                                    std::ref(data_tracker))};
      return TestCase{std::move(description),
                      test_case_json.at("valid").to_boolean(), std::move(data),
                      std::move(data_tracker)};
    }
  }
};

struct TestSuite {
  struct Result {
    std::size_t total;
    std::size_t passed;
  };

  std::string target;
  std::vector<TestCase> tests;
  sourcemeta::blaze::Template schema_fast;
  sourcemeta::blaze::Template schema_exhaustive;
  sourcemeta::blaze::Evaluator evaluator;

  using Callback = std::function<void(std::size_t index, std::size_t total,
                                      const TestCase &test_case, bool actual)>;

  auto run(Callback callback) -> Result {
    Result result{this->tests.size(), 0};
    for (std::size_t index = 0; index < this->tests.size(); ++index) {
      const auto &test_case = this->tests[index];
      const auto actual{
          this->evaluator.validate(this->schema_fast, test_case.data)};
      callback(index + 1, this->tests.size(), test_case, actual);
      if (test_case.valid == actual) {
        result.passed += 1;
      }
    }

    return result;
  }

  static auto
  parse(const sourcemeta::core::JSON &document,
        const sourcemeta::core::PointerPositionTracker &tracker,
        const std::filesystem::path &base_path,
        const sourcemeta::core::SchemaResolver &schema_resolver,
        const sourcemeta::core::SchemaWalker &walker,
        const sourcemeta::blaze::Compiler &compiler,
        const std::optional<std::string> &default_dialect = std::nullopt,
        const std::optional<std::string> &default_id = std::nullopt,
        const std::optional<sourcemeta::blaze::Tweaks> &tweaks = std::nullopt)
      -> TestSuite {
    assert(std::filesystem::is_directory(base_path));
    TEST_ERROR_IF(!document.is_object(), tracker,
                  sourcemeta::core::empty_pointer,
                  "The test document must be an object");
    TEST_ERROR_IF(!document.defines("target"), tracker,
                  sourcemeta::core::empty_pointer,
                  "The test document must contain a `target` property");
    TEST_ERROR_IF(!document.at("target").is_string(), tracker,
                  sourcemeta::core::Pointer{"target"},
                  "The test document `target` property must be a URI");
    TEST_ERROR_IF(!document.defines("tests"), tracker,
                  sourcemeta::core::empty_pointer,
                  "The test document must contain a `tests` property");
    TEST_ERROR_IF(!document.at("tests").is_array(), tracker,
                  sourcemeta::core::Pointer{"tests"},
                  "The test document `tests` property must be an array");

    // Resolve target schema URI
    // Append a dummy segment so relative URI resolution works correctly
    const auto base_path_uri{
        sourcemeta::core::URI::from_path(base_path / "test.json")};
    sourcemeta::core::URI schema_uri{document.at("target").to_string()};
    schema_uri.resolve_from(base_path_uri);
    schema_uri.canonicalize();

    TestSuite test_suite;
    test_suite.target = schema_uri.recompose();

    std::size_t index{0};
    for (const auto &test_case_json : document.at("tests").as_array()) {
      test_suite.tests.push_back(
          TestCase::parse(test_case_json, tracker, base_path,
                          sourcemeta::core::Pointer{"tests", index}));
      index += 1;
    }

    // Compile schema
    const auto target_schema{sourcemeta::core::wrap(test_suite.target)};

    try {
      test_suite.schema_fast = sourcemeta::blaze::compile(
          target_schema, walker, schema_resolver, compiler,
          sourcemeta::blaze::Mode::FastValidation, default_dialect, default_id,
          tweaks);
      test_suite.schema_exhaustive = sourcemeta::blaze::compile(
          target_schema, walker, schema_resolver, compiler,
          sourcemeta::blaze::Mode::Exhaustive, default_dialect, default_id,
          tweaks);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      if (error.location() == sourcemeta::core::Pointer{"$ref"} &&
          error.identifier() == test_suite.target) {
        throw sourcemeta::core::SchemaResolutionError{
            test_suite.target, "Could not resolve schema under test"};
      }

      throw;
    }

    return test_suite;
  }
};

} // namespace sourcemeta::jsonschema

#endif
