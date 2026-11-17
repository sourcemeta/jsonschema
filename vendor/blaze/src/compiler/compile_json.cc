#include <sourcemeta/blaze/compiler.h>

#include <cassert> // assert
#include <variant> // std::visit

namespace {
auto to_json(const sourcemeta::blaze::Instruction &instruction,
             const std::vector<sourcemeta::blaze::InstructionExtra> &extra)
    -> sourcemeta::core::JSON {
  // Note that we purposely avoid objects to help consumers avoid potentially
  // expensive hash-map or flat-map lookups when parsing back
  auto result{sourcemeta::core::JSON::make_array()};
  result.push_back(sourcemeta::core::to_json(instruction.type));

  const auto &meta{extra[instruction.extra_index]};
  result.push_back(sourcemeta::core::to_json(meta.relative_schema_location));
  result.push_back(
      sourcemeta::core::to_json(instruction.relative_instance_location));
  result.push_back(sourcemeta::core::to_json(meta.keyword_location));
  result.push_back(sourcemeta::core::to_json(meta.schema_resource));
  result.push_back(sourcemeta::core::to_json(meta.vocabulary));
  result.push_back(sourcemeta::core::to_json(meta.message));
  result.push_back(sourcemeta::core::to_json(meta.message_detail));

  // Note that we purposely avoid objects to help consumers avoid potentially
  // expensive hash-map or flat-map lookups when parsing back
  auto value{sourcemeta::core::JSON::make_array()};
  const auto value_index{instruction.value.index()};
  value.push_back(sourcemeta::core::to_json(value_index));
  // Don't encode empty values, which tend to happen a lot
  if (value_index != 0) {
    value.push_back(std::visit(
        [](const auto &variant) -> auto {
          return sourcemeta::core::to_json(variant);
        },
        instruction.value));
  }
  assert(value.is_array());
  assert(!value.empty());
  assert(value.at(0).is_integer());
  result.push_back(std::move(value));

  if (!instruction.children.empty()) {
    auto children_json{sourcemeta::core::JSON::make_array()};
    result.push_back(sourcemeta::core::to_json(
        instruction.children, [&extra](const auto &subinstruction) -> auto {
          return to_json(subinstruction, extra);
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
  result.push_back(sourcemeta::core::JSON{
      static_cast<std::int64_t>(sourcemeta::blaze::JSON_VERSION)});
  result.push_back(sourcemeta::core::JSON{schema_template.dynamic});
  result.push_back(sourcemeta::core::JSON{schema_template.frames});

  auto targets{sourcemeta::core::JSON::make_array()};
  for (const auto &target : schema_template.targets) {
    targets.push_back(sourcemeta::core::to_json(
        target, [&schema_template](const auto &instruction) -> auto {
          return ::to_json(instruction, schema_template.extra);
        }));
  }

  result.push_back(std::move(targets));

  auto bindings{sourcemeta::core::JSON::make_array()};
  for (const auto &list : schema_template.bindings) {
    auto entries{sourcemeta::core::JSON::make_array()};
    for (const auto &[anchor, target] : list) {
      auto pair{sourcemeta::core::JSON::make_array()};
      pair.push_back(sourcemeta::core::JSON{static_cast<std::int64_t>(anchor)});
      pair.push_back(sourcemeta::core::JSON{static_cast<std::int64_t>(target)});
      entries.push_back(std::move(pair));
    }
    bindings.push_back(std::move(entries));
  }
  result.push_back(std::move(bindings));

  auto target_bindings{sourcemeta::core::JSON::make_array()};
  for (const auto &index : schema_template.target_bindings) {
    target_bindings.push_back(
        sourcemeta::core::JSON{static_cast<std::int64_t>(index)});
  }
  result.push_back(std::move(target_bindings));

  auto anchors{sourcemeta::core::JSON::make_array()};
  for (const auto &anchor : schema_template.anchors) {
    anchors.push_back(sourcemeta::core::JSON{anchor});
  }
  result.push_back(std::move(anchors));

  auto vocabularies{sourcemeta::core::JSON::make_array()};
  for (const auto &vocabulary : schema_template.vocabularies) {
    vocabularies.push_back(sourcemeta::core::JSON{vocabulary});
  }
  result.push_back(std::move(vocabularies));

  auto messages{sourcemeta::core::JSON::make_array()};
  for (const auto &message : schema_template.messages) {
    messages.push_back(sourcemeta::core::JSON{message});
  }
  result.push_back(std::move(messages));

  return result;
}

} // namespace sourcemeta::blaze
