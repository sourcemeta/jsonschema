#include <sourcemeta/blaze/test.h>

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/uri.h>
#include <sourcemeta/core/yaml.h>

#include <cassert>     // assert
#include <string_view> // std::string_view
#include <tuple>       // std::get
#include <utility>     // std::move

namespace {
inline auto TEST_ERROR_IF(
    bool condition, const sourcemeta::core::PointerPositionTracker &tracker,
    const sourcemeta::core::Pointer &pointer, const char *message) -> void {
  if (condition) [[unlikely]] {
    const auto position{tracker.get(pointer)};
    assert(position.has_value());
    throw sourcemeta::blaze::TestParseError{message, pointer,
                                            std::get<0>(position.value()),
                                            std::get<1>(position.value())};
  }
}
} // namespace

namespace sourcemeta::blaze {

auto TestCase::parse(
    const sourcemeta::core::JSON &test_case_json,
    const sourcemeta::core::PointerPositionTracker &tracker,
    const std::filesystem::path &base_path,
    const sourcemeta::core::Pointer &location,
    const sourcemeta::core::PointerPositionTracker::Position &position)
    -> TestCase {
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

  sourcemeta::core::JSON::String description;
  if (test_case_json.defines("description")) {
    description = test_case_json.at("description").to_string();
  }

  sourcemeta::core::PointerPositionTracker data_tracker;

  if (test_case_json.defines("data")) {
    return TestCase{.description = std::move(description),
                    .valid = test_case_json.at("valid").to_boolean(),
                    .data = test_case_json.at("data"),
                    .tracker = std::move(data_tracker),
                    .position = position};
  } else {
    const std::filesystem::path data_path{sourcemeta::core::weakly_canonical(
        base_path / test_case_json.at("dataPath").to_string())};
    sourcemeta::core::JSON data{nullptr};
    sourcemeta::core::read_yaml_or_json(data_path, data,
                                        std::ref(data_tracker));
    return TestCase{.description = std::move(description),
                    .valid = test_case_json.at("valid").to_boolean(),
                    .data = std::move(data),
                    .tracker = std::move(data_tracker),
                    .position = position};
  }
}

auto TestSuite::parse(const sourcemeta::core::JSON &document,
                      const sourcemeta::core::PointerPositionTracker &tracker,
                      const std::filesystem::path &base_path,
                      const sourcemeta::core::SchemaResolver &schema_resolver,
                      const sourcemeta::core::SchemaWalker &walker,
                      const Compiler &compiler,
                      const std::string_view default_dialect,
                      const std::string_view default_id,
                      const std::optional<Tweaks> &tweaks) -> TestSuite {
  assert(std::filesystem::is_directory(base_path));
  TEST_ERROR_IF(!document.is_object(), tracker, sourcemeta::core::empty_pointer,
                "The test document must be an object");
  TEST_ERROR_IF(!document.defines("target"), tracker,
                sourcemeta::core::empty_pointer,
                "The test document must contain a `target` property");
  TEST_ERROR_IF(!document.at("target").is_string() &&
                    !document.at("target").is_array(),
                tracker, sourcemeta::core::Pointer{"target"},
                "The test document `target` property must be a URI or an "
                "array of URIs");
  TEST_ERROR_IF(!document.defines("tests"), tracker,
                sourcemeta::core::empty_pointer,
                "The test document must contain a `tests` property");
  TEST_ERROR_IF(!document.at("tests").is_array(), tracker,
                sourcemeta::core::Pointer{"tests"},
                "The test document `tests` property must be an array");

  const auto base_path_uri{
      sourcemeta::core::URI::from_path(base_path / "test.json")};

  TestSuite test_suite;

  if (document.at("target").is_string()) {
    sourcemeta::core::URI schema_uri{document.at("target").to_string()};
    schema_uri.resolve_from(base_path_uri);
    schema_uri.canonicalize();
    test_suite.targets.push_back(schema_uri.recompose());
  } else {
    TEST_ERROR_IF(document.at("target").empty(), tracker,
                  sourcemeta::core::Pointer{"target"},
                  "The test document `target` array must contain at least "
                  "one URI");
    // TODO(C++23): Use std::views::enumerate when available in libc++
    std::size_t target_index{0};
    for (const auto &target_entry : document.at("target").as_array()) {
      const sourcemeta::core::Pointer target_location{"target", target_index};
      TEST_ERROR_IF(!target_entry.is_string(), tracker, target_location,
                    "Each entry in the test document `target` array must be "
                    "a URI");
      sourcemeta::core::URI schema_uri{target_entry.to_string()};
      schema_uri.resolve_from(base_path_uri);
      schema_uri.canonicalize();
      test_suite.targets.push_back(schema_uri.recompose());
      target_index += 1;
    }
  }

  // TODO(C++23): Use std::views::enumerate when available in libc++
  std::size_t index{0};
  for (const auto &test_case_json : document.at("tests").as_array()) {
    const sourcemeta::core::Pointer location{"tests", index};
    const auto position{tracker.get(location)};
    assert(position.has_value());
    test_suite.tests.push_back(TestCase::parse(
        test_case_json, tracker, base_path, location, position.value()));
    index += 1;
  }

  test_suite.schemas_fast.reserve(test_suite.targets.size());
  test_suite.schemas_exhaustive.reserve(test_suite.targets.size());

  for (const auto &target : test_suite.targets) {
    const auto target_schema{sourcemeta::core::wrap(target)};

    try {
      test_suite.schemas_fast.push_back(compile(
          target_schema, walker, schema_resolver, compiler,
          Mode::FastValidation, default_dialect, default_id, "", tweaks));
      test_suite.schemas_exhaustive.push_back(
          compile(target_schema, walker, schema_resolver, compiler,
                  Mode::Exhaustive, default_dialect, default_id, "", tweaks));
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      if (error.location() == sourcemeta::core::Pointer{"$ref"} &&
          error.identifier() == target) {
        throw sourcemeta::core::SchemaResolutionError{
            target, "Could not resolve schema under test"};
      }

      throw;
    }
  }

  return test_suite;
}

} // namespace sourcemeta::blaze
