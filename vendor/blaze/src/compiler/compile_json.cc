#include <sourcemeta/blaze/compiler.h>

#include <cassert>     // assert
#include <string_view> // std::string_view
#include <variant>     // std::visit

namespace {
auto to_json(const sourcemeta::blaze::Instruction &instruction,
             const std::vector<sourcemeta::blaze::InstructionExtra> &extra,
             std::vector<sourcemeta::core::JSON::String> &resources)
    -> sourcemeta::core::JSON {
  // Note that we purposely avoid objects to help consumers avoid potentially
  // expensive hash-map or flat-map lookups when parsing back
  auto result{sourcemeta::core::JSON::make_array()};

  // We use single characters to save space, as this serialised format
  // is not meant to be human-readable anyway
  result.push_back(sourcemeta::core::to_json(instruction.type));

  const auto &meta{extra[instruction.extra_index]};
  result.push_back(sourcemeta::core::to_json(meta.relative_schema_location));
  result.push_back(
      sourcemeta::core::to_json(instruction.relative_instance_location));

  const auto match{meta.keyword_location.find('#')};
  if (meta.schema_resource > 0 && match != std::string::npos) {
    if (resources.size() < meta.schema_resource) {
      resources.resize(meta.schema_resource);
    }

    if (resources[meta.schema_resource - 1].empty()) {
      resources[meta.schema_resource - 1] =
          meta.keyword_location.substr(0, match);
    }

    result.push_back(
        sourcemeta::core::JSON{meta.keyword_location.substr(match)});
  } else {
    result.push_back(sourcemeta::core::to_json(meta.keyword_location));
  }

  result.push_back(sourcemeta::core::to_json(meta.schema_resource));

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
        instruction.children, [&extra, &resources](const auto &subinstruction) {
          return to_json(subinstruction, extra, resources);
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

  auto targets{sourcemeta::core::JSON::make_array()};
  for (const auto &target : schema_template.targets) {
    targets.push_back(sourcemeta::core::to_json(
        target, [&schema_template, &resources](const auto &instruction) {
          return ::to_json(instruction, schema_template.extra, resources);
        }));
  }

  result.push_back(sourcemeta::core::to_json(resources));
  result.push_back(std::move(targets));

  auto labels{sourcemeta::core::JSON::make_array()};
  for (const auto &[key, value] : schema_template.labels) {
    auto pair{sourcemeta::core::JSON::make_array()};
    pair.push_back(sourcemeta::core::JSON{static_cast<std::int64_t>(key)});
    pair.push_back(sourcemeta::core::JSON{static_cast<std::int64_t>(value)});
    labels.push_back(std::move(pair));
  }
  result.push_back(std::move(labels));

  return result;
}

} // namespace sourcemeta::blaze
