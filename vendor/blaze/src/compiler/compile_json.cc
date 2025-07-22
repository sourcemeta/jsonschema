#include <sourcemeta/blaze/compiler.h>

#include <cassert> // assert
#include <variant> // std::visit

namespace {
auto value_from_json(const sourcemeta::core::JSON &wrapper)
    -> std::optional<sourcemeta::blaze::Value> {
  if (!wrapper.is_object()) {
    return std::nullopt;
  }

  const auto type{wrapper.try_at("t")};
  const auto value{wrapper.try_at("v")};
  if (!type || !value) {
    return std::nullopt;
  }

  using namespace sourcemeta::blaze;
  switch (type->to_integer()) {
      // clang-format off
    case 0:  return sourcemeta::core::from_json<ValueNone>(*value);
    case 1:  return sourcemeta::core::from_json<ValueJSON>(*value);
    case 2:  return sourcemeta::core::from_json<ValueSet>(*value);
    case 3:  return sourcemeta::core::from_json<ValueString>(*value);
    case 4:  return sourcemeta::core::from_json<ValueProperty>(*value);
    case 5:  return sourcemeta::core::from_json<ValueStrings>(*value);
    case 6:  return sourcemeta::core::from_json<ValueStringSet>(*value);
    case 7:  return sourcemeta::core::from_json<ValueTypes>(*value);
    case 8:  return sourcemeta::core::from_json<ValueType>(*value);
    case 9:  return sourcemeta::core::from_json<ValueRegex>(*value);
    case 10: return sourcemeta::core::from_json<ValueUnsignedInteger>(*value);
    case 11: return sourcemeta::core::from_json<ValueRange>(*value);
    case 12: return sourcemeta::core::from_json<ValueBoolean>(*value);
    case 13: return sourcemeta::core::from_json<ValueNamedIndexes>(*value);
    case 14: return sourcemeta::core::from_json<ValueStringType>(*value);
    case 15: return sourcemeta::core::from_json<ValueStringMap>(*value);
    case 16: return sourcemeta::core::from_json<ValuePropertyFilter>(*value);
    case 17: return sourcemeta::core::from_json<ValueIndexPair>(*value);
    case 18: return sourcemeta::core::from_json<ValuePointer>(*value);
    case 19: return sourcemeta::core::from_json<ValueTypedProperties>(*value);
    case 20: return sourcemeta::core::from_json<ValueStringHashes>(*value);
    case 21: return sourcemeta::core::from_json<ValueTypedHashes>(*value);
    // clang-format on
    default:
      assert(false);
      return ValueNone{};
  }
}

auto instructions_from_json(const sourcemeta::core::JSON &instructions)
    -> std::optional<sourcemeta::blaze::Instructions> {
  if (!instructions.is_array()) {
    return std::nullopt;
  }

  sourcemeta::blaze::Instructions result;
  result.reserve(instructions.size());
  for (const auto &instruction : instructions.as_array()) {
    if (!instruction.is_object()) {
      return std::nullopt;
    }

    const auto type{instruction.try_at("t")};
    const auto relative_schema_location{instruction.try_at("s")};
    const auto relative_instance_location{instruction.try_at("i")};
    const auto keyword_location{instruction.try_at("k")};
    const auto schema_resource{instruction.try_at("r")};
    const auto value{instruction.try_at("v")};
    const auto children{instruction.try_at("c")};

    if (!type || !relative_schema_location || !relative_instance_location ||
        !keyword_location || !schema_resource || !value || !children ||
        !type->is_positive() || !relative_schema_location->is_string() ||
        !relative_instance_location->is_string() ||
        !keyword_location->is_string() || !schema_resource->is_positive() ||
        !value->is_object() || !children->is_array()) {
      return std::nullopt;
    }

    auto type_result{
        sourcemeta::core::from_json<sourcemeta::blaze::InstructionIndex>(
            *type)};
    auto relative_schema_location_result{
        sourcemeta::core::from_json<sourcemeta::core::Pointer>(
            *relative_schema_location)};
    auto relative_instance_location_result{
        sourcemeta::core::from_json<sourcemeta::core::Pointer>(
            *relative_instance_location)};
    auto keyword_location_result{
        sourcemeta::core::from_json<std::string>(*keyword_location)};
    auto schema_resource_result{
        sourcemeta::core::from_json<std::size_t>(*schema_resource)};
    auto value_result{value_from_json(*value)};
    auto children_result{instructions_from_json(*children)};

    if (!type_result.has_value() ||
        !relative_schema_location_result.has_value() ||
        !relative_instance_location_result.has_value() ||
        !keyword_location_result.has_value() ||
        !schema_resource_result.has_value() || !value_result.has_value() ||
        !children_result.has_value()) {
      return std::nullopt;
    }

    // TODO: Maybe we should emplace here?
    result.push_back({std::move(type_result).value(),
                      std::move(relative_schema_location_result).value(),
                      std::move(relative_instance_location_result).value(),
                      std::move(keyword_location_result).value(),
                      std::move(schema_resource_result).value(),
                      std::move(value_result).value(),
                      std::move(children_result).value()});
  }

  return result;
}

auto to_json(const sourcemeta::blaze::Instruction &instruction)
    -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  // We use single characters to save space, as this serialised format
  // is not meant to be human-readable anyway
  result.assign("t", sourcemeta::core::to_json(instruction.type));
  result.assign(
      "s", sourcemeta::core::to_json(instruction.relative_schema_location));
  result.assign(
      "i", sourcemeta::core::to_json(instruction.relative_instance_location));
  result.assign("k", sourcemeta::core::to_json(instruction.keyword_location));
  result.assign("r", sourcemeta::core::to_json(instruction.schema_resource));

  auto value{sourcemeta::core::JSON::make_object()};
  value.assign("t", sourcemeta::core::to_json(instruction.value.index()));
  value.assign("v", std::visit(
                        [](const auto &variant) {
                          return sourcemeta::core::to_json(variant);
                        },
                        instruction.value));
  result.assign("v", std::move(value));

  assert(result.at("v").is_object());
  assert(result.at("v").size() == 2);
  assert(result.at("v").defines("t"));
  assert(result.at("v").defines("v"));
  assert(result.at("v").at("t").is_integer());

  auto children_json{sourcemeta::core::JSON::make_array()};
  result.assign("c", sourcemeta::core::to_json(instruction.children,
                                               [](const auto &subinstruction) {
                                                 return to_json(subinstruction);
                                               }));
  return result;
}
} // namespace

namespace sourcemeta::blaze {

auto to_json(const Template &schema_template) -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign("dynamic", sourcemeta::core::JSON{schema_template.dynamic});
  result.assign("track", sourcemeta::core::JSON{schema_template.track});
  result.assign("instructions",
                sourcemeta::core::to_json(schema_template.instructions,
                                          [](const auto &instruction) {
                                            return ::to_json(instruction);
                                          }));
  return result;
}

auto from_json(const sourcemeta::core::JSON &json) -> std::optional<Template> {
  if (!json.is_object()) {
    return std::nullopt;
  }

  const auto instructions{json.try_at("instructions")};
  const auto dynamic{json.try_at("dynamic")};
  const auto track{json.try_at("track")};
  if (!instructions || !dynamic || !track) {
    return std::nullopt;
  }

  auto instructions_result{instructions_from_json(*instructions)};
  if (!instructions_result.has_value() || !dynamic->is_boolean() ||
      !track->is_boolean()) {
    return std::nullopt;
  }

  return Template{.instructions = std::move(instructions_result).value(),
                  .dynamic = dynamic->to_boolean(),
                  .track = track->to_boolean()};
}

} // namespace sourcemeta::blaze
