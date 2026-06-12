#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <algorithm> // std::ranges::any_of, std::ranges::all_of, std::ranges::none_of, std::find_if
#include <cassert> // assert
#include <set>     // std::set
#include <utility> // std::move, std::to_underlying

#include "compile_helpers.h"
#include "default_compiler_draft3.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft4_validation_required(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &current)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return {};
  }

  return compile_required_assertions(
      context, schema_context, dynamic_context, current,
      json_array_to_string_set(
          schema_context.schema.at(dynamic_context.keyword)));
}

auto compiler_draft4_applicator_allof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return {};
  }

  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Instructions children;

  if (context.mode == Mode::FastValidation &&
      // TODO: Make this work with `$dynamicRef`
      !context.uses_dynamic_scopes) {
    for (std::uint64_t index = 0;
         index < schema_context.schema.at(dynamic_context.keyword).size();
         index++) {
      for (auto &&step : compile(
               context, schema_context, dynamic_context,
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return children;
  } else {
    for (std::uint64_t index = 0;
         index < schema_context.schema.at(dynamic_context.keyword).size();
         index++) {
      for (auto &&step : compile(
               context, schema_context, relative_dynamic_context(),
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_applicator_anyof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return {};
  }

  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Instructions disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make(
        sourcemeta::blaze::InstructionIndex::ControlGroup, context,
        schema_context, relative_dynamic_context(), ValueNone{},
        compile(
            context, schema_context, relative_dynamic_context(),
            {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})));
  }

  if (context.mode == Mode::FastValidation &&
      std::ranges::all_of(disjunctors, [](const auto &instruction) {
        return instruction.children.size() == 1 &&
               (instruction.children.front().type ==
                    sourcemeta::blaze::InstructionIndex::AssertionTypeStrict ||
                instruction.children.front().type ==
                    sourcemeta::blaze::InstructionIndex::
                        AssertionTypeStrictAny);
      })) {
    ValueTypes types{};
    for (const auto &instruction : disjunctors) {
      if (instruction.children.front().type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStrict) {
        const auto &value{
            *std::get_if<ValueType>(&instruction.children.front().value)};
        types.set(static_cast<std::uint8_t>(value));
      }

      if (instruction.children.front().type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny) {
        const auto &value{
            *std::get_if<ValueTypes>(&instruction.children.front().value)};
        types |= value;
      }
    }

    assert(types.any());
    const auto popcount{types.count()};
    if (popcount > 1) {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, types)};
    } else {
      std::uint8_t type_index{0};
      for (std::uint8_t bit{0}; bit < 8; bit++) {
        if (types.test(bit)) {
          type_index = bit;
          break;
        }
      }
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   static_cast<ValueType>(type_index))};
    }
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 requires_evaluation(context, schema_context)};

  return {make(sourcemeta::blaze::InstructionIndex::LogicalOr, context,
               schema_context, dynamic_context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

auto compiler_draft4_applicator_oneof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_array()) {
    return {};
  }

  assert(!schema_context.schema.at(dynamic_context.keyword).empty());

  Instructions disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    disjunctors.push_back(make(
        sourcemeta::blaze::InstructionIndex::ControlGroup, context,
        schema_context, relative_dynamic_context(), ValueNone{},
        compile(
            context, schema_context, relative_dynamic_context(),
            {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})));
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 requires_evaluation(context, schema_context)};

  return {make(sourcemeta::blaze::InstructionIndex::LogicalXor, context,
               schema_context, dynamic_context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

auto compiler_draft4_applicator_not(const Context &context,
                                    const SchemaContext &schema_context,
                                    const DynamicContext &dynamic_context,
                                    const Instructions &) -> Instructions {
  const auto subschemas{defines_nested_subschemas(context, schema_context)};

  Instructions children{compile(context, schema_context,
                                relative_dynamic_context(),
                                sourcemeta::core::empty_weak_pointer,
                                sourcemeta::core::empty_weak_pointer)};

  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track_items{
      std::ranges::any_of(context.unevaluated, [](const auto &dependency) {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  // Only emit a `not` instruction that keeps track of
  // evaluation if we really need it. If the "not" subschema
  // does not define applicators, then that's an easy case
  // we can skip
  if (subschemas &&
      (requires_evaluation(context, schema_context) || track_items)) {
    return {make(sourcemeta::blaze::InstructionIndex::LogicalNotEvaluate,
                 context, schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  } else {
    return {make(sourcemeta::blaze::InstructionIndex::LogicalNot, context,
                 schema_context, dynamic_context, ValueNone{},
                 std::move(children))};
  }
}

auto compiler_draft4_validation_maxproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_integral()) {
    return {};
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "object") {
    return {};
  }

  return {make(
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeLess, context,
      schema_context, dynamic_context,
      ValueUnsignedInteger{
          static_cast<unsigned long>(
              schema_context.schema.at(dynamic_context.keyword).as_integer()) +
          1})};
}

auto compiler_draft4_validation_minproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_integral()) {
    return {};
  }

  assert(schema_context.schema.at(dynamic_context.keyword).is_positive());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // We'll handle it at the type level as an optimization
  if (context.mode == Mode::FastValidation &&
      schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() == "object") {
    return {};
  }

  const auto value{static_cast<unsigned long>(
      schema_context.schema.at(dynamic_context.keyword).as_integer())};
  if (value <= 0) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionObjectSizeGreater,
               context, schema_context, dynamic_context,
               ValueUnsignedInteger{value - 1})};
}

} // namespace internal
#endif
