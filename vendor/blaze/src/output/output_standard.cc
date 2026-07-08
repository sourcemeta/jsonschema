#include <sourcemeta/blaze/output_simple.h>
#include <sourcemeta/blaze/output_standard.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cassert>    // assert
#include <functional> // std::ref, std::reference_wrapper
#include <map>        // std::map
#include <string>     // std::string
#include <tuple>      // std::tie
#include <vector>     // std::vector

namespace sourcemeta::blaze {

namespace {

// NOLINTNEXTLINE(bugprone-exception-escape)
struct AnnotationLocation {
  auto operator<(const AnnotationLocation &other) const noexcept -> bool {
    return std::tie(this->instance_location, this->evaluate_path,
                    this->schema_location.get()) <
           std::tie(other.instance_location, other.evaluate_path,
                    other.schema_location.get());
  }

  sourcemeta::core::WeakPointer instance_location;
  sourcemeta::core::WeakPointer evaluate_path;
  std::reference_wrapper<const std::string> schema_location;
};

auto group_annotations(const SimpleOutput &output)
    -> std::map<AnnotationLocation, std::vector<sourcemeta::core::JSON>> {
  std::map<AnnotationLocation, std::vector<sourcemeta::core::JSON>> result;
  for (const auto &entry : output.annotations()) {
    auto &values{result[{.instance_location = entry.instance_location,
                         .evaluate_path = entry.evaluate_path,
                         .schema_location = entry.schema_location}]};
    if (values.empty() || values.back() != entry.value) {
      values.push_back(entry.value);
    }
  }

  return result;
}

auto handle_standard(Evaluator &evaluator, const Template &schema,
                     const sourcemeta::core::JSON &instance,
                     const StandardOutput format,
                     const sourcemeta::core::PointerPositionTracker *tracker)
    -> sourcemeta::core::JSON {
  // We avoid a callback for this specific case for performance reasons
  if (format == StandardOutput::Flag) {
    auto result{sourcemeta::core::JSON::make_object()};
    const auto valid{evaluator.validate(schema, instance)};
    result.assign_assume_new("valid", sourcemeta::core::JSON{valid});
    return result;
  } else {
    assert(format == StandardOutput::Basic);
    SimpleOutput output{instance};
    const auto valid{evaluator.validate(schema, instance, std::ref(output))};

    if (valid) {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign_assume_new("valid", sourcemeta::core::JSON{valid});
      auto annotations{sourcemeta::core::JSON::make_array()};
      for (const auto &annotation : group_annotations(output)) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign_assume_new(
            "keywordLocation",
            sourcemeta::core::JSON{
                sourcemeta::core::to_string(annotation.first.evaluate_path)});
        unit.assign_assume_new(
            "absoluteKeywordLocation",
            sourcemeta::core::JSON{annotation.first.schema_location});
        unit.assign_assume_new(
            "instanceLocation",
            sourcemeta::core::JSON{sourcemeta::core::to_string(
                annotation.first.instance_location)});

        if (tracker != nullptr) {
          const auto position{tracker->get(sourcemeta::core::to_pointer(
              annotation.first.instance_location))};
          if (position.has_value()) {
            unit.assign_assume_new("instancePosition",
                                   sourcemeta::core::to_json(position.value()));
          }
        }

        unit.assign_assume_new("annotation",
                               sourcemeta::core::to_json(annotation.second));
        annotations.push_back(std::move(unit));
      }

      if (!annotations.empty()) {
        result.assign_assume_new("annotations", std::move(annotations));
      }

      return result;
    } else {
      auto result{sourcemeta::core::JSON::make_object()};
      result.assign_assume_new("valid", sourcemeta::core::JSON{valid});
      auto errors{sourcemeta::core::JSON::make_array()};
      for (const auto &entry : output) {
        auto unit{sourcemeta::core::JSON::make_object()};
        unit.assign_assume_new(
            "keywordLocation",
            sourcemeta::core::JSON{
                sourcemeta::core::to_string(entry.evaluate_path)});
        unit.assign_assume_new("absoluteKeywordLocation",
                               sourcemeta::core::JSON{entry.schema_location});
        unit.assign_assume_new(
            "instanceLocation",
            sourcemeta::core::JSON{
                sourcemeta::core::to_string(entry.instance_location)});

        if (tracker != nullptr) {
          const auto position{tracker->get(
              sourcemeta::core::to_pointer(entry.instance_location))};
          if (position.has_value()) {
            unit.assign_assume_new("instancePosition",
                                   sourcemeta::core::to_json(position.value()));
          }
        }

        unit.assign_assume_new("error", sourcemeta::core::JSON{entry.message});
        errors.push_back(std::move(unit));
      }

      assert(!errors.empty());
      result.assign_assume_new("errors", std::move(errors));
      return result;
    }
  }
}

} // namespace

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format) -> sourcemeta::core::JSON {
  return handle_standard(evaluator, schema, instance, format, nullptr);
}

auto standard(Evaluator &evaluator, const Template &schema,
              const sourcemeta::core::JSON &instance,
              const StandardOutput format,
              const sourcemeta::core::PointerPositionTracker &instanceTracker)
    -> sourcemeta::core::JSON {
  return handle_standard(evaluator, schema, instance, format, &instanceTracker);
}

} // namespace sourcemeta::blaze
