#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <algorithm> // std::ranges::any_of, std::ranges::all_of, std::ranges::none_of
#include <cassert> // assert
#include <set>     // std::set
#include <utility> // std::move, std::to_underlying

#include "compile_helpers.h"
#include "default_compiler_draft3.h"

namespace sourcemeta::blaze {

auto compiler_draft4_validation_required(const KeywordContext &context,
                                         const Instructions &current)
    -> Instructions {
  // Every dialect that spells `required` as an array of property names asks
  // for unique entries. Draft 4 asks that it name at least one property too,
  // and the OpenAPI 3.0 Schema Object, whose meta-schema is a Draft 4
  // document, asks the same
  const auto allows_empty{!is_draft4(context.vocabularies()) &&
                          !is_openapi_3_0(context.vocabularies())};
  if (!is_property_name_array(context.value()) || !context.value().unique() ||
      (!allows_empty && context.value().empty())) {
    throw_keyword_error(context, EXPECTED_PROPERTY_NAME_ARRAY);
  }

  return compile_required_assertions(context, current,
                                     json_array_to_string_set(context.value()));
}

auto compiler_draft4_applicator_allof(const KeywordContext &context,
                                      const Instructions &) -> Instructions {
  const auto allows_empty{
      in_place_applicators_may_be_empty(context.vocabularies())};
  if (!is_schema_array(context.value(), context.vocabularies(), allows_empty)) {
    throw_keyword_error(context, allows_empty
                                     ? EXPECTED_SCHEMA_ARRAY
                                     : EXPECTED_SCHEMA_ARRAY_NON_EMPTY);
  }

  Instructions children;

  if (context.mode() == Mode::FastValidation &&
      // TODO: Make this work with `$dynamicRef`
      !context.global.uses_dynamic_scopes) {
    for (std::uint64_t index = 0; index < context.value().size(); index++) {
      for (auto &&step : compile(
               context,
               {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
        children.push_back(std::move(step));
      }
    }

    return children;
  }
  for (std::uint64_t index = 0; index < context.value().size(); index++) {
    for (auto &&step : compile(
             context.relative(),
             {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
      children.push_back(std::move(step));
    }
  }

  // Every branch imposed no constraints, so neither does the conjunction
  if (children.empty()) {
    return {};
  }

  return {make(InstructionIndex::LogicalAnd, context, ValueNone{},
               std::move(children))};
}

auto compiler_draft4_applicator_anyof(const KeywordContext &context,
                                      const Instructions &) -> Instructions {
  const auto allows_empty{
      in_place_applicators_may_be_empty(context.vocabularies())};
  if (!is_schema_array(context.value(), context.vocabularies(), allows_empty)) {
    throw_keyword_error(context, allows_empty
                                     ? EXPECTED_SCHEMA_ARRAY
                                     : EXPECTED_SCHEMA_ARRAY_NON_EMPTY);
  }

  // A disjunction over no branches can never be satisfied. Only a dialect that
  // permits the empty array gets here, as the shape check above refuses it
  // everywhere else
  if (context.value().empty()) {
    return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
  }

  Instructions disjunctors;
  for (std::uint64_t index = 0; index < context.value().size(); index++) {
    disjunctors.push_back(make(
        InstructionIndex::ControlGroup, context.relative(), ValueNone{},
        compile(
            context.relative(),
            {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})));
  }

  if (context.mode() == Mode::FastValidation &&
      std::ranges::all_of(disjunctors, [](const auto &instruction) -> auto {
        return instruction.children.size() == 1 &&
               (instruction.children.front().type ==
                    InstructionIndex::AssertionTypeStrict ||
                instruction.children.front().type ==
                    InstructionIndex::AssertionTypeStrictAny);
      })) {
    ValueTypes types{};
    for (const auto &instruction : disjunctors) {
      if (instruction.children.front().type ==
          InstructionIndex::AssertionTypeStrict) {
        const auto &value{
            *std::get_if<ValueType>(&instruction.children.front().value)};
        types.set(static_cast<std::uint8_t>(value));
      }

      if (instruction.children.front().type ==
          InstructionIndex::AssertionTypeStrictAny) {
        const auto &value{
            *std::get_if<ValueTypes>(&instruction.children.front().value)};
        types |= value;
      }
    }

    assert(types.any());
    const auto popcount{types.count()};
    if (popcount > 1) {
      return {make(InstructionIndex::AssertionTypeStrictAny, context, types)};
    }
    std::uint8_t type_index{0};
    for (std::uint8_t bit{0}; bit < 8; bit++) {
      if (types.test(bit)) {
        type_index = bit;
        break;
      }
    }
    return {make(InstructionIndex::AssertionTypeStrict, context,
                 static_cast<ValueType>(type_index))};
  }

  const auto requires_exhaustive{context.mode() == Mode::Exhaustive ||
                                 context.global.collects_annotations ||
                                 requires_evaluation(context)};

  return {make(InstructionIndex::LogicalOr, context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

auto compiler_draft4_applicator_oneof(const KeywordContext &context,
                                      const Instructions &) -> Instructions {
  const auto allows_empty{
      in_place_applicators_may_be_empty(context.vocabularies())};
  if (!is_schema_array(context.value(), context.vocabularies(), allows_empty)) {
    throw_keyword_error(context, allows_empty
                                     ? EXPECTED_SCHEMA_ARRAY
                                     : EXPECTED_SCHEMA_ARRAY_NON_EMPTY);
  }

  // A disjunction over no branches can never be satisfied. Only a dialect that
  // permits the empty array gets here, as the shape check above refuses it
  // everywhere else
  if (context.value().empty()) {
    return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
  }

  Instructions disjunctors;
  for (std::uint64_t index = 0; index < context.value().size(); index++) {
    disjunctors.push_back(make(
        InstructionIndex::ControlGroup, context.relative(), ValueNone{},
        compile(
            context.relative(),
            {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})));
  }

  const auto requires_exhaustive{context.mode() == Mode::Exhaustive ||
                                 context.global.collects_annotations ||
                                 requires_evaluation(context)};

  return {make(InstructionIndex::LogicalXor, context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

auto compiler_draft4_applicator_not(const KeywordContext &context,
                                    const Instructions &) -> Instructions {
  const auto subschemas{defines_nested_subschemas(context)};

  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track_items{std::ranges::any_of(
      context.global.unevaluated, [](const auto &dependency) -> auto {
        return dependency.first.ends_with("unevaluatedItems");
      })};

  // Only emit a `not` instruction that keeps track of
  // evaluation if we really need it. If the "not" subschema
  // does not define applicators, then that's an easy case
  // we can skip
  if (subschemas && (requires_evaluation(context) || track_items)) {
    return {make(InstructionIndex::LogicalNotEvaluate, context, ValueNone{},
                 std::move(children))};
  }
  return {make(InstructionIndex::LogicalNot, context, ValueNone{},
               std::move(children))};
}

auto compiler_draft4_validation_maxproperties(const KeywordContext &context,
                                              const Instructions &)
    -> Instructions {
  return compile_size_upper_bound(
      context, {.type = sourcemeta::core::JSON::Type::Object,
                .name = "object",
                .instruction = InstructionIndex::AssertionObjectSizeLess});
}

auto compiler_draft4_validation_minproperties(const KeywordContext &context,
                                              const Instructions &)
    -> Instructions {
  return compile_size_lower_bound(
      context, {.type = sourcemeta::core::JSON::Type::Object,
                .name = "object",
                .instruction = InstructionIndex::AssertionObjectSizeGreater});
}

} // namespace sourcemeta::blaze
#endif
