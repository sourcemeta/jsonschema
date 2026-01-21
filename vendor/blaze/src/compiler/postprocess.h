#ifndef SOURCEMETA_BLAZE_COMPILER_POSTPROCESS_H
#define SOURCEMETA_BLAZE_COMPILER_POSTPROCESS_H

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

// TODO: Move all `FastValidation` conditional optimisations from the default
// compilers here. Only the structural ones from Draft 4 seem to be missing

namespace sourcemeta::blaze {

struct TargetStatistics {
  std::size_t count;
  std::unordered_map<std::size_t, std::size_t> jump_targets;
  bool requires_empty_instance_location;
};

inline auto is_noop_without_children(const InstructionIndex type) noexcept
    -> bool {
  switch (type) {
    case InstructionIndex::LogicalAnd:
    case InstructionIndex::LogicalCondition:
    case InstructionIndex::LogicalWhenType:
    case InstructionIndex::LogicalWhenDefines:
    case InstructionIndex::LogicalWhenArraySizeGreater:
    case InstructionIndex::LoopPropertiesMatch:
    case InstructionIndex::LoopProperties:
    case InstructionIndex::LoopPropertiesEvaluate:
    case InstructionIndex::LoopPropertiesRegex:
    case InstructionIndex::LoopPropertiesStartsWith:
    case InstructionIndex::LoopPropertiesExcept:
    case InstructionIndex::LoopKeys:
    case InstructionIndex::LoopItems:
    case InstructionIndex::LoopItemsFrom:
    case InstructionIndex::LoopItemsUnevaluated:
    case InstructionIndex::LoopContains:
    case InstructionIndex::ControlGroupWhenDefines:
    case InstructionIndex::ControlGroupWhenDefinesDirect:
    case InstructionIndex::ControlGroupWhenType:
      return true;
    default:
      return false;
  }
}

inline auto
is_parent_to_children_instruction(const InstructionIndex type) noexcept
    -> bool {
  switch (type) {
    case InstructionIndex::LoopPropertiesMatch:
    case InstructionIndex::LoopPropertiesMatchClosed:
    case InstructionIndex::ControlGroupWhenDefines:
    case InstructionIndex::ControlGroupWhenDefinesDirect:
      return true;
    default:
      return false;
  }
}

inline auto convert_to_property_type_assertions(Instructions &instructions)
    -> void {
  for (auto &instruction : instructions) {
    if (!instruction.relative_instance_location.empty()) {
      switch (instruction.type) {
        case InstructionIndex::AssertionTypeStrict:
          instruction.type = InstructionIndex::AssertionPropertyTypeStrict;
          break;
        case InstructionIndex::AssertionType:
          instruction.type = InstructionIndex::AssertionPropertyType;
          break;
        case InstructionIndex::AssertionTypeStrictAny:
          instruction.type = InstructionIndex::AssertionPropertyTypeStrictAny;
          break;
        default:
          break;
      }
    }

    if (!is_parent_to_children_instruction(instruction.type)) {
      convert_to_property_type_assertions(instruction.children);
    }
  }
}

inline auto rebase(Instruction &instruction,
                   const sourcemeta::core::Pointer &schema_prefix,
                   const sourcemeta::core::Pointer &instance_prefix) -> void {
  instruction.relative_schema_location =
      schema_prefix.concat(instruction.relative_schema_location);
  instruction.relative_instance_location =
      instance_prefix.concat(instruction.relative_instance_location);

  if (instruction.type == InstructionIndex::LogicalCondition) {
    const auto &value{std::get<ValueIndexPair>(instruction.value)};
    const auto then_cursor{value.first};
    for (std::size_t index = then_cursor; index < instruction.children.size();
         ++index) {
      auto &child{instruction.children[index]};
      child.relative_schema_location =
          schema_prefix.concat(child.relative_schema_location);
      for (auto &grandchild : child.children) {
        grandchild.relative_schema_location =
            schema_prefix.concat(grandchild.relative_schema_location);
      }
    }
  }
}

inline auto collect_statistics(const Instructions &instructions,
                               TargetStatistics &statistics) -> void {
  for (const auto &instruction : instructions) {
    statistics.count += 1;

    if (instruction.type == InstructionIndex::ControlJump) {
      statistics
          .jump_targets[std::get<ValueUnsignedInteger>(instruction.value)]++;
    }

    if (instruction.type == InstructionIndex::ControlGroupWhenDefinesDirect ||
        instruction.type == InstructionIndex::ControlGroupWhenType) {
      statistics.requires_empty_instance_location = true;
    }

    collect_statistics(instruction.children, statistics);
  }
}

inline auto
transform_instruction(Instruction &instruction, Instructions &output,
                      const std::vector<Instructions> &targets,
                      const std::vector<TargetStatistics> &statistics,
                      TargetStatistics &current_stats, const Tweaks &tweaks,
                      const bool uses_dynamic_scopes) -> bool {
  if (instruction.type == InstructionIndex::ControlJump) {
    const auto jump_target_index{
        std::get<ValueUnsignedInteger>(instruction.value)};
    const auto &jump_target_stats{statistics[jump_target_index]};

    if (jump_target_stats.count == 0) {
      current_stats.jump_targets[jump_target_index]--;
      return true;
    }

    const auto jump_target_self_loop{
        jump_target_stats.jump_targets.find(jump_target_index)};
    if (jump_target_stats.count < tweaks.target_inline_threshold &&
        (jump_target_self_loop == jump_target_stats.jump_targets.end() ||
         jump_target_self_loop->second == 0) &&
        !uses_dynamic_scopes &&
        (!jump_target_stats.requires_empty_instance_location ||
         instruction.relative_instance_location.empty())) {
      current_stats.jump_targets[jump_target_index]--;
      for (const auto &[inlined_jump, inlined_count] :
           jump_target_stats.jump_targets) {
        current_stats.jump_targets[inlined_jump] += inlined_count;
      }
      current_stats.requires_empty_instance_location |=
          jump_target_stats.requires_empty_instance_location;

      for (auto target_instruction : targets[jump_target_index]) {
        rebase(target_instruction, instruction.relative_schema_location,
               instruction.relative_instance_location);
        output.push_back(std::move(target_instruction));
      }
      return true;
    }
  }

  if (is_noop_without_children(instruction.type) &&
      instruction.children.empty()) {
    return true;
  }

  // TODO: De-duplicate this logic from default_compiler_draft4.h. Just do it
  // all here

  if (instruction.type == InstructionIndex::LoopProperties &&
      instruction.children.size() == 1) {
    auto &child{instruction.children.front()};
    if (child.type == InstructionIndex::AssertionTypeStrict) {
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesTypeStrict,
                      .relative_schema_location =
                          instruction.relative_schema_location.concat(
                              child.relative_schema_location),
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .keyword_location = std::move(child.keyword_location),
                      .schema_resource = child.schema_resource,
                      .value = std::move(child.value),
                      .children = {}});
      return true;
    }

    if (child.type == InstructionIndex::AssertionType) {
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesType,
                      .relative_schema_location =
                          instruction.relative_schema_location.concat(
                              child.relative_schema_location),
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .keyword_location = std::move(child.keyword_location),
                      .schema_resource = child.schema_resource,
                      .value = std::move(child.value),
                      .children = {}});
      return true;
    }

    if (child.type == InstructionIndex::AssertionTypeStrictAny) {
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesTypeStrictAny,
                      .relative_schema_location =
                          instruction.relative_schema_location.concat(
                              child.relative_schema_location),
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .keyword_location = std::move(child.keyword_location),
                      .schema_resource = child.schema_resource,
                      .value = std::move(child.value),
                      .children = {}});
      return true;
    }
  }

  if (instruction.type == InstructionIndex::LoopPropertiesEvaluate &&
      instruction.children.size() == 1) {
    auto &child{instruction.children.front()};
    if (child.type == InstructionIndex::AssertionTypeStrict) {
      output.push_back(Instruction{
          .type = InstructionIndex::LoopPropertiesTypeStrictEvaluate,
          .relative_schema_location =
              instruction.relative_schema_location.concat(
                  child.relative_schema_location),
          .relative_instance_location =
              std::move(instruction.relative_instance_location),
          .keyword_location = std::move(child.keyword_location),
          .schema_resource = child.schema_resource,
          .value = std::move(child.value),
          .children = {}});
      return true;
    }

    if (child.type == InstructionIndex::AssertionType) {
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesTypeEvaluate,
                      .relative_schema_location =
                          instruction.relative_schema_location.concat(
                              child.relative_schema_location),
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .keyword_location = std::move(child.keyword_location),
                      .schema_resource = child.schema_resource,
                      .value = std::move(child.value),
                      .children = {}});
      return true;
    }

    if (child.type == InstructionIndex::AssertionTypeStrictAny) {
      output.push_back(Instruction{
          .type = InstructionIndex::LoopPropertiesTypeStrictAnyEvaluate,
          .relative_schema_location =
              instruction.relative_schema_location.concat(
                  child.relative_schema_location),
          .relative_instance_location =
              std::move(instruction.relative_instance_location),
          .keyword_location = std::move(child.keyword_location),
          .schema_resource = child.schema_resource,
          .value = std::move(child.value),
          .children = {}});
      return true;
    }
  }

  if (is_parent_to_children_instruction(instruction.type)) {
    convert_to_property_type_assertions(instruction.children);
  }

  if (!instruction.relative_instance_location.empty() &&
      std::ranges::all_of(
          instruction.relative_instance_location,
          [](const auto &token) { return token.is_property(); })) {
    switch (instruction.type) {
      case InstructionIndex::AssertionTypeStrict:
        instruction.type = InstructionIndex::AssertionPropertyTypeStrict;
        break;
      case InstructionIndex::AssertionType:
        instruction.type = InstructionIndex::AssertionPropertyType;
        break;
      case InstructionIndex::AssertionTypeStrictAny:
        instruction.type = InstructionIndex::AssertionPropertyTypeStrictAny;
        break;
      default:
        break;
    }
  }

  output.push_back(std::move(instruction));
  return false;
}

inline auto postprocess(std::vector<Instructions> &targets,
                        const Tweaks &tweaks, const bool uses_dynamic_scopes)
    -> void {
  std::vector<TargetStatistics> statistics;
  statistics.reserve(targets.size());
  for (const auto &target : targets) {
    TargetStatistics entry{.count = 0,
                           .jump_targets = {},
                           .requires_empty_instance_location = false};
    collect_statistics(target, entry);
    statistics.push_back(std::move(entry));
  }

  bool changed{true};
  while (changed) {
    changed = false;

    for (std::size_t current_target_index = 0;
         current_target_index < targets.size(); ++current_target_index) {
      auto &target{targets[current_target_index]};
      auto &current_stats{statistics[current_target_index]};

      std::vector<Instructions *> worklist;
      std::vector<std::pair<Instructions *, std::size_t>> stack;
      stack.emplace_back(&target, 0);

      while (!stack.empty()) {
        auto current{stack.back().first};
        auto index{stack.back().second};
        stack.pop_back();

        while (index < current->size() && (*current)[index].children.empty()) {
          ++index;
        }

        if (index < current->size()) {
          stack.emplace_back(current, index + 1);
          stack.emplace_back(&(*current)[index].children, 0);
        } else {
          worklist.push_back(current);
        }
      }

      for (auto *current : worklist) {
        Instructions result;
        result.reserve(current->size());

        for (auto &instruction : *current) {
          if (transform_instruction(instruction, result, targets, statistics,
                                    current_stats, tweaks, uses_dynamic_scopes))
            changed = true;
        }

        *current = std::move(result);
      }
    }
  }
}

} // namespace sourcemeta::blaze

#endif
