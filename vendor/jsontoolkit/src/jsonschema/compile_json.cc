#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert>     // assert
#include <functional>  // std::less
#include <map>         // std::map
#include <sstream>     // std::ostringstream
#include <string_view> // std::string_view
#include <type_traits> // std::is_same_v
#include <utility>     // std::move

namespace {

auto target_to_json(const sourcemeta::jsontoolkit::SchemaCompilerTarget &target)
    -> sourcemeta::jsontoolkit::JSON {
  using namespace sourcemeta::jsontoolkit;
  JSON result{JSON::make_object()};
  result.assign("category", JSON{"target"});
  result.assign("location", JSON{to_string(target.second)});
  switch (target.first) {
    case SchemaCompilerTargetType::Instance:
      result.assign("type", JSON{"instance"});
      return result;
    case SchemaCompilerTargetType::InstanceBasename:
      result.assign("type", JSON{"instance-basename"});
      return result;
    case SchemaCompilerTargetType::InstanceParent:
      result.assign("type", JSON{"instance-parent"});
      return result;
    case SchemaCompilerTargetType::AdjacentAnnotations:
      result.assign("type", JSON{"adjacent-annotations"});
      return result;
    case SchemaCompilerTargetType::ParentAdjacentAnnotations:
      result.assign("type", JSON{"parent-adjacent-annotations"});
      return result;
    case SchemaCompilerTargetType::ParentAnnotations:
      result.assign("type", JSON{"parent-annotations"});
      return result;
    case SchemaCompilerTargetType::Annotations:
      result.assign("type", JSON{"annotations"});
      return result;
    default:
      // We should never get here
      assert(false);
      return result;
  }
}

template <typename T>
auto value_to_json(const sourcemeta::jsontoolkit::SchemaCompilerStepValue<T>
                       &value) -> sourcemeta::jsontoolkit::JSON {
  using namespace sourcemeta::jsontoolkit;
  if (std::holds_alternative<SchemaCompilerTarget>(value)) {
    return target_to_json(std::get<SchemaCompilerTarget>(value));
  }

  assert(std::holds_alternative<T>(value));
  JSON result{JSON::make_object()};
  result.assign("category", JSON{"value"});
  if constexpr (std::is_same_v<SchemaCompilerValueJSON, T>) {
    result.assign("type", JSON{"json"});
    result.assign("value", JSON{std::get<T>(value)});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueBoolean, T>) {
    result.assign("type", JSON{"boolean"});
    result.assign("value", JSON{std::get<T>(value)});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueRegex, T>) {
    result.assign("type", JSON{"regex"});
    result.assign("value", JSON{std::get<T>(value).second});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueType, T>) {
    result.assign("type", JSON{"type"});
    std::ostringstream type_string;
    type_string << std::get<T>(value);
    result.assign("value", JSON{type_string.str()});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueTypes, T>) {
    result.assign("type", JSON{"types"});
    JSON types{JSON::make_array()};
    for (const auto type : std::get<T>(value)) {
      std::ostringstream type_string;
      type_string << type;
      types.push_back(JSON{type_string.str()});
    }

    result.assign("value", std::move(types));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueString, T>) {
    result.assign("type", JSON{"string"});
    result.assign("value", JSON{std::get<T>(value)});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueStrings, T>) {
    result.assign("type", JSON{"strings"});
    JSON items{JSON::make_array()};
    for (const auto &item : std::get<T>(value)) {
      items.push_back(JSON{item});
    }

    result.assign("value", std::move(items));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueArray, T>) {
    result.assign("type", JSON{"array"});
    JSON items{JSON::make_array()};
    for (const auto &item : std::get<T>(value)) {
      items.push_back(item);
    }

    result.assign("value", std::move(items));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueUnsignedInteger, T>) {
    result.assign("type", JSON{"unsigned-integer"});
    result.assign("value", JSON{std::get<T>(value)});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueRange, T>) {
    result.assign("type", JSON{"range"});
    JSON values{JSON::make_array()};
    const auto &range{std::get<T>(value)};
    values.push_back(JSON{std::get<0>(range)});
    values.push_back(std::get<1>(range).has_value()
                         ? JSON{std::get<1>(range).value()}
                         : JSON{nullptr});
    values.push_back(JSON{std::get<2>(range)});
    result.assign("value", std::move(values));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueStringType, T>) {
    result.assign("type", JSON{"string-type"});
    switch (std::get<T>(value)) {
      case SchemaCompilerValueStringType::URI:
        result.assign("value", JSON{"uri"});
        break;
      default:
        // We should never get here
        assert(false);
    }

    return result;
  } else {
    static_assert(std::is_same_v<SchemaCompilerValueNone, T>);
    return JSON{nullptr};
  }
}

template <typename T>
auto data_to_json(const T &data) -> sourcemeta::jsontoolkit::JSON {
  using namespace sourcemeta::jsontoolkit;
  JSON result{JSON::make_object()};
  result.assign("category", JSON{"data"});
  if constexpr (std::is_same_v<SchemaCompilerValueStrings, T>) {
    result.assign("type", JSON{"strings"});
    JSON items{JSON::make_array()};
    for (const auto &item : data) {
      items.push_back(JSON{item});
    }

    result.assign("value", std::move(items));
    return result;
  } else {
    // We should never get here
    assert(false);
    return JSON{nullptr};
  }
}

template <typename V>
auto step_to_json(
    const sourcemeta::jsontoolkit::SchemaCompilerTemplate::value_type &step)
    -> sourcemeta::jsontoolkit::JSON {
  static V visitor;
  return std::visit(visitor, step);
}

template <typename V, typename T>
auto encode_step(const std::string_view category, const std::string_view type,
                 const T &step) -> sourcemeta::jsontoolkit::JSON {
  using namespace sourcemeta::jsontoolkit;
  JSON result{JSON::make_object()};
  result.assign("category", JSON{category});
  result.assign("type", JSON{type});
  result.assign("relativeSchemaLocation",
                JSON{to_string(step.relative_schema_location)});
  result.assign("relativeInstanceLocation",
                JSON{to_string(step.relative_instance_location)});
  result.assign("absoluteKeywordLocation", JSON{step.keyword_location});
  result.assign("schemaResource", JSON{step.schema_resource});
  result.assign("dynamic", JSON{step.dynamic});

  if constexpr (requires { step.id; }) {
    result.assign("id", JSON{step.id});
  }

  if constexpr (requires { step.target; }) {
    result.assign("target", target_to_json(step.target));
  }

  if constexpr (requires { step.value; }) {
    result.assign("value", value_to_json(step.value));
  }

  if constexpr (requires { step.data; }) {
    result.assign("data", data_to_json(step.data));
  }

  if constexpr (requires { step.condition; }) {
    result.assign("condition", JSON::make_array());
    for (const auto &substep : step.condition) {
      result.at("condition").push_back(step_to_json<V>(substep));
    }
  }

  if constexpr (requires { step.children; }) {
    result.assign("children", JSON::make_array());
    for (const auto &child : step.children) {
      result.at("children").push_back(step_to_json<V>(child));
    }
  }

  return result;
}

struct StepVisitor {
#define HANDLE_STEP(category, type, name)                                      \
  auto operator()(const sourcemeta::jsontoolkit::name &step)                   \
      const->sourcemeta::jsontoolkit::JSON {                                   \
    return encode_step<StepVisitor>(category, type, step);                     \
  }

  HANDLE_STEP("assertion", "fail", SchemaCompilerAssertionFail)
  HANDLE_STEP("assertion", "defines", SchemaCompilerAssertionDefines)
  HANDLE_STEP("assertion", "defines-all", SchemaCompilerAssertionDefinesAll)
  HANDLE_STEP("assertion", "type", SchemaCompilerAssertionType)
  HANDLE_STEP("assertion", "type-any", SchemaCompilerAssertionTypeAny)
  HANDLE_STEP("assertion", "type-strict", SchemaCompilerAssertionTypeStrict)
  HANDLE_STEP("assertion", "type-strict-any",
              SchemaCompilerAssertionTypeStrictAny)
  HANDLE_STEP("assertion", "regex", SchemaCompilerAssertionRegex)
  HANDLE_STEP("assertion", "size-greater", SchemaCompilerAssertionSizeGreater)
  HANDLE_STEP("assertion", "size-less", SchemaCompilerAssertionSizeLess)
  HANDLE_STEP("assertion", "equal", SchemaCompilerAssertionEqual)
  HANDLE_STEP("assertion", "greater-equal", SchemaCompilerAssertionGreaterEqual)
  HANDLE_STEP("assertion", "less-equal", SchemaCompilerAssertionLessEqual)
  HANDLE_STEP("assertion", "greater", SchemaCompilerAssertionGreater)
  HANDLE_STEP("assertion", "less", SchemaCompilerAssertionLess)
  HANDLE_STEP("assertion", "unique", SchemaCompilerAssertionUnique)
  HANDLE_STEP("assertion", "divisible", SchemaCompilerAssertionDivisible)
  HANDLE_STEP("assertion", "string-type", SchemaCompilerAssertionStringType)
  HANDLE_STEP("assertion", "equals-any", SchemaCompilerAssertionEqualsAny)
  HANDLE_STEP("annotation", "public", SchemaCompilerAnnotationPublic)
  HANDLE_STEP("logical", "or", SchemaCompilerLogicalOr)
  HANDLE_STEP("logical", "and", SchemaCompilerLogicalAnd)
  HANDLE_STEP("logical", "xor", SchemaCompilerLogicalXor)
  HANDLE_STEP("logical", "try", SchemaCompilerLogicalTry)
  HANDLE_STEP("logical", "not", SchemaCompilerLogicalNot)
  HANDLE_STEP("internal", "size-equal", SchemaCompilerInternalSizeEqual)
  HANDLE_STEP("internal", "annotation", SchemaCompilerInternalAnnotation)
  HANDLE_STEP("internal", "no-adjacent-annotation",
              SchemaCompilerInternalNoAdjacentAnnotation)
  HANDLE_STEP("internal", "no-annotation", SchemaCompilerInternalNoAnnotation)
  HANDLE_STEP("internal", "container", SchemaCompilerInternalContainer)
  HANDLE_STEP("internal", "defines-all", SchemaCompilerInternalDefinesAll)
  HANDLE_STEP("loop", "properties", SchemaCompilerLoopProperties)
  HANDLE_STEP("loop", "keys", SchemaCompilerLoopKeys)
  HANDLE_STEP("loop", "items", SchemaCompilerLoopItems)
  HANDLE_STEP("loop", "items-from-annotation-index",
              SchemaCompilerLoopItemsFromAnnotationIndex)
  HANDLE_STEP("loop", "contains", SchemaCompilerLoopContains)
  HANDLE_STEP("control", "label", SchemaCompilerControlLabel)
  HANDLE_STEP("control", "mark", SchemaCompilerControlMark)
  HANDLE_STEP("control", "jump", SchemaCompilerControlJump)
  HANDLE_STEP("control", "dynamic-anchor-jump",
              SchemaCompilerControlDynamicAnchorJump)

#undef HANDLE_STEP
};

} // namespace

namespace sourcemeta::jsontoolkit {

auto to_json(const SchemaCompilerTemplate &steps) -> JSON {
  JSON result{JSON::make_array()};
  for (const auto &step : steps) {
    result.push_back(step_to_json<StepVisitor>(step));
  }

  return result;
}

auto compiler_template_format_compare(const JSON::String &left,
                                      const JSON::String &right) -> bool {
  using Rank =
      std::map<JSON::String, std::uint64_t, std::less<JSON::String>,
               JSON::Allocator<std::pair<const JSON::String, std::uint64_t>>>;
  static Rank rank{{"category", 0},
                   {"type", 1},
                   {"value", 2},
                   {"schemaResource", 3},
                   {"absoluteKeywordLocation", 4},
                   {"relativeSchemaLocation", 5},
                   {"relativeInstanceLocation", 6},
                   {"target", 7},
                   {"location", 8},
                   {"id", 9},
                   {"dynamic", 10},
                   {"condition", 11},
                   {"children", 12}};

  // We define and control all of these keywords, so if we are missing
  // some here, then we did something wrong?
  assert(rank.contains(left));
  assert(rank.contains(right));

  return rank.at(left) < rank.at(right);
}

} // namespace sourcemeta::jsontoolkit
