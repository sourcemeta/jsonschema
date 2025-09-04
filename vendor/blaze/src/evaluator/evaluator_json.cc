#include <sourcemeta/blaze/evaluator.h>

#include <cassert> // assert

namespace {
auto value_from_json(const sourcemeta::core::JSON &wrapper)
    -> std::optional<sourcemeta::blaze::Value> {
  if (!wrapper.is_array() || wrapper.array_size() == 0 ||
      !wrapper.at(0).is_integer()) {
    return std::nullopt;
  } else if (wrapper.array_size() == 1) {
    return sourcemeta::blaze::ValueNone{};
  }

  const auto &value{wrapper.at(1)};

  using namespace sourcemeta::blaze;
  switch (wrapper.at(0).to_integer()) {
      // clang-format off
    case 0:  return ValueNone{};
    case 1:  return sourcemeta::core::from_json<ValueJSON>(value);
    case 2:  return sourcemeta::core::from_json<ValueSet>(value);
    case 3:  return sourcemeta::core::from_json<ValueString>(value);
    case 4:  return sourcemeta::core::from_json<ValueProperty>(value);
    case 5:  return sourcemeta::core::from_json<ValueStrings>(value);
    case 6:  return sourcemeta::core::from_json<ValueStringSet>(value);
    case 7:  return sourcemeta::core::from_json<ValueTypes>(value);
    case 8:  return sourcemeta::core::from_json<ValueType>(value);
    case 9:  return sourcemeta::core::from_json<ValueRegex>(value);
    case 10: return sourcemeta::core::from_json<ValueUnsignedInteger>(value);
    case 11: return sourcemeta::core::from_json<ValueRange>(value);
    case 12: return sourcemeta::core::from_json<ValueBoolean>(value);
    case 13: return sourcemeta::core::from_json<ValueNamedIndexes>(value);
    case 14: return sourcemeta::core::from_json<ValueStringType>(value);
    case 15: return sourcemeta::core::from_json<ValueStringMap>(value);
    case 16: return sourcemeta::core::from_json<ValuePropertyFilter>(value);
    case 17: return sourcemeta::core::from_json<ValueIndexPair>(value);
    case 18: return sourcemeta::core::from_json<ValuePointer>(value);
    case 19: return sourcemeta::core::from_json<ValueTypedProperties>(value);
    case 20: return sourcemeta::core::from_json<ValueStringHashes>(value);
    case 21: return sourcemeta::core::from_json<ValueTypedHashes>(value);
    // clang-format on
    default:
      assert(false);
      return ValueNone{};
  }
}

auto instructions_from_json(const sourcemeta::core::JSON &instructions,
                            const sourcemeta::core::JSON &resources)
    -> std::optional<sourcemeta::blaze::Instructions> {
  if (!instructions.is_array()) {
    return std::nullopt;
  }

  sourcemeta::blaze::Instructions result;
  result.reserve(instructions.size());
  for (const auto &instruction : instructions.as_array()) {
    if (!instruction.is_array() || instruction.array_size() < 6) {
      return std::nullopt;
    }

    const auto &type{instruction.at(0)};
    const auto &relative_schema_location{instruction.at(1)};
    const auto &relative_instance_location{instruction.at(2)};
    const auto &keyword_location{instruction.at(3)};
    const auto &schema_resource{instruction.at(4)};
    const auto &value{instruction.at(5)};

    auto type_result{
        sourcemeta::core::from_json<sourcemeta::blaze::InstructionIndex>(type)};
    auto relative_schema_location_result{
        sourcemeta::core::from_json<sourcemeta::core::Pointer>(
            relative_schema_location)};
    auto relative_instance_location_result{
        sourcemeta::core::from_json<sourcemeta::core::Pointer>(
            relative_instance_location)};
    auto keyword_location_result{
        sourcemeta::core::from_json<std::string>(keyword_location)};
    auto schema_resource_result{
        sourcemeta::core::from_json<std::size_t>(schema_resource)};
    auto value_result{value_from_json(value)};

    // Parse children if there
    std::optional<sourcemeta::blaze::Instructions> children_result{
        instruction.array_size() == 7
            ? instructions_from_json(instruction.at(6), resources)
            : sourcemeta::blaze::Instructions{}};

    if (!type_result.has_value() ||
        !relative_schema_location_result.has_value() ||
        !relative_instance_location_result.has_value() ||
        !keyword_location_result.has_value() ||
        !schema_resource_result.has_value() || !value_result.has_value() ||
        !children_result.has_value()) {
      return std::nullopt;
    }

    if (schema_resource_result.value() > 0 &&
        resources.array_size() >= schema_resource_result.value() &&
        keyword_location_result.value().starts_with('#')) {
      // TODO: Maybe we should emplace here?
      result.push_back(
          {std::move(type_result).value(),
           std::move(relative_schema_location_result).value(),
           std::move(relative_instance_location_result).value(),
           resources.at(schema_resource_result.value() - 1).to_string() +
               std::move(keyword_location_result).value(),
           schema_resource_result.value(), std::move(value_result).value(),
           std::move(children_result).value()});
    } else {
      // TODO: Maybe we should emplace here?
      result.push_back({std::move(type_result).value(),
                        std::move(relative_schema_location_result).value(),
                        std::move(relative_instance_location_result).value(),
                        std::move(keyword_location_result).value(),
                        std::move(schema_resource_result).value(),
                        std::move(value_result).value(),
                        std::move(children_result).value()});
    }
  }

  return result;
}

} // namespace

namespace sourcemeta::blaze {

auto from_json(const sourcemeta::core::JSON &json) -> std::optional<Template> {
  if (!json.is_array() || json.array_size() != 4) {
    return std::nullopt;
  }

  const auto &dynamic{json.at(0)};
  const auto &track{json.at(1)};
  const auto &resources{json.at(2)};

  if (!dynamic.is_boolean() || !track.is_boolean() || !resources.is_array()) {
    return std::nullopt;
  }

  const auto &instructions{json.at(3)};
  auto instructions_result{instructions_from_json(instructions, resources)};
  if (!instructions_result.has_value()) {
    return std::nullopt;
  }

  return Template{.instructions = std::move(instructions_result).value(),
                  .dynamic = dynamic.to_boolean(),
                  .track = track.to_boolean()};
}

} // namespace sourcemeta::blaze
