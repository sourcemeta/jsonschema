#include <sourcemeta/blaze/compiler.h>

#include <cassert>     // assert
#include <string_view> // std::string_view
#include <variant>     // std::visit

namespace {
auto to_json(const sourcemeta::blaze::Instruction &instruction,
             std::vector<sourcemeta::core::JSON::String> &resources)
    -> sourcemeta::core::JSON {
  // Note that we purposely avoid objects to help consumers avoid potentially
  // expensive hash-map or flat-map lookups when parsing back
  auto result{sourcemeta::core::JSON::make_array()};

  // We use single characters to save space, as this serialised format
  // is not meant to be human-readable anyway
  result.push_back(sourcemeta::core::to_json(instruction.type));

  result.push_back(
      sourcemeta::core::to_json(instruction.relative_schema_location));
  result.push_back(
      sourcemeta::core::to_json(instruction.relative_instance_location));

  const auto match{instruction.keyword_location.find('#')};
  if (instruction.schema_resource > 0 && match != std::string::npos) {
    if (resources.size() < instruction.schema_resource) {
      resources.resize(instruction.schema_resource);
    }

    if (resources[instruction.schema_resource - 1].empty()) {
      resources[instruction.schema_resource - 1] =
          instruction.keyword_location.substr(0, match);
    }

    result.push_back(
        sourcemeta::core::JSON{instruction.keyword_location.substr(match)});
  } else {
    result.push_back(sourcemeta::core::to_json(instruction.keyword_location));
  }

  result.push_back(sourcemeta::core::to_json(instruction.schema_resource));

  // Note that we purposely avoid objects to help consumers avoid potentially
  // expensive hash-map or flat-map lookups when parsing back
  auto value{sourcemeta::core::JSON::make_array()};
  const auto value_index{instruction.value.index()};
  value.push_back(sourcemeta::core::to_json(value_index));
  // Don't encode empty values, which tend to happen a lot
  if (value_index != 0) {
    value.push_back(std::visit(
        [](const auto &variant) { return sourcemeta::core::to_json(variant); },
        instruction.value));
  }
  assert(value.is_array());
  assert(!value.empty());
  assert(value.at(0).is_integer());
  result.push_back(std::move(value));

  if (!instruction.children.empty()) {
    auto children_json{sourcemeta::core::JSON::make_array()};
    result.push_back(sourcemeta::core::to_json(
        instruction.children, [&resources](const auto &subinstruction) {
          return to_json(subinstruction, resources);
        }));
  }

  return result;
}
} // namespace

namespace sourcemeta::blaze {

auto to_json(const Template &schema_template) -> sourcemeta::core::JSON {
  // Note that we purposely avoid objects to help consumers avoid potentially
  // expensive hash-map or flat-map lookups when parsing back
  auto result{sourcemeta::core::JSON::make_array()};
  result.push_back(sourcemeta::core::JSON{schema_template.dynamic});
  result.push_back(sourcemeta::core::JSON{schema_template.track});
  std::vector<sourcemeta::core::JSON::String> resources;
  auto instructions{sourcemeta::core::to_json(
      schema_template.instructions, [&resources](const auto &instruction) {
        return ::to_json(instruction, resources);
      })};
  result.push_back(sourcemeta::core::to_json(resources));
  result.push_back(std::move(instructions));
  return result;
}

} // namespace sourcemeta::blaze
