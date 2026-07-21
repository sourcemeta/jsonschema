#ifndef SOURCEMETA_BLAZE_COMPILER_POSTPROCESS_H
#define SOURCEMETA_BLAZE_COMPILER_POSTPROCESS_H

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <algorithm>
#include <cstddef>
#include <string> // std::string
#include <tuple>  // std::tuple
#include <unordered_map>
#include <unordered_set> // std::unordered_set
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
    case InstructionIndex::LoopPropertiesRegex:
    case InstructionIndex::LoopPropertiesStartsWith:
    case InstructionIndex::LoopPropertiesExcept:
    case InstructionIndex::LoopKeys:
    case InstructionIndex::LoopItems:
    case InstructionIndex::LoopItemsFrom:
    // Deliberately absent are the loops that still do something with no
    // children: the one carrying a match count still asserts that the array
    // size falls within that range, and the ones that mark their instance
    // location as evaluated still carry that side effect for a later
    // `unevaluatedProperties` or `unevaluatedItems` to consult. Dropping any of
    // them would discard those semantics
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
    if (!instruction.relative_instance_location.empty() &&
        std::ranges::all_of(
            instruction.relative_instance_location,
            [](const auto &token) -> auto { return token.is_property(); })) {
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

inline auto duplicate_metadata(Instruction &instruction,
                               std::vector<InstructionExtra> &extra) -> void {
  const auto new_index{extra.size()};
  auto source_extra{extra[instruction.extra_index]};
  extra.push_back(std::move(source_extra));
  instruction.extra_index = new_index;
  for (auto &child : instruction.children) {
    duplicate_metadata(child, extra);
  }
}

// Whether an instruction relays its children without pushing its own schema
// location onto the evaluation path, leaving them anchored on its own parent
inline auto is_pass_through_instruction(const InstructionIndex type) noexcept
    -> bool {
  switch (type) {
    case InstructionIndex::ControlGroup:
    case InstructionIndex::ControlGroupWhenDefines:
    case InstructionIndex::ControlGroupWhenDefinesDirect:
    case InstructionIndex::ControlGroupWhenType:
    case InstructionIndex::ControlEvaluate:
      return true;
    default:
      return false;
  }
}

// Prefix every instruction that anchors its schema location on the point an
// inlined target is spliced into. That is the top instruction itself, plus
// the instructions that evaluation reaches without a location of their own in
// between: the consequence children of a conditional, which re-anchor on the
// conditional's parent, and the children of pass-through instructions.
// Condition children and ordinary nested children stay relative to their
// parent instruction and must be left alone
inline auto rebase_anchored(Instruction &instruction,
                            std::vector<InstructionExtra> &extra,
                            const sourcemeta::core::Pointer &schema_prefix)
    -> void {
  extra[instruction.extra_index].relative_schema_location =
      schema_prefix.concat(
          extra[instruction.extra_index].relative_schema_location);

  if (instruction.type == InstructionIndex::LogicalCondition) {
    const auto &value{std::get<ValueIndexPair>(instruction.value)};
    for (std::size_t index = value.first; index < instruction.children.size();
         ++index) {
      rebase_anchored(instruction.children[index], extra, schema_prefix);
    }
  } else if (is_pass_through_instruction(instruction.type)) {
    for (auto &child : instruction.children) {
      rebase_anchored(child, extra, schema_prefix);
    }
  }
}

inline auto rebase(Instruction &instruction,
                   std::vector<InstructionExtra> &extra,
                   const sourcemeta::core::Pointer &schema_prefix,
                   const sourcemeta::core::Pointer &instance_prefix) -> void {
  instruction.relative_instance_location =
      instance_prefix.concat(instruction.relative_instance_location);
  rebase_anchored(instruction, extra, schema_prefix);
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
                      std::vector<InstructionExtra> &extra,
                      const std::vector<Instructions> &targets,
                      const std::vector<TargetStatistics> &statistics,
                      TargetStatistics &current_stats, const Tweaks &tweaks,
                      const bool uses_dynamic_scopes, const bool positional)
    -> bool {
  // A positional owner pairs each child with an entry of its own data by
  // index, and inlining a jump expands one child into many, which would
  // re-pair the rest
  if (!positional && instruction.type == InstructionIndex::ControlJump) {
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

      const auto schema_prefix{
          extra[instruction.extra_index].relative_schema_location};
      for (auto target_instruction : targets[jump_target_index]) {
        duplicate_metadata(target_instruction, extra);
        rebase(target_instruction, extra, schema_prefix,
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
      const auto new_extra_index{extra.size()};
      auto &instruction_meta{extra[instruction.extra_index]};
      auto &child_meta{extra[child.extra_index]};
      extra.push_back(
          {.relative_schema_location =
               instruction_meta.relative_schema_location.concat(
                   child_meta.relative_schema_location),
           .keyword_location = std::move(child_meta.keyword_location),
           .schema_resource = child_meta.schema_resource});
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesTypeStrict,
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .value = std::move(child.value),
                      .children = {},
                      .extra_index = new_extra_index});
      return true;
    }

    if (child.type == InstructionIndex::AssertionType) {
      const auto new_extra_index{extra.size()};
      auto &instruction_meta{extra[instruction.extra_index]};
      auto &child_meta{extra[child.extra_index]};
      extra.push_back(
          {.relative_schema_location =
               instruction_meta.relative_schema_location.concat(
                   child_meta.relative_schema_location),
           .keyword_location = std::move(child_meta.keyword_location),
           .schema_resource = child_meta.schema_resource});
      output.push_back(Instruction{.type = InstructionIndex::LoopPropertiesType,
                                   .relative_instance_location = std::move(
                                       instruction.relative_instance_location),
                                   .value = std::move(child.value),
                                   .children = {},
                                   .extra_index = new_extra_index});
      return true;
    }

    if (child.type == InstructionIndex::AssertionTypeStrictAny) {
      const auto new_extra_index{extra.size()};
      auto &instruction_meta{extra[instruction.extra_index]};
      auto &child_meta{extra[child.extra_index]};
      extra.push_back(
          {.relative_schema_location =
               instruction_meta.relative_schema_location.concat(
                   child_meta.relative_schema_location),
           .keyword_location = std::move(child_meta.keyword_location),
           .schema_resource = child_meta.schema_resource});
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesTypeStrictAny,
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .value = std::move(child.value),
                      .children = {},
                      .extra_index = new_extra_index});
      return true;
    }
  }

  if (instruction.type == InstructionIndex::LoopPropertiesEvaluate &&
      instruction.children.size() == 1) {
    auto &child{instruction.children.front()};
    if (child.type == InstructionIndex::AssertionTypeStrict) {
      const auto new_extra_index{extra.size()};
      auto &instruction_meta{extra[instruction.extra_index]};
      auto &child_meta{extra[child.extra_index]};
      extra.push_back(
          {.relative_schema_location =
               instruction_meta.relative_schema_location.concat(
                   child_meta.relative_schema_location),
           .keyword_location = std::move(child_meta.keyword_location),
           .schema_resource = child_meta.schema_resource});
      output.push_back(Instruction{
          .type = InstructionIndex::LoopPropertiesTypeStrictEvaluate,
          .relative_instance_location =
              std::move(instruction.relative_instance_location),
          .value = std::move(child.value),
          .children = {},
          .extra_index = new_extra_index});
      return true;
    }

    if (child.type == InstructionIndex::AssertionType) {
      const auto new_extra_index{extra.size()};
      auto &instruction_meta{extra[instruction.extra_index]};
      auto &child_meta{extra[child.extra_index]};
      extra.push_back(
          {.relative_schema_location =
               instruction_meta.relative_schema_location.concat(
                   child_meta.relative_schema_location),
           .keyword_location = std::move(child_meta.keyword_location),
           .schema_resource = child_meta.schema_resource});
      output.push_back(
          Instruction{.type = InstructionIndex::LoopPropertiesTypeEvaluate,
                      .relative_instance_location =
                          std::move(instruction.relative_instance_location),
                      .value = std::move(child.value),
                      .children = {},
                      .extra_index = new_extra_index});
      return true;
    }

    if (child.type == InstructionIndex::AssertionTypeStrictAny) {
      const auto new_extra_index{extra.size()};
      auto &instruction_meta{extra[instruction.extra_index]};
      auto &child_meta{extra[child.extra_index]};
      extra.push_back(
          {.relative_schema_location =
               instruction_meta.relative_schema_location.concat(
                   child_meta.relative_schema_location),
           .keyword_location = std::move(child_meta.keyword_location),
           .schema_resource = child_meta.schema_resource});
      output.push_back(Instruction{
          .type = InstructionIndex::LoopPropertiesTypeStrictAnyEvaluate,
          .relative_instance_location =
              std::move(instruction.relative_instance_location),
          .value = std::move(child.value),
          .children = {},
          .extra_index = new_extra_index});
      return true;
    }
  }

  if (is_parent_to_children_instruction(instruction.type)) {
    convert_to_property_type_assertions(instruction.children);
  }

  if (!instruction.relative_instance_location.empty() &&
      std::ranges::all_of(
          instruction.relative_instance_location,
          [](const auto &token) -> auto { return token.is_property(); })) {
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
                        std::vector<InstructionExtra> &extra,
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

      std::vector<std::pair<Instructions *, Instruction *>> worklist;
      std::vector<std::tuple<Instructions *, std::size_t, Instruction *>> stack;
      stack.emplace_back(&target, 0, nullptr);

      while (!stack.empty()) {
        auto [current, index, owner] = stack.back();
        stack.pop_back();

        while (index < current->size() && (*current)[index].children.empty()) {
          ++index;
        }

        if (index < current->size()) {
          stack.emplace_back(current, index + 1, owner);
          stack.emplace_back(&(*current)[index].children, 0,
                             &(*current)[index]);
        } else {
          worklist.emplace_back(current, owner);
        }
      }

      for (const auto &[current, owner] : worklist) {
        Instructions result;
        result.reserve(current->size());

        // A conditional stores the indices where its branches begin, so when
        // this pass drops or expands any child, those indices must be remapped
        // onto the rewritten list
        const bool remap{owner != nullptr &&
                         owner->type == InstructionIndex::LogicalCondition};
        std::vector<std::size_t> boundaries;
        if (remap) {
          boundaries.reserve(current->size() + 1);
        }

        // The children of a conditional span its mutually exclusive
        // condition, consequence, and alternative segments, and the children
        // of a disjunction are alternative branches, so an instruction in one
        // of them cannot subsume an instruction in another. Every other
        // children list is a conjunction evaluated as a whole, where
        // subsumption holds
        // A fused simple-properties instruction pairs its entries with its
        // children by index, so its children may neither be dropped, expanded,
        // nor subsumed by each other, as each one applies to a different
        // property despite them all sharing an empty instance location
        const bool positional{
            owner != nullptr &&
            owner->type == InstructionIndex::AssertionObjectPropertiesSimple};

        const bool disjoint{
            positional || (owner != nullptr &&
                           (owner->type == InstructionIndex::LogicalCondition ||
                            owner->type == InstructionIndex::LogicalOr ||
                            owner->type == InstructionIndex::LogicalXor))};

        // A fused simple-properties instruction only enforces that a property
        // is present for the entries it marks as required, so only those may
        // subsume a sibling presence assertion. Every fused instruction, on
        // the other hand, rejects a non-object outright, so any of them can
        // subsume a sibling object type assertion. None of that says anything
        // about the rest of the instance, so a fused instruction may only
        // subsume siblings that apply to the very same instance location
        std::vector<std::pair<sourcemeta::core::Pointer,
                              std::unordered_set<std::string>>>
            fusions;
        if (!disjoint) {
          for (const auto &instruction : *current) {
            if (instruction.type !=
                InstructionIndex::AssertionObjectPropertiesSimple) {
              continue;
            }

            auto match{std::ranges::find_if(
                fusions, [&instruction](const auto &entry) -> bool {
                  return entry.first == instruction.relative_instance_location;
                })};
            if (match == fusions.end()) {
              fusions.emplace_back(instruction.relative_instance_location,
                                   std::unordered_set<std::string>{});
              match = std::prev(fusions.end());
            }

            for (const auto &entry :
                 std::get<ValueObjectProperties>(instruction.value)) {
              if (std::get<2>(entry)) {
                match->second.insert(std::get<0>(entry));
              }
            }
          }
        }

        // A positional owner pairs child N with entry N of its own data, and
        // every entry past the last child only carries a presence requirement.
        // Dropping a child therefore means moving its entry onto that tail, so
        // the entries left in front stay paired with the children that survived
        std::vector<std::size_t> surviving;
        std::vector<std::size_t> dropped;
        std::size_t child_index{0};

        for (auto &instruction : *current) {
          if (remap) {
            boundaries.push_back(result.size());
          }

          const auto result_size{result.size()};
          const auto positional_index{child_index};
          child_index += 1;

          const auto fusion{std::ranges::find_if(
              fusions, [&instruction](const auto &entry) -> bool {
                return entry.first == instruction.relative_instance_location;
              })};

          if (fusion != fusions.cend()) {
            switch (instruction.type) {
              case InstructionIndex::AssertionDefinesAllStrict:
              case InstructionIndex::AssertionDefinesAll: {
                const auto &value{std::get<ValueStringSet>(instruction.value)};
                bool all_covered{true};
                for (const auto &property : value) {
                  if (!fusion->second.contains(property.first)) {
                    all_covered = false;
                    break;
                  }
                }
                if (all_covered) {
                  changed = true;
                  continue;
                }
                break;
              }
              case InstructionIndex::AssertionDefinesStrict:
              case InstructionIndex::AssertionDefines: {
                const auto &value{std::get<ValueProperty>(instruction.value)};
                if (fusion->second.contains(value.first)) {
                  changed = true;
                  continue;
                }
                break;
              }
              case InstructionIndex::AssertionTypeStrict:
              case InstructionIndex::AssertionType:
                if (std::get<ValueType>(instruction.value) ==
                    sourcemeta::core::JSON::Type::Object) {
                  changed = true;
                  continue;
                }
                break;
              default:
                break;
            }
          }

          if (transform_instruction(instruction, result, extra, targets,
                                    statistics, current_stats, tweaks,
                                    uses_dynamic_scopes, positional))
            changed = true;

          if (positional) {
            if (result.size() == result_size) {
              dropped.push_back(positional_index);
            } else {
              surviving.push_back(positional_index);
            }
          }
        }

        if (positional && !dropped.empty()) {
          auto &entries{std::get<ValueObjectProperties>(owner->value)};
          ValueObjectProperties reordered;
          for (const auto index : surviving) {
            reordered.push_back(entries[index]);
          }

          for (const auto index : dropped) {
            reordered.push_back(entries[index]);
          }

          for (auto index = child_index; index < entries.size(); index++) {
            reordered.push_back(entries[index]);
          }

          entries = std::move(reordered);
        }

        if (remap) {
          boundaries.push_back(result.size());
          auto &cursors{std::get<ValueIndexPair>(owner->value)};
          const auto then_start{boundaries[cursors.first]};
          if (then_start == 0) {
            // The condition dropped to nothing, so it is always true: the then
            // branch applies unconditionally and the else branch is dead. Keep
            // only the then instructions and record an empty condition with no
            // else, which the evaluator runs as the consequence
            const auto else_start{cursors.second > 0
                                      ? boundaries[cursors.second]
                                      : result.size()};
            result.erase(result.begin() +
                             static_cast<std::ptrdiff_t>(else_start),
                         result.end());
            cursors.first = 0;
            cursors.second = 0;
          } else {
            cursors.first = then_start;
            if (cursors.second > 0) {
              cursors.second = boundaries[cursors.second];
            }
          }
        }

        *current = std::move(result);
      }
    }
  }
}

} // namespace sourcemeta::blaze

#endif
