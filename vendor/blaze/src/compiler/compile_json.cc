#include <sourcemeta/blaze/compiler.h>

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
  using namespace sourcemeta::blaze;
  sourcemeta::jsontoolkit::JSON result{
      sourcemeta::jsontoolkit::JSON::make_object()};
  result.assign("category", sourcemeta::jsontoolkit::JSON{"value"});
  if constexpr (std::is_same_v<ValueJSON, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"json"});
    result.assign("value", sourcemeta::jsontoolkit::JSON{value});
    return result;
  } else if constexpr (std::is_same_v<ValueBoolean, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"boolean"});
    result.assign("value", sourcemeta::jsontoolkit::JSON{value});
    return result;
  } else if constexpr (std::is_same_v<ValueRegex, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"regex"});
    result.assign("value", sourcemeta::jsontoolkit::JSON{value.second});
    return result;
  } else if constexpr (std::is_same_v<ValueType, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"type"});
    std::ostringstream type_string;
    type_string << value;
    result.assign("value", sourcemeta::jsontoolkit::JSON{type_string.str()});
    return result;
  } else if constexpr (std::is_same_v<ValueTypes, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"types"});
    sourcemeta::jsontoolkit::JSON types{
        sourcemeta::jsontoolkit::JSON::make_array()};
    for (const auto type : value) {
      std::ostringstream type_string;
      type_string << type;
      types.push_back(sourcemeta::jsontoolkit::JSON{type_string.str()});
    }

    result.assign("value", std::move(types));
    return result;
  } else if constexpr (std::is_same_v<ValueString, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"string"});
    result.assign("value", sourcemeta::jsontoolkit::JSON{value});
    return result;
  } else if constexpr (std::is_same_v<ValueStrings, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"strings"});
    sourcemeta::jsontoolkit::JSON items{
        sourcemeta::jsontoolkit::JSON::make_array()};
    for (const auto &item : value) {
      items.push_back(sourcemeta::jsontoolkit::JSON{item});
    }

    result.assign("value", std::move(items));
    return result;
  } else if constexpr (std::is_same_v<ValueArray, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"array"});
    sourcemeta::jsontoolkit::JSON items{
        sourcemeta::jsontoolkit::JSON::make_array()};
    for (const auto &item : value) {
      items.push_back(item);
    }

    result.assign("value", std::move(items));
    return result;
  } else if constexpr (std::is_same_v<ValueUnsignedInteger, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"unsigned-integer"});
    result.assign("value", sourcemeta::jsontoolkit::JSON{value});
    return result;
  } else if constexpr (std::is_same_v<ValueRange, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"range"});
    sourcemeta::jsontoolkit::JSON values{
        sourcemeta::jsontoolkit::JSON::make_array()};
    const auto &range{value};
    values.push_back(sourcemeta::jsontoolkit::JSON{std::get<0>(range)});
    values.push_back(
        std::get<1>(range).has_value()
            ? sourcemeta::jsontoolkit::JSON{std::get<1>(range).value()}
            : sourcemeta::jsontoolkit::JSON{nullptr});
    values.push_back(sourcemeta::jsontoolkit::JSON{std::get<2>(range)});
    result.assign("value", std::move(values));
    return result;
  } else if constexpr (std::is_same_v<ValueNamedIndexes, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"named-indexes"});
    sourcemeta::jsontoolkit::JSON values{
        sourcemeta::jsontoolkit::JSON::make_object()};
    for (const auto &[name, index] : value) {
      values.assign(name, sourcemeta::jsontoolkit::JSON{index});
    }

    result.assign("value", std::move(values));
    return result;
  } else if constexpr (std::is_same_v<ValueStringMap, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"string-map"});
    sourcemeta::jsontoolkit::JSON map{
        sourcemeta::jsontoolkit::JSON::make_object()};
    for (const auto &[string, strings] : value) {
      sourcemeta::jsontoolkit::JSON dependencies{
          sourcemeta::jsontoolkit::JSON::make_array()};
      for (const auto &substring : strings) {
        dependencies.push_back(sourcemeta::jsontoolkit::JSON{substring});
      }

      map.assign(string, std::move(dependencies));
    }

    result.assign("value", std::move(map));
    return result;
  } else if constexpr (std::is_same_v<ValuePropertyFilter, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"property-filter"});
    sourcemeta::jsontoolkit::JSON data{
        sourcemeta::jsontoolkit::JSON::make_object()};
    data.assign("names", sourcemeta::jsontoolkit::JSON::make_array());
    data.assign("prefixes", sourcemeta::jsontoolkit::JSON::make_array());
    data.assign("patterns", sourcemeta::jsontoolkit::JSON::make_array());

    for (const auto &name : std::get<0>(value)) {
      data.at("names").push_back(sourcemeta::jsontoolkit::JSON{name});
    }

    for (const auto &prefix : std::get<1>(value)) {
      data.at("prefixes").push_back(sourcemeta::jsontoolkit::JSON{prefix});
    }

    for (const auto &pattern : std::get<2>(value)) {
      data.at("patterns")
          .push_back(sourcemeta::jsontoolkit::JSON{pattern.second});
    }

    result.assign("value", std::move(data));
    return result;
  } else if constexpr (std::is_same_v<ValueStringType, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"string-type"});
    switch (value) {
      case ValueStringType::URI:
        result.assign("value", sourcemeta::jsontoolkit::JSON{"uri"});
        break;
      default:
        // We should never get here
        assert(false);
    }

    return result;
  } else if constexpr (std::is_same_v<ValueIndexPair, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"index-pair"});
    sourcemeta::jsontoolkit::JSON data{
        sourcemeta::jsontoolkit::JSON::make_array()};
    data.push_back(sourcemeta::jsontoolkit::JSON{value.first});
    data.push_back(sourcemeta::jsontoolkit::JSON{value.second});
    result.assign("value", std::move(data));
    return result;
  } else if constexpr (std::is_same_v<ValuePointer, T>) {
    result.assign("type", sourcemeta::jsontoolkit::JSON{"pointer"});
    result.assign("value", sourcemeta::jsontoolkit::JSON{
                               sourcemeta::jsontoolkit::to_string(value)});
    return result;
  } else {
    static_assert(std::is_same_v<ValueNone, T>);
    return sourcemeta::jsontoolkit::JSON{nullptr};
  }
}

template <typename V>
auto step_to_json(const sourcemeta::blaze::Template::value_type &step)
    -> sourcemeta::jsontoolkit::JSON {
  static V visitor;
  return std::visit(visitor, step);
}

template <typename V, typename T>
auto encode_step(const std::string_view category, const std::string_view type,
                 const T &step) -> sourcemeta::jsontoolkit::JSON {
  sourcemeta::jsontoolkit::JSON result{
      sourcemeta::jsontoolkit::JSON::make_object()};
  result.assign("category", sourcemeta::jsontoolkit::JSON{category});
  result.assign("type", sourcemeta::jsontoolkit::JSON{type});
  result.assign(
      "relativeSchemaLocation",
      sourcemeta::jsontoolkit::JSON{to_string(step.relative_schema_location)});
  result.assign("relativeInstanceLocation",
                sourcemeta::jsontoolkit::JSON{
                    to_string(step.relative_instance_location)});
  result.assign("absoluteKeywordLocation",
                sourcemeta::jsontoolkit::JSON{step.keyword_location});
  result.assign("schemaResource",
                sourcemeta::jsontoolkit::JSON{step.schema_resource});
  result.assign("dynamic", sourcemeta::jsontoolkit::JSON{step.dynamic});
  result.assign("track", sourcemeta::jsontoolkit::JSON{step.track});
  result.assign("value", value_to_json(step.value));

  if constexpr (requires { step.children; }) {
    result.assign("children", sourcemeta::jsontoolkit::JSON::make_array());
    for (const auto &child : step.children) {
      result.at("children").push_back(step_to_json<V>(child));
    }
  }

  return result;
}

struct StepVisitor {
#define HANDLE_STEP(category, type, name)                                      \
  auto operator()(const sourcemeta::blaze::name &step)                         \
      const->sourcemeta::jsontoolkit::JSON {                                   \
    return encode_step<StepVisitor>(category, type, step);                     \
  }

  HANDLE_STEP("assertion", "fail", AssertionFail)
  HANDLE_STEP("assertion", "defines", AssertionDefines)
  HANDLE_STEP("assertion", "defines-all", AssertionDefinesAll)
  HANDLE_STEP("assertion", "property-dependencies",
              AssertionPropertyDependencies)
  HANDLE_STEP("assertion", "type", AssertionType)
  HANDLE_STEP("assertion", "type-any", AssertionTypeAny)
  HANDLE_STEP("assertion", "type-strict", AssertionTypeStrict)
  HANDLE_STEP("assertion", "type-strict-any", AssertionTypeStrictAny)
  HANDLE_STEP("assertion", "type-string-bounded", AssertionTypeStringBounded)
  HANDLE_STEP("assertion", "type-array-bounded", AssertionTypeArrayBounded)
  HANDLE_STEP("assertion", "type-object-bounded", AssertionTypeObjectBounded)
  HANDLE_STEP("assertion", "regex", AssertionRegex)
  HANDLE_STEP("assertion", "string-size-less", AssertionStringSizeLess)
  HANDLE_STEP("assertion", "string-size-greater", AssertionStringSizeGreater)
  HANDLE_STEP("assertion", "array-size-less", AssertionArraySizeLess)
  HANDLE_STEP("assertion", "array-size-greater", AssertionArraySizeGreater)
  HANDLE_STEP("assertion", "object-size-less", AssertionObjectSizeLess)
  HANDLE_STEP("assertion", "object-size-greater", AssertionObjectSizeGreater)
  HANDLE_STEP("assertion", "equal", AssertionEqual)
  HANDLE_STEP("assertion", "greater-equal", AssertionGreaterEqual)
  HANDLE_STEP("assertion", "less-equal", AssertionLessEqual)
  HANDLE_STEP("assertion", "greater", AssertionGreater)
  HANDLE_STEP("assertion", "less", AssertionLess)
  HANDLE_STEP("assertion", "unique", AssertionUnique)
  HANDLE_STEP("assertion", "divisible", AssertionDivisible)
  HANDLE_STEP("assertion", "string-type", AssertionStringType)
  HANDLE_STEP("assertion", "property-type", AssertionPropertyType)
  HANDLE_STEP("assertion", "property-type-evaluate",
              AssertionPropertyTypeEvaluate)
  HANDLE_STEP("assertion", "property-type-strict", AssertionPropertyTypeStrict)
  HANDLE_STEP("assertion", "property-type-strict-evaluate",
              AssertionPropertyTypeStrictEvaluate)
  HANDLE_STEP("assertion", "property-type-strict-any",
              AssertionPropertyTypeStrictAny)
  HANDLE_STEP("assertion", "property-type-strict-any-evaluate",
              AssertionPropertyTypeStrictAnyEvaluate)
  HANDLE_STEP("assertion", "array-prefix", AssertionArrayPrefix)
  HANDLE_STEP("assertion", "array-prefix-evaluate",
              AssertionArrayPrefixEvaluate)
  HANDLE_STEP("assertion", "equals-any", AssertionEqualsAny)
  HANDLE_STEP("annotation", "emit", AnnotationEmit)
  HANDLE_STEP("annotation", "to-parent", AnnotationToParent)
  HANDLE_STEP("annotation", "basename-to-parent", AnnotationBasenameToParent)
  HANDLE_STEP("logical", "not", LogicalNot)
  HANDLE_STEP("logical", "not-evaluate", LogicalNotEvaluate)
  HANDLE_STEP("logical", "or", LogicalOr)
  HANDLE_STEP("logical", "and", LogicalAnd)
  HANDLE_STEP("logical", "xor", LogicalXor)
  HANDLE_STEP("logical", "condition", LogicalCondition)
  HANDLE_STEP("logical", "when-type", LogicalWhenType)
  HANDLE_STEP("logical", "when-defines", LogicalWhenDefines)
  HANDLE_STEP("logical", "when-array-size-greater", LogicalWhenArraySizeGreater)
  HANDLE_STEP("loop", "properties-unevaluated", LoopPropertiesUnevaluated)
  HANDLE_STEP("loop", "properties-unevaluated-except",
              LoopPropertiesUnevaluatedExcept)
  HANDLE_STEP("loop", "properties-match", LoopPropertiesMatch)
  HANDLE_STEP("loop", "properties", LoopProperties)
  HANDLE_STEP("loop", "properties-evaluate", LoopPropertiesEvaluate)
  HANDLE_STEP("loop", "properties-regex", LoopPropertiesRegex)
  HANDLE_STEP("loop", "properties-starts-with", LoopPropertiesStartsWith)
  HANDLE_STEP("loop", "properties-except", LoopPropertiesExcept)
  HANDLE_STEP("loop", "properties-whitelist", LoopPropertiesWhitelist)
  HANDLE_STEP("loop", "properties-type", LoopPropertiesType)
  HANDLE_STEP("loop", "properties-type-evaluate", LoopPropertiesTypeEvaluate)
  HANDLE_STEP("loop", "properties-type-strict", LoopPropertiesTypeStrict)
  HANDLE_STEP("loop", "properties-type-strict-evaluate",
              LoopPropertiesTypeStrictEvaluate)
  HANDLE_STEP("loop", "properties-type-strict-any", LoopPropertiesTypeStrictAny)
  HANDLE_STEP("loop", "properties-type-strict-evaluate-any",
              LoopPropertiesTypeStrictAnyEvaluate)
  HANDLE_STEP("loop", "keys", LoopKeys)
  HANDLE_STEP("loop", "items", LoopItems)
  HANDLE_STEP("loop", "items-unevaluated", LoopItemsUnevaluated)
  HANDLE_STEP("loop", "items-type", LoopItemsType)
  HANDLE_STEP("loop", "items-type-strict", LoopItemsTypeStrict)
  HANDLE_STEP("loop", "items-type-strict-any", LoopItemsTypeStrictAny)
  HANDLE_STEP("loop", "contains", LoopContains)
  HANDLE_STEP("control", "group", ControlGroup)
  HANDLE_STEP("control", "group-when-defines", ControlGroupWhenDefines)
  HANDLE_STEP("control", "label", ControlLabel)
  HANDLE_STEP("control", "mark", ControlMark)
  HANDLE_STEP("control", "evaluate", ControlEvaluate)
  HANDLE_STEP("control", "jump", ControlJump)
  HANDLE_STEP("control", "dynamic-anchor-jump", ControlDynamicAnchorJump)

#undef HANDLE_STEP
};

} // namespace

namespace sourcemeta::blaze {

auto to_json(const Template &steps) -> sourcemeta::jsontoolkit::JSON {
  sourcemeta::jsontoolkit::JSON result{
      sourcemeta::jsontoolkit::JSON::make_array()};
  for (const auto &step : steps) {
    result.push_back(step_to_json<StepVisitor>(step));
  }

  return result;
}

auto template_format_compare(const sourcemeta::jsontoolkit::JSON::String &left,
                             const sourcemeta::jsontoolkit::JSON::String &right)
    -> bool {
  using Rank = std::map<
      sourcemeta::jsontoolkit::JSON::String, std::uint64_t,
      std::less<sourcemeta::jsontoolkit::JSON::String>,
      sourcemeta::jsontoolkit::JSON::Allocator<std::pair<
          const sourcemeta::jsontoolkit::JSON::String, std::uint64_t>>>;
  static Rank rank{{"category", 0},
                   {"type", 1},
                   {"value", 2},
                   {"schemaResource", 3},
                   {"absoluteKeywordLocation", 4},
                   {"relativeSchemaLocation", 5},
                   {"relativeInstanceLocation", 6},
                   {"evaluatePath", 7},
                   {"dynamic", 8},
                   {"track", 9},
                   {"children", 10}};

  constexpr std::uint64_t DEFAULT_RANK{999};
  const auto left_rank{rank.contains(left) ? rank.at(left) : DEFAULT_RANK};
  const auto right_rank{rank.contains(right) ? rank.at(right) : DEFAULT_RANK};
  return left_rank < right_rank;
}

} // namespace sourcemeta::blaze
