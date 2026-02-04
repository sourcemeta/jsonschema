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
  if (condition) {
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
    auto data{
        sourcemeta::core::read_yaml_or_json(data_path, std::ref(data_tracker))};
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
  TEST_ERROR_IF(!document.at("target").is_string(), tracker,
                sourcemeta::core::Pointer{"target"},
                "The test document `target` property must be a URI");
  TEST_ERROR_IF(!document.defines("tests"), tracker,
                sourcemeta::core::empty_pointer,
                "The test document must contain a `tests` property");
  TEST_ERROR_IF(!document.at("tests").is_array(), tracker,
                sourcemeta::core::Pointer{"tests"},
                "The test document `tests` property must be an array");

  const auto base_path_uri{
      sourcemeta::core::URI::from_path(base_path / "test.json")};
  sourcemeta::core::URI schema_uri{document.at("target").to_string()};
  schema_uri.resolve_from(base_path_uri);
  schema_uri.canonicalize();

  TestSuite test_suite;
  test_suite.target = schema_uri.recompose();

  std::size_t index{0};
  for (const auto &test_case_json : document.at("tests").as_array()) {
    const sourcemeta::core::Pointer location{"tests", index};
    const auto position{tracker.get(location)};
    assert(position.has_value());
    test_suite.tests.push_back(TestCase::parse(
        test_case_json, tracker, base_path, location, position.value()));
    index += 1;
  }

  const auto target_schema{sourcemeta::core::wrap(test_suite.target)};

  try {
    test_suite.schema_fast =
        compile(target_schema, walker, schema_resolver, compiler,
                Mode::FastValidation, default_dialect, default_id, "", tweaks);
    test_suite.schema_exhaustive =
        compile(target_schema, walker, schema_resolver, compiler,
                Mode::Exhaustive, default_dialect, default_id, "", tweaks);
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

} // namespace sourcemeta::blaze
