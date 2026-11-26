#ifndef SOURCEMETA_BLAZE_COMPILER_POSTPROCESS_H
#define SOURCEMETA_BLAZE_COMPILER_POSTPROCESS_H

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/uri.h>

#include "compile_helpers.h"

#include <algorithm> // std::ranges::all_of, std::ranges::find, std::max, std::min
#include <array>     // std::array
#include <cstddef>   // std::size_t
#include <cstdint>   // std::int64_t
#include <limits>    // std::numeric_limits
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

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
    case InstructionIndex::LogicalConditionEvaluate:
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
    case InstructionIndex::ControlFrame:
    case InstructionIndex::ControlResource:
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
                               InstructionExtras &extra) -> void {
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
    case InstructionIndex::ControlFrame:
    case InstructionIndex::ControlResource:
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
inline auto rebase_anchored(Instruction &instruction, InstructionExtras &extra,
                            const sourcemeta::core::Pointer &schema_prefix)
    -> void {
  extra[instruction.extra_index].relative_schema_location =
      schema_prefix.concat(
          extra[instruction.extra_index].relative_schema_location);

  if (is_conditional(instruction.type)) {
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

inline auto rebase(Instruction &instruction, InstructionExtras &extra,
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
    } else if (instruction.type == InstructionIndex::ControlResourceJump) {
      // Counted even though this one never inlines, as what it reaches is what
      // decides whether a target loops back on itself
      statistics
          .jump_targets[std::get<ValueIndexPair>(instruction.value).first]++;
    }

    if (instruction.type == InstructionIndex::ControlGroupWhenDefinesDirect ||
        instruction.type == InstructionIndex::ControlGroupWhenType ||
        instruction.type == InstructionIndex::ControlFrame ||
        instruction.type == InstructionIndex::ControlResource) {
      statistics.requires_empty_instance_location = true;
    }

    collect_statistics(instruction.children, statistics);
  }
}

inline auto instruction_parent_location(const Instruction &instruction,
                                        const InstructionExtras &extra)
    -> std::optional<std::pair<sourcemeta::core::Pointer, std::string>> {
  const auto &metadata{extra[instruction.extra_index]};
  if (metadata.relative_schema_location.empty()) {
    return std::nullopt;
  }

  auto keyword_location{sourcemeta::core::URI{metadata.keyword_location}};
  auto keyword_pointer{sourcemeta::core::fragment_to_pointer(keyword_location)};
  if (!keyword_pointer.has_value() || keyword_pointer->empty()) {
    return std::nullopt;
  }

  auto parent_pointer{keyword_pointer->initial()};
  keyword_location.fragment(sourcemeta::core::to_string(parent_pointer));
  return std::pair{metadata.relative_schema_location.initial(),
                   keyword_location.recompose()};
}

inline auto is_numeric_integer_type_check(const Instruction &instruction)
    -> bool {
  return (instruction.type == InstructionIndex::AssertionType ||
          instruction.type == InstructionIndex::AssertionTypeStrict) &&
         std::get<ValueType>(instruction.value) ==
             sourcemeta::core::JSON::Type::Integer;
}

// A multiple of one admits exactly the integers, so it bounds a number the way
// an integer type check does, without saying anything about the type
inline auto is_numeric_integral_multiple_check(const Instruction &instruction)
    -> bool {
  if (instruction.type != InstructionIndex::AssertionDivisible) {
    return false;
  }

  const auto &divisor{std::get<ValueJSON>(instruction.value)};
  if (!divisor.is_integral()) {
    return false;
  }

  // A real or decimal may spell an integer too large to represent, and such a
  // divisor is never one, so a failed conversion just means no fusion
  try {
    return divisor.as_integer() == 1;
  } catch (const sourcemeta::core::JSONIntegerRangeError &) {
    return false;
  }
}

inline auto is_numeric_bound_check(const Instruction &instruction) -> bool {
  return instruction.type == InstructionIndex::AssertionGreaterEqual ||
         instruction.type == InstructionIndex::AssertionGreater ||
         instruction.type == InstructionIndex::AssertionLessEqual ||
         instruction.type == InstructionIndex::AssertionLess;
}

inline auto merge_integer_bound(const Instruction &instruction,
                                std::optional<std::int64_t> &minimum,
                                std::optional<std::int64_t> &maximum) -> bool {
  if (!is_numeric_bound_check(instruction)) {
    return false;
  }

  const auto &bound{std::get<ValueJSON>(instruction.value)};
  if (!bound.is_integer()) {
    return false;
  }

  const auto value{bound.to_integer()};
  if (instruction.type == InstructionIndex::AssertionGreaterEqual) {
    minimum = minimum.has_value() ? std::max(minimum.value(), value) : value;
  } else if (instruction.type == InstructionIndex::AssertionGreater) {
    // Tightening the bound past the representable range would wrap around, so
    // leave such a bound to the standalone instruction that can still hold it
    if (value == std::numeric_limits<std::int64_t>::max()) {
      return false;
    }

    const auto adjusted{value + 1};
    minimum =
        minimum.has_value() ? std::max(minimum.value(), adjusted) : adjusted;
  } else if (instruction.type == InstructionIndex::AssertionLessEqual) {
    maximum = maximum.has_value() ? std::min(maximum.value(), value) : value;
  } else if (instruction.type == InstructionIndex::AssertionLess) {
    if (value == std::numeric_limits<std::int64_t>::min()) {
      return false;
    }

    const auto adjusted{value - 1};
    maximum =
        maximum.has_value() ? std::min(maximum.value(), adjusted) : adjusted;
  } else {
    return false;
  }

  return true;
}
inline auto fuse_numeric_bounds(Instructions &instructions,
                                InstructionExtras &extra) -> bool {
  for (std::size_t type_index = 0; type_index < instructions.size();
       ++type_index) {
    const auto &type_instruction{instructions[type_index]};
    const auto anchors_on_type{is_numeric_integer_type_check(type_instruction)};
    if (!anchors_on_type &&
        !is_numeric_integral_multiple_check(type_instruction)) {
      continue;
    }

    const auto parent{instruction_parent_location(type_instruction, extra)};
    if (!parent.has_value()) {
      continue;
    }

    const auto &type_metadata{extra[type_instruction.extra_index]};
    std::vector<std::size_t> bound_indices;
    std::optional<std::int64_t> minimum;
    std::optional<std::int64_t> maximum;
    std::optional<std::size_t> minimum_index;

    for (std::size_t index = 0; index < instructions.size(); ++index) {
      const auto &candidate{instructions[index]};
      if (!is_numeric_bound_check(candidate) ||
          candidate.relative_instance_location !=
              type_instruction.relative_instance_location ||
          extra[candidate.extra_index].schema_resource !=
              type_metadata.schema_resource ||
          instruction_parent_location(candidate, extra) != parent) {
        continue;
      }

      const auto previous_minimum{minimum};
      if (!merge_integer_bound(candidate, minimum, maximum)) {
        continue;
      }

      if (minimum != previous_minimum) {
        minimum_index = index;
      }

      bound_indices.push_back(index);
    }

    if (bound_indices.empty() ||
        (!minimum.has_value() && !maximum.has_value())) {
      continue;
    }

    const auto is_strict{type_instruction.type ==
                         InstructionIndex::AssertionTypeStrict};
    InstructionIndex fused_type;
    Value fused_value;

    if (!anchors_on_type) {
      // Without a type check to absorb there is no lower-bound-only form to
      // fall back on, as the instruction has to carry both ends of the range
      if (!minimum.has_value() || !maximum.has_value()) {
        continue;
      }

      fused_type = InstructionIndex::AssertionNumberIntegerBounded;
      fused_value = ValueIntegerBounds{minimum.value(), maximum.value()};
    } else if (minimum.has_value() && maximum.has_value()) {
      fused_type = is_strict
                       ? InstructionIndex::AssertionTypeIntegerBoundedStrict
                       : InstructionIndex::AssertionTypeIntegerBounded;
      fused_value = ValueIntegerBounds{minimum.value(), maximum.value()};
    } else if (minimum.has_value()) {
      fused_type = is_strict
                       ? InstructionIndex::AssertionTypeIntegerLowerBoundStrict
                       : InstructionIndex::AssertionTypeIntegerLowerBound;
      fused_value = ValueIntegerBounds{minimum.value(), 0};
    } else {
      continue;
    }

    const auto first_index{
        std::min(type_index, *std::ranges::min_element(bound_indices))};
    const auto &minimum_metadata{
        extra[instructions[minimum_index.value()].extra_index]};
    const auto extra_index{extra.size()};
    extra.push_back(
        {.relative_schema_location = minimum_metadata.relative_schema_location,
         .keyword_location = minimum_metadata.keyword_location,
         .schema_resource = type_metadata.schema_resource,
         .vocabulary = type_metadata.vocabulary,
         .message = type_metadata.message,
         .message_detail = type_metadata.message_detail});

    Instructions result;
    result.reserve(instructions.size() - bound_indices.size());
    for (std::size_t index = 0; index < instructions.size(); ++index) {
      if (index == first_index) {
        result.push_back({.type = fused_type,
                          .relative_instance_location =
                              type_instruction.relative_instance_location,
                          .value = fused_value,
                          .children = {},
                          .extra_index = extra_index});
      }

      if (index == type_index ||
          std::ranges::find(bound_indices, index) != bound_indices.cend()) {
        continue;
      }

      result.push_back(std::move(instructions[index]));
    }

    instructions = std::move(result);
    return true;
  }

  return false;
}

// An instruction that records what it evaluated, which a later
// `unevaluatedProperties` or `unevaluatedItems` consults, cannot be moved into
// a property loop that may decide a child without dispatching to it
inline auto marks_evaluation(const InstructionIndex type) noexcept -> bool {
  switch (type) {
    case InstructionIndex::Evaluate:
    case InstructionIndex::AssertionPropertyTypeEvaluate:
    case InstructionIndex::AssertionPropertyTypeStrictEvaluate:
    case InstructionIndex::AssertionPropertyTypeStrictAnyEvaluate:
    case InstructionIndex::AssertionArrayPrefixEvaluate:
    case InstructionIndex::LogicalNotEvaluate:
    case InstructionIndex::LogicalOrEvaluate:
    case InstructionIndex::LogicalXorEvaluate:
    case InstructionIndex::LogicalConditionEvaluate:
    case InstructionIndex::LoopPropertiesEvaluate:
    case InstructionIndex::LoopPropertiesTypeEvaluate:
    case InstructionIndex::LoopPropertiesTypeStrictEvaluate:
    case InstructionIndex::LoopPropertiesTypeStrictAnyEvaluate:
    case InstructionIndex::LoopPropertiesUnevaluated:
    case InstructionIndex::LoopPropertiesUnevaluatedExcept:
    case InstructionIndex::LoopItemsUnevaluated:
    case InstructionIndex::ControlEvaluate:
      return true;
    default:
      return false;
  }
}

inline auto fuses_into_property_loop(const Instruction &instruction) -> bool {
  if (marks_evaluation(instruction.type) || is_annotation(instruction.type)) {
    return false;
  }

  return std::ranges::all_of(instruction.children, fuses_into_property_loop);
}

// Sibling instructions that all decide something about the properties of the
// same object, which one property loop could decide in a single pass over it.
// The names are what the merged loop would match on, `required` are the ones a
// presence assertion that stays behind already proves are there, and the two
// flags are what rules a set out: a closed loop among the siblings, or an
// absorbed instruction carrying a side effect that a loop may skip
struct ObjectPropertiesFusion {
  sourcemeta::core::Pointer instance_location;
  std::vector<std::size_t> indices;
  std::vector<ValueString> names;
  std::unordered_set<ValueString> required;
  std::size_t loops;
  std::size_t guards;
  std::size_t accesses;
  bool closed;
  bool side_effects;
};

inline auto
object_properties_fusion_at(std::vector<ObjectPropertiesFusion> &fusions,
                            const sourcemeta::core::Pointer &instance_location)
    -> ObjectPropertiesFusion & {
  for (auto &fusion : fusions) {
    if (fusion.instance_location == instance_location) {
      return fusion;
    }
  }

  fusions.push_back({.instance_location = instance_location,
                     .indices = {},
                     .names = {},
                     .required = {},
                     .loops = 0,
                     .guards = 0,
                     .accesses = 0,
                     .closed = false,
                     .side_effects = false});
  return fusions.back();
}

inline auto object_properties_fusion_name(ObjectPropertiesFusion &fusion,
                                          const ValueString &name) -> void {
  if (std::ranges::find(fusion.names, name) == fusion.names.cend()) {
    fusion.names.push_back(name);
  }
}

// Group the instructions of one conjunction by the object they decide
// something about, without rewriting any of them
inline auto
collect_object_properties_fusions(const Instructions &instructions,
                                  std::vector<ObjectPropertiesFusion> &fusions)
    -> void {
  for (std::size_t index = 0; index < instructions.size(); ++index) {
    const auto &instruction{instructions[index]};
    switch (instruction.type) {
      case InstructionIndex::LoopPropertiesMatch: {
        auto &fusion{object_properties_fusion_at(
            fusions, instruction.relative_instance_location)};
        for (const auto &entry :
             std::get<ValueNamedIndexes>(instruction.value).first) {
          object_properties_fusion_name(fusion, std::get<2>(entry));
        }

        fusion.indices.push_back(index);
        fusion.loops += 1;
        fusion.side_effects |= !fuses_into_property_loop(instruction);
        break;
      }

      // Closedness is a different contract, so a set holding one of these is
      // left alone rather than merged into an open loop
      case InstructionIndex::LoopPropertiesMatchClosed:
        object_properties_fusion_at(fusions,
                                    instruction.relative_instance_location)
            .closed = true;
        break;

      // A presence assertion is not absorbed, as a merged loop cannot enforce
      // presence without counting what it matched. It stays a sibling, and
      // only says which of the merged names are resolved anyway
      case InstructionIndex::AssertionDefines:
      case InstructionIndex::AssertionDefinesStrict:
        object_properties_fusion_at(fusions,
                                    instruction.relative_instance_location)
            .required.insert(std::get<ValueProperty>(instruction.value).first);
        break;

      case InstructionIndex::AssertionDefinesAll:
      case InstructionIndex::AssertionDefinesAllStrict: {
        auto &fusion{object_properties_fusion_at(
            fusions, instruction.relative_instance_location)};
        for (const auto &property :
             std::get<ValueStringSet>(instruction.value)) {
          fusion.required.insert(property.first);
        }

        break;
      }

      // A presence guard becomes the merged loop's own match on that name, and
      // its children become what that name's entry holds
      case InstructionIndex::ControlGroupWhenDefinesDirect: {
        auto &fusion{object_properties_fusion_at(
            fusions, instruction.relative_instance_location)};
        object_properties_fusion_name(
            fusion, std::get<ValueProperty>(instruction.value).first);
        fusion.indices.push_back(index);
        fusion.guards += 1;
        fusion.side_effects |= !fuses_into_property_loop(instruction);
        break;
      }

      default: {
        if (instruction.relative_instance_location.empty() ||
            !instruction.relative_instance_location.back().is_property()) {
          break;
        }

        auto &fusion{object_properties_fusion_at(
            fusions, instruction.relative_instance_location.initial())};
        object_properties_fusion_name(
            fusion,
            instruction.relative_instance_location.back().to_property());
        fusion.indices.push_back(index);
        fusion.accesses += 1;
        fusion.side_effects |= !fuses_into_property_loop(instruction);
        break;
      }
    }
  }
}

// How many of the merged names a sibling presence assertion already proves are
// there, which is what the loop versus unroll balance counts as required
inline auto
object_properties_fusion_required(const ObjectPropertiesFusion &fusion)
    -> std::size_t {
  std::size_t result{0};
  for (const auto &name : fusion.names) {
    if (fusion.required.contains(name)) {
      result += 1;
    }
  }

  return result;
}

// Merging is worth doing when it collapses more than one pass over the object
// and when the merged counts ask for a loop the same way the compiler asks for
// one.
//
// A set has to hold a loop already. The numeric balance is only the core of
// `properties_as_loop`: the rest of it asks whether the subschema is reachable
// from a disjunction and whether any of its properties carries `enum` or
// `const`, and unrolls whatever the counts say when both hold, so that one
// comparison can decide the whole branch. That lives in the schema and in the
// reference graph, neither of which this pass can see. Insisting on a loop
// means at least one of the subschemas feeding the set was judged to want one,
// and that the pass never turns a deliberate unroll into a loop
inline auto is_object_properties_fusable(const ObjectPropertiesFusion &fusion)
    -> bool {
  return !fusion.closed && !fusion.side_effects && fusion.loops > 0 &&
         fusion.loops + fusion.guards + fusion.accesses > 1 &&
         // Everything absorbed navigates from the object the loop walks, so a
         // set deeper than the instructions' own base would need its children
         // rebased onto the loop's target first
         fusion.instance_location.empty() &&
         favours_property_loop(fusion.names.size(),
                               object_properties_fusion_required(fusion));
}

// The entry a name's work belongs to, creating it if the merged loop does not
// hold one yet. The entries record which child applies to them, so a new one
// goes at the end of the children and records that position
inline auto object_properties_fusion_entry(
    std::vector<std::pair<ValueString, ValueUnsignedInteger>> &names,
    Instructions &children, const ValueString &name,
    const std::size_t extra_index) -> Instructions & {
  for (const auto &entry : names) {
    if (entry.first == name) {
      return children[entry.second].children;
    }
  }

  names.emplace_back(name, children.size());
  children.push_back({.type = InstructionIndex::ControlGroup,
                      .relative_instance_location = {},
                      .value = ValueNone{},
                      .children = {},
                      .extra_index = extra_index});
  return children.back().children;
}

// Collapse the sibling instructions that each walk or resolve into the same
// object into one loop over it.
//
// The merged loop pushes nothing of its own onto the evaluation path, as the
// siblings it absorbs were all reached from the instructions' own base and go
// on reporting the schema location they reported before. What the loop it
// absorbed used to push becomes a prefix on that loop's own entries instead,
// so that every instruction still answers for where it came from
inline auto fuse_object_properties(Instructions &instructions,
                                   InstructionExtras &extra) -> bool {
  std::vector<ObjectPropertiesFusion> fusions;
  collect_object_properties_fusions(instructions, fusions);

  for (const auto &fusion : fusions) {
    if (!is_object_properties_fusable(fusion)) {
      continue;
    }

    std::vector<std::pair<ValueString, ValueUnsignedInteger>> names;
    Instructions children;
    std::optional<std::size_t> source_index;

    for (const auto index : fusion.indices) {
      auto &instruction{instructions[index]};
      if (instruction.type == InstructionIndex::LoopPropertiesMatch) {
        if (!source_index.has_value()) {
          source_index = instruction.extra_index;
        }

        const auto prefix{
            extra[instruction.extra_index].relative_schema_location};
        for (const auto &entry :
             std::get<ValueNamedIndexes>(instruction.value).first) {
          auto &entry_children{object_properties_fusion_entry(
              names, children, std::get<2>(entry), instruction.extra_index)};
          auto &group{instruction.children[std::get<1>(entry)]};
          for (auto &child : group.children) {
            duplicate_metadata(child, extra);
            rebase_anchored(child, extra, prefix);
            entry_children.push_back(std::move(child));
          }
        }
      } else if (instruction.type ==
                 InstructionIndex::ControlGroupWhenDefinesDirect) {
        auto &entry_children{object_properties_fusion_entry(
            names, children, std::get<ValueProperty>(instruction.value).first,
            instruction.extra_index)};
        for (auto &child : instruction.children) {
          entry_children.push_back(std::move(child));
        }
      } else {
        assert(instruction.relative_instance_location.size() == 1);
        auto &entry_children{object_properties_fusion_entry(
            names, children,
            instruction.relative_instance_location.back().to_property(),
            instruction.extra_index)};
        entry_children.push_back(std::move(instruction));
      }
    }

    assert(source_index.has_value());
    assert(!names.empty());
    const auto extra_index{extra.size()};
    const auto &source_metadata{extra[source_index.value()]};
    extra.push_back({.relative_schema_location = {},
                     .keyword_location = source_metadata.keyword_location,
                     .schema_resource = source_metadata.schema_resource,
                     .vocabulary = source_metadata.vocabulary,
                     .message = source_metadata.message,
                     .message_detail = source_metadata.message_detail});

    Instruction merged{.type = InstructionIndex::LoopPropertiesMatch,
                       .relative_instance_location = {},
                       .value = to_named_indexes(names),
                       .children = std::move(children),
                       .extra_index = extra_index};

    // The merged loop stands where the first sibling it absorbed stood, and
    // every other one goes away
    Instructions result;
    result.reserve(instructions.size() - fusion.indices.size() + 1);
    for (std::size_t index = 0; index < fusion.indices.front(); ++index) {
      result.push_back(std::move(instructions[index]));
    }

    result.push_back(std::move(merged));

    for (std::size_t index = fusion.indices.front();
         index < instructions.size(); ++index) {
      if (std::ranges::find(fusion.indices, index) != fusion.indices.cend()) {
        continue;
      }

      result.push_back(std::move(instructions[index]));
    }

    instructions = std::move(result);
    return true;
  }

  return false;
}

// Whether a target can be reached while a disjunction or a conditional is
// being decided. A presence assertion is an early exit: a branch whose
// `required` is unsatisfied fails on one lookup and its property loop never
// runs, so folding the requirement into the loop would make that branch pay
// for the whole walk first. Jumps are followed through, as a target a branch
// reaches can reach others, and the marks are taken before any inlining, which
// only ever splices a target into its caller and so can never widen this
inline auto collect_jump_targets(const Instructions &instructions,
                                 const bool branching,
                                 std::vector<std::size_t> &reached,
                                 std::vector<std::size_t> &reached_branching)
    -> void {
  for (const auto &instruction : instructions) {
    if (instruction.type == InstructionIndex::ControlJump) {
      const auto target{std::get<ValueUnsignedInteger>(instruction.value)};
      reached.push_back(target);
      if (branching) {
        reached_branching.push_back(target);
      }
    } else if (instruction.type == InstructionIndex::ControlResourceJump) {
      const auto target{std::get<ValueIndexPair>(instruction.value).first};
      reached.push_back(target);
      if (branching) {
        reached_branching.push_back(target);
      }
    }

    collect_jump_targets(instruction.children,
                         branching || is_disjunction(instruction.type) ||
                             is_conditional(instruction.type),
                         reached, reached_branching);
  }
}

inline auto branching_targets(const std::vector<Instructions> &targets)
    -> std::vector<bool> {
  std::vector<bool> result(targets.size(), false);
  std::vector<std::vector<std::size_t>> edges;
  edges.reserve(targets.size());

  for (const auto &target : targets) {
    std::vector<std::size_t> reached;
    std::vector<std::size_t> reached_branching;
    collect_jump_targets(target, false, reached, reached_branching);
    for (const auto index : reached_branching) {
      assert(index < result.size());
      result[index] = true;
    }

    edges.push_back(std::move(reached));
  }

  bool changed{true};
  while (changed) {
    changed = false;
    for (std::size_t index = 0; index < targets.size(); ++index) {
      if (!result[index]) {
        continue;
      }

      for (const auto reached : edges[index]) {
        assert(reached < result.size());
        if (!result[reached]) {
          result[reached] = true;
          changed = true;
        }
      }
    }
  }

  return result;
}

// The names a presence assertion stands for, or nothing when it is not one the
// loop can take over. Only the strict forms qualify: those reject a non-object
// outright, which is the contract the loop takes on when it absorbs them,
// whereas the others pass vacuously on one and so cannot be told apart from a
// loop that matched nothing
inline auto presence_assertion_names(const Instruction &instruction)
    -> std::vector<ValueString> {
  std::vector<ValueString> result;
  switch (instruction.type) {
    case InstructionIndex::AssertionDefinesStrict:
      result.push_back(std::get<ValueProperty>(instruction.value).first);
      break;
    case InstructionIndex::AssertionDefinesAllStrict:
      for (const auto &property : std::get<ValueStringSet>(instruction.value)) {
        result.push_back(property.first);
      }

      break;
    default:
      break;
  }

  return result;
}

// Fold the presence assertions that sit beside a property loop into the loop
// itself, which then proves presence by counting the required entries its walk
// matched. That trades the lookups and the dispatch the assertions cost on
// every validation for a verdict that only arrives once the walk is over, so
// an object that is missing a required name is decided later than it was. Fast
// validation is where this lives, and a conjunction walks the object anyway
inline auto fold_required_into_property_loop(Instructions &instructions)
    -> bool {
  for (auto &instruction : instructions) {
    if (instruction.type != InstructionIndex::LoopPropertiesMatch) {
      continue;
    }

    const auto &indexes{std::get<ValueNamedIndexes>(instruction.value)};
    std::vector<std::size_t> absorbed;
    std::unordered_set<ValueString> required;

    for (std::size_t index = 0; index < instructions.size(); ++index) {
      const auto &candidate{instructions[index]};
      if (candidate.relative_instance_location !=
          instruction.relative_instance_location) {
        continue;
      }

      const auto names{presence_assertion_names(candidate)};
      if (names.empty()) {
        continue;
      }

      // A loop can only count what it matches on, so an assertion naming
      // anything the loop holds no entry for stays where it is
      const auto covered{
          std::ranges::all_of(names, [&indexes](const auto &name) -> bool {
            return std::ranges::any_of(indexes.first,
                                       [&name](const auto &entry) -> bool {
                                         return std::get<2>(entry) == name;
                                       });
          })};
      if (!covered) {
        continue;
      }

      absorbed.push_back(index);
      required.insert(names.cbegin(), names.cend());
    }

    if (absorbed.empty() || required.empty()) {
      continue;
    }

    ValueNamedIndexesRequired value;
    value.first.second = indexes.second;
    value.first.first.reserve(indexes.first.size());
    value.second = 0;
    for (const auto &entry : indexes.first) {
      const auto entry_required{required.contains(std::get<2>(entry))};
      if (entry_required) {
        value.second += 1;
      }

      value.first.first.emplace_back(std::get<0>(entry), std::get<1>(entry),
                                     std::get<2>(entry), entry_required);
    }

    assert(value.second == required.size());
    instruction.type = InstructionIndex::LoopPropertiesMatchRequired;
    instruction.value = std::move(value);

    Instructions result;
    result.reserve(instructions.size() - absorbed.size());
    for (std::size_t index = 0; index < instructions.size(); ++index) {
      if (std::ranges::find(absorbed, index) != absorbed.cend()) {
        continue;
      }

      result.push_back(std::move(instructions[index]));
    }

    instructions = std::move(result);
    return true;
  }

  return false;
}

// A loop whose only child is a type check folds into a single instruction that
// applies that check to every entry, as the loop type that admits the fusion,
// the child type that triggers it, and what the two become
struct LoopTypeFusion {
  InstructionIndex loop;
  InstructionIndex child;
  InstructionIndex result;
};

inline constexpr std::array<LoopTypeFusion, 6> LOOP_TYPE_FUSIONS{
    {{.loop = InstructionIndex::LoopProperties,
      .child = InstructionIndex::AssertionTypeStrict,
      .result = InstructionIndex::LoopPropertiesTypeStrict},
     {.loop = InstructionIndex::LoopProperties,
      .child = InstructionIndex::AssertionType,
      .result = InstructionIndex::LoopPropertiesType},
     {.loop = InstructionIndex::LoopProperties,
      .child = InstructionIndex::AssertionTypeStrictAny,
      .result = InstructionIndex::LoopPropertiesTypeStrictAny},
     {.loop = InstructionIndex::LoopPropertiesEvaluate,
      .child = InstructionIndex::AssertionTypeStrict,
      .result = InstructionIndex::LoopPropertiesTypeStrictEvaluate},
     {.loop = InstructionIndex::LoopPropertiesEvaluate,
      .child = InstructionIndex::AssertionType,
      .result = InstructionIndex::LoopPropertiesTypeEvaluate},
     {.loop = InstructionIndex::LoopPropertiesEvaluate,
      .child = InstructionIndex::AssertionTypeStrictAny,
      .result = InstructionIndex::LoopPropertiesTypeStrictAnyEvaluate}}};

// The fused instruction stands where the loop stood but says what the child
// said, so it takes the loop's instance location and the child's metadata,
// anchored on the loop's own schema location
inline auto fuse_loop_type(Instruction &instruction, Instruction &child,
                           const InstructionIndex result, Instructions &output,
                           InstructionExtras &extra) -> void {
  const auto new_extra_index{extra.size()};
  auto &instruction_meta{extra[instruction.extra_index]};
  auto &child_meta{extra[child.extra_index]};
  extra.push_back({.relative_schema_location =
                       instruction_meta.relative_schema_location.concat(
                           child_meta.relative_schema_location),
                   .keyword_location = std::move(child_meta.keyword_location),
                   .schema_resource = child_meta.schema_resource,
                   .vocabulary = child_meta.vocabulary,
                   .message = child_meta.message,
                   .message_detail = child_meta.message_detail});
  output.push_back(Instruction{.type = result,
                               .relative_instance_location = std::move(
                                   instruction.relative_instance_location),
                               .value = std::move(child.value),
                               .children = {},
                               .extra_index = new_extra_index});
}

inline auto
transform_instruction(Instruction &instruction, Instructions &output,
                      InstructionExtras &extra,
                      const std::vector<Instructions> &targets,
                      const std::vector<TargetStatistics> &statistics,
                      TargetStatistics &current_stats, const Tweaks &tweaks,
                      const bool positional) -> bool {
  // A positional owner pairs each child with an entry of its own data by
  // index, and inlining a jump expands one child into many, which would
  // re-pair the rest
  const auto jumps{!positional &&
                   (instruction.type == InstructionIndex::ControlJump ||
                    instruction.type == InstructionIndex::ControlResourceJump)};
  if (jumps) {
    // A jump that brings dynamic anchors into scope carries the target beside
    // what it binds, rather than on its own
    const auto binds{instruction.type == InstructionIndex::ControlResourceJump};
    const auto jump_target_index{
        binds ? std::get<ValueIndexPair>(instruction.value).first
              : std::get<ValueUnsignedInteger>(instruction.value)};
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
        (!jump_target_stats.requires_empty_instance_location ||
         instruction.relative_instance_location.empty())) {
      current_stats.jump_targets[jump_target_index]--;
      for (const auto &[inlined_jump, inlined_count] :
           jump_target_stats.jump_targets) {
        current_stats.jump_targets[inlined_jump] += inlined_count;
      }
      current_stats.requires_empty_instance_location |=
          jump_target_stats.requires_empty_instance_location;

      // What the jump bound on entry has to survive it going away, so the
      // instructions that replace it are wrapped in the marker that binds the
      // same names. That is a pass-through, which costs less than the jump did
      Instructions inlined;
      inlined.reserve(targets[jump_target_index].size());
      const auto schema_prefix{
          extra[instruction.extra_index].relative_schema_location};
      for (auto target_instruction : targets[jump_target_index]) {
        duplicate_metadata(target_instruction, extra);
        rebase(target_instruction, extra, schema_prefix,
               instruction.relative_instance_location);
        inlined.push_back(std::move(target_instruction));
      }

      if (binds) {
        current_stats.requires_empty_instance_location = true;
        output.push_back(
            {.type = InstructionIndex::ControlResource,
             .relative_instance_location = {},
             .value =
                 ValueUnsignedInteger{
                     std::get<ValueIndexPair>(instruction.value).second},
             .children = std::move(inlined),
             .extra_index = instruction.extra_index});
      } else {
        for (auto &target_instruction : inlined) {
          output.push_back(std::move(target_instruction));
        }
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

  if (instruction.children.size() == 1) {
    auto &child{instruction.children.front()};
    for (const auto &fusion : LOOP_TYPE_FUSIONS) {
      if (fusion.loop == instruction.type && fusion.child == child.type) {
        fuse_loop_type(instruction, child, fusion.result, output, extra);
        return true;
      }
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

// Reference inlining replaces a branch that was a bare jump with the
// instructions it stood for, so the discriminating property of a branch
// written as a reference only becomes visible once inlining has settled. A
// disjunction that must try every branch reports each of them, which makes the
// order its instructions run in observable, and it carries that requirement as
// its own value
inline auto hoist_discriminators(Instructions &instructions) -> void {
  for (auto &instruction : instructions) {
    if ((instruction.type == InstructionIndex::LogicalXor ||
         instruction.type == InstructionIndex::LogicalXorEvaluate) &&
        !std::get<ValueBoolean>(instruction.value)) {
      for (auto &branch : instruction.children) {
        if (branch.type == InstructionIndex::ControlGroup) {
          hoist_discriminator(branch.children);
        }
      }
    }

    hoist_discriminators(instruction.children);
  }
}

// Whether a fused simple-properties instruction at the same location
// already enforces what this sibling says, so that the sibling can go.
// Only the names it marks as required are presence, and every fused
// instruction rejects a non-object, so it can also stand in for an
// object type assertion
// Whether a fused simple-properties instruction at the same location already
// enforces what this sibling says, so that the sibling can go. Only the names
// it marks as required are presence, and every fused instruction rejects a
// non-object, so it can also stand in for an object type assertion
inline auto subsumed_by_fusion(const Instruction &instruction,
                               const std::unordered_set<std::string> &covered)
    -> bool {
  switch (instruction.type) {
    case InstructionIndex::AssertionDefinesAllStrict:
    case InstructionIndex::AssertionDefinesAll: {
      const auto &value{std::get<ValueStringSet>(instruction.value)};
      for (const auto &property : value) {
        if (!covered.contains(property.first)) {
          return false;
        }
      }

      return true;
    }
    case InstructionIndex::AssertionDefinesStrict:
    case InstructionIndex::AssertionDefines:
      return covered.contains(std::get<ValueProperty>(instruction.value).first);
    case InstructionIndex::AssertionTypeStrict:
    case InstructionIndex::AssertionType:
      return std::get<ValueType>(instruction.value) ==
             sourcemeta::core::JSON::Type::Object;
    default:
      return false;
  }
}

// A conditional stores where its branches begin, so when this pass drops or
// expands any child those indices have to be read off the rewritten list
inline auto remap_boundaries(Instruction &owner,
                             const std::vector<std::size_t> &boundaries,
                             Instructions &result) -> void {
  auto &cursors{std::get<ValueIndexPair>(owner.value)};
  const auto then_start{boundaries[cursors.first]};
  if (then_start == 0) {
    // The condition dropped to nothing, so it is always true: the then
    // branch applies unconditionally and the else branch is dead. Keep
    // only the then instructions and record an empty condition with no
    // else, which the evaluator runs as the consequence
    const auto else_start{cursors.second > 0 ? boundaries[cursors.second]
                                             : result.size()};
    result.erase(result.begin() + static_cast<std::ptrdiff_t>(else_start),
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

// What a group of fused simple-properties instructions requires, by the
// instance location each of them speaks about
using SimplePropertiesFusions = std::vector<
    std::pair<sourcemeta::core::Pointer, std::unordered_set<std::string>>>;

// What the fused simple-properties instructions of one conjunction already
// require, grouped by the instance location they speak about
inline auto simple_properties_fusions(const Instructions &instructions)
    -> SimplePropertiesFusions {
  SimplePropertiesFusions fusions;
  for (const auto &instruction : instructions) {
    if (instruction.type != InstructionIndex::AssertionObjectPropertiesSimple) {
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

  return fusions;
}

// Every children list of a target, each paired with the instruction that owns
// it and with whether it is reached through a disjunction or a conditional.
// Children come before the list that holds them, so a rewrite of one has
// already happened by the time its parent is looked at. Whether a list is
// reached through a branch is carried down the walk, as what holds for a
// branch holds for everything nested inside it
inline auto build_worklist(Instructions &target)
    -> std::vector<std::tuple<Instructions *, Instruction *, bool>> {
  std::vector<std::tuple<Instructions *, Instruction *, bool>> worklist;
  std::vector<std::tuple<Instructions *, std::size_t, Instruction *, bool>>
      stack;
  stack.emplace_back(&target, 0, nullptr, false);

  while (!stack.empty()) {
    auto [current, index, owner, branching] = stack.back();
    stack.pop_back();

    while (index < current->size() && (*current)[index].children.empty()) {
      ++index;
    }

    if (index < current->size()) {
      stack.emplace_back(current, index + 1, owner, branching);
      stack.emplace_back(&(*current)[index].children, 0, &(*current)[index],
                         branching || is_disjunction((*current)[index].type) ||
                             is_conditional((*current)[index].type));
    } else {
      worklist.emplace_back(current, owner, branching);
    }
  }

  return worklist;
}

inline auto postprocess(std::vector<Instructions> &targets,
                        InstructionExtras &extra, const Tweaks &tweaks)
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

  // Taken before any inlining, as a target spliced into a branch is caught by
  // the ancestry of the list it lands in
  const auto reached_from_a_branch{branching_targets(targets)};

  bool changed{true};
  while (changed) {
    changed = false;

    for (std::size_t current_target_index = 0;
         current_target_index < targets.size(); ++current_target_index) {
      auto &target{targets[current_target_index]};
      auto &current_stats{statistics[current_target_index]};
      bool target_changed{false};

      // Whether a list is reached through a disjunction or a conditional is
      // carried down the walk, as what holds for a branch holds for
      // everything nested inside it
      const auto worklist{build_worklist(target)};

      for (const auto &[current, owner, branching] : worklist) {
        Instructions result;
        result.reserve(current->size());

        // A conditional stores the indices where its branches begin, so when
        // this pass drops or expands any child, those indices must be remapped
        // onto the rewritten list
        const bool remap{owner != nullptr && is_conditional(owner->type)};
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
            positional || (owner != nullptr && (is_conditional(owner->type) ||
                                                is_disjunction(owner->type)))};

        if (!disjoint && fuse_numeric_bounds(*current, extra)) {
          target_changed = true;
        }

        // A disjunction branch is unrolled on purpose, so that one comparison
        // can decide it and the rest of the branch never runs. Merging its
        // properties into a loop would bury that comparison inside the walk
        if (!disjoint && !branching &&
            fuse_object_properties(*current, extra)) {
          target_changed = true;
        }

        if (!disjoint && !branching &&
            !reached_from_a_branch[current_target_index] &&
            fold_required_into_property_loop(*current)) {
          target_changed = true;
        }

        // A fused simple-properties instruction only enforces that a property
        // is present for the entries it marks as required, so only those may
        // subsume a sibling presence assertion. Every fused instruction, on
        // the other hand, rejects a non-object outright, so any of them can
        // subsume a sibling object type assertion. None of that says anything
        // about the rest of the instance, so a fused instruction may only
        // subsume siblings that apply to the very same instance location
        const auto fusions{disjoint ? SimplePropertiesFusions{}
                                    : simple_properties_fusions(*current)};

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

          if (fusion != fusions.cend() &&
              subsumed_by_fusion(instruction, fusion->second)) {
            target_changed = true;
            continue;
          }

          if (transform_instruction(instruction, result, extra, targets,
                                    statistics, current_stats, tweaks,
                                    positional)) {
            target_changed = true;
          }

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
          remap_boundaries(*owner, boundaries, result);
        }

        *current = std::move(result);
      }

      if (target_changed) {
        // Later targets may inline this one, so their size and scope checks
        // must describe the rewritten instructions rather than the original
        // jumps. Otherwise a small shared graph can expand into a large tree
        // while every target still appears to fit the inlining threshold.
        current_stats = {.count = 0,
                         .jump_targets = {},
                         .requires_empty_instance_location = false};
        collect_statistics(target, current_stats);
        changed = true;
      }
    }
  }

  for (auto &target : targets) {
    hoist_discriminators(target);
  }
}

} // namespace sourcemeta::blaze

#endif
