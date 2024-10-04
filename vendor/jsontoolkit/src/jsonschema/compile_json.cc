#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <cassert>     // assert
#include <functional>  // std::less
#include <map>         // std::map
#include <sstream>     // std::ostringstream
#include <string_view> // std::string_view
#include <type_traits> // std::is_same_v
#include <utility>     // std::move

namespace {

template <typename T>
auto value_to_json(const T &value) -> sourcemeta::jsontoolkit::JSON {
  using namespace sourcemeta::jsontoolkit;
  JSON result{JSON::make_object()};
  result.assign("category", JSON{"value"});
  if constexpr (std::is_same_v<SchemaCompilerValueJSON, T>) {
    result.assign("type", JSON{"json"});
    result.assign("value", JSON{value});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueBoolean, T>) {
    result.assign("type", JSON{"boolean"});
    result.assign("value", JSON{value});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueRegex, T>) {
    result.assign("type", JSON{"regex"});
    result.assign("value", JSON{value.second});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueType, T>) {
    result.assign("type", JSON{"type"});
    std::ostringstream type_string;
    type_string << value;
    result.assign("value", JSON{type_string.str()});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueTypes, T>) {
    result.assign("type", JSON{"types"});
    JSON types{JSON::make_array()};
    for (const auto type : value) {
      std::ostringstream type_string;
      type_string << type;
      types.push_back(JSON{type_string.str()});
    }

    result.assign("value", std::move(types));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueString, T>) {
    result.assign("type", JSON{"string"});
    result.assign("value", JSON{value});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueStrings, T>) {
    result.assign("type", JSON{"strings"});
    JSON items{JSON::make_array()};
    for (const auto &item : value) {
      items.push_back(JSON{item});
    }

    result.assign("value", std::move(items));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueArray, T>) {
    result.assign("type", JSON{"array"});
    JSON items{JSON::make_array()};
    for (const auto &item : value) {
      items.push_back(item);
    }

    result.assign("value", std::move(items));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueUnsignedInteger, T>) {
    result.assign("type", JSON{"unsigned-integer"});
    result.assign("value", JSON{value});
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueRange, T>) {
    result.assign("type", JSON{"range"});
    JSON values{JSON::make_array()};
    const auto &range{value};
    values.push_back(JSON{std::get<0>(range)});
    values.push_back(std::get<1>(range).has_value()
                         ? JSON{std::get<1>(range).value()}
                         : JSON{nullptr});
    values.push_back(JSON{std::get<2>(range)});
    result.assign("value", std::move(values));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueNamedIndexes, T>) {
    result.assign("type", JSON{"named-indexes"});
    JSON values{JSON::make_object()};
    for (const auto &[name, index] : value) {
      values.assign(name, JSON{index});
    }

    result.assign("value", std::move(values));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueStringMap, T>) {
    result.assign("type", JSON{"string-map"});
    JSON map{JSON::make_object()};
    for (const auto &[string, strings] : value) {
      JSON dependencies{JSON::make_array()};
      for (const auto &substring : strings) {
        dependencies.push_back(JSON{substring});
      }

      map.assign(string, std::move(dependencies));
    }

    result.assign("value", std::move(map));
    return result;
  } else if constexpr (std::is_same_v<
                           SchemaCompilerValueItemsAnnotationKeywords, T>) {
    result.assign("type", JSON{"items-annotation-keywords"});
    JSON data{JSON::make_object()};
    data.assign("index", JSON{value.index});

    JSON mask{JSON::make_array()};
    for (const auto &keyword : value.mask) {
      mask.push_back(JSON{keyword});
    }
    data.assign("mask", std::move(mask));

    JSON filter{JSON::make_array()};
    for (const auto &keyword : value.filter) {
      filter.push_back(JSON{keyword});
    }
    data.assign("filter", std::move(filter));

    result.assign("value", std::move(data));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueIndexedJSON, T>) {
    result.assign("type", JSON{"indexed-json"});
    JSON data{JSON::make_object()};
    data.assign("index", JSON{value.first});
    data.assign("value", value.second);
    result.assign("value", std::move(data));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValuePropertyFilter, T>) {
    result.assign("type", JSON{"property-filter"});
    JSON data{JSON::make_object()};
    data.assign("names", JSON::make_array());
    data.assign("patterns", JSON::make_array());

    for (const auto &name : value.first) {
      data.at("names").push_back(JSON{name});
    }

    for (const auto &pattern : value.second) {
      data.at("patterns").push_back(JSON{pattern.second});
    }

    result.assign("value", std::move(data));
    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueStringType, T>) {
    result.assign("type", JSON{"string-type"});
    switch (value) {
      case SchemaCompilerValueStringType::URI:
        result.assign("value", JSON{"uri"});
        break;
      default:
        // We should never get here
        assert(false);
    }

    return result;
  } else if constexpr (std::is_same_v<SchemaCompilerValueIndexPair, T>) {
    result.assign("type", JSON{"index-pair"});
    JSON data{JSON::make_array()};
    data.push_back(JSON{value.first});
    data.push_back(JSON{value.second});
    result.assign("value", std::move(data));
    return result;
  } else {
    static_assert(std::is_same_v<SchemaCompilerValueNone, T>);
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
  result.assign("report", JSON{step.report});
  result.assign("value", value_to_json(step.value));

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
  HANDLE_STEP("assertion", "property-dependencies",
              SchemaCompilerAssertionPropertyDependencies)
  HANDLE_STEP("assertion", "type", SchemaCompilerAssertionType)
  HANDLE_STEP("assertion", "type-any", SchemaCompilerAssertionTypeAny)
  HANDLE_STEP("assertion", "type-strict", SchemaCompilerAssertionTypeStrict)
  HANDLE_STEP("assertion", "type-strict-any",
              SchemaCompilerAssertionTypeStrictAny)
  HANDLE_STEP("assertion", "type-string-bounded",
              SchemaCompilerAssertionTypeStringBounded)
  HANDLE_STEP("assertion", "type-array-bounded",
              SchemaCompilerAssertionTypeArrayBounded)
  HANDLE_STEP("assertion", "type-object-bounded",
              SchemaCompilerAssertionTypeObjectBounded)
  HANDLE_STEP("assertion", "regex", SchemaCompilerAssertionRegex)
  HANDLE_STEP("assertion", "string-size-less",
              SchemaCompilerAssertionStringSizeLess)
  HANDLE_STEP("assertion", "string-size-greater",
              SchemaCompilerAssertionStringSizeGreater)
  HANDLE_STEP("assertion", "array-size-less",
              SchemaCompilerAssertionArraySizeLess)
  HANDLE_STEP("assertion", "array-size-greater",
              SchemaCompilerAssertionArraySizeGreater)
  HANDLE_STEP("assertion", "object-size-less",
              SchemaCompilerAssertionObjectSizeLess)
  HANDLE_STEP("assertion", "object-size-greater",
              SchemaCompilerAssertionObjectSizeGreater)
  HANDLE_STEP("assertion", "equal", SchemaCompilerAssertionEqual)
  HANDLE_STEP("assertion", "greater-equal", SchemaCompilerAssertionGreaterEqual)
  HANDLE_STEP("assertion", "less-equal", SchemaCompilerAssertionLessEqual)
  HANDLE_STEP("assertion", "greater", SchemaCompilerAssertionGreater)
  HANDLE_STEP("assertion", "less", SchemaCompilerAssertionLess)
  HANDLE_STEP("assertion", "unique", SchemaCompilerAssertionUnique)
  HANDLE_STEP("assertion", "divisible", SchemaCompilerAssertionDivisible)
  HANDLE_STEP("assertion", "string-type", SchemaCompilerAssertionStringType)
  HANDLE_STEP("assertion", "property-type", SchemaCompilerAssertionPropertyType)
  HANDLE_STEP("assertion", "property-type-strict",
              SchemaCompilerAssertionPropertyTypeStrict)
  HANDLE_STEP("assertion", "equals-any", SchemaCompilerAssertionEqualsAny)
  HANDLE_STEP("annotation", "emit", SchemaCompilerAnnotationEmit)
  HANDLE_STEP("annotation", "when-array-size-equal",
              SchemaCompilerAnnotationWhenArraySizeEqual)
  HANDLE_STEP("annotation", "when-array-size-greater",
              SchemaCompilerAnnotationWhenArraySizeGreater)
  HANDLE_STEP("annotation", "to-parent", SchemaCompilerAnnotationToParent)
  HANDLE_STEP("annotation", "basename-to-parent",
              SchemaCompilerAnnotationBasenameToParent)
  HANDLE_STEP("logical", "or", SchemaCompilerLogicalOr)
  HANDLE_STEP("logical", "and", SchemaCompilerLogicalAnd)
  HANDLE_STEP("logical", "xor", SchemaCompilerLogicalXor)
  HANDLE_STEP("logical", "condition", SchemaCompilerLogicalCondition)
  HANDLE_STEP("logical", "not", SchemaCompilerLogicalNot)
  HANDLE_STEP("logical", "when-type", SchemaCompilerLogicalWhenType)
  HANDLE_STEP("logical", "when-defines", SchemaCompilerLogicalWhenDefines)
  HANDLE_STEP("logical", "when-array-size-greater",
              SchemaCompilerLogicalWhenArraySizeGreater)
  HANDLE_STEP("logical", "when-array-size-equal",
              SchemaCompilerLogicalWhenArraySizeEqual)
  HANDLE_STEP("loop", "properties-match", SchemaCompilerLoopPropertiesMatch)
  HANDLE_STEP("loop", "properties", SchemaCompilerLoopProperties)
  HANDLE_STEP("loop", "properties-regex", SchemaCompilerLoopPropertiesRegex)
  HANDLE_STEP("loop", "properties-no-annotation",
              SchemaCompilerLoopPropertiesNoAnnotation)
  HANDLE_STEP("loop", "properties-except", SchemaCompilerLoopPropertiesExcept)
  HANDLE_STEP("loop", "properties-type", SchemaCompilerLoopPropertiesType)
  HANDLE_STEP("loop", "properties-type-strict",
              SchemaCompilerLoopPropertiesTypeStrict)
  HANDLE_STEP("loop", "keys", SchemaCompilerLoopKeys)
  HANDLE_STEP("loop", "items", SchemaCompilerLoopItems)
  HANDLE_STEP("loop", "items-unmarked", SchemaCompilerLoopItemsUnmarked)
  HANDLE_STEP("loop", "items-unevaluated", SchemaCompilerLoopItemsUnevaluated)
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
                   {"report", 7},
                   {"dynamic", 8},
                   {"children", 9}};

  constexpr std::uint64_t DEFAULT_RANK{999};
  const auto left_rank{rank.contains(left) ? rank.at(left) : DEFAULT_RANK};
  const auto right_rank{rank.contains(right) ? rank.at(right) : DEFAULT_RANK};
  return left_rank < right_rank;
}

} // namespace sourcemeta::jsontoolkit
