#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT4_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <algorithm> // std::ranges::any_of, std::ranges::all_of, std::ranges::none_of, std::find_if, std::rotate
#include <cassert>   // assert
#include <iterator>  // std::next
#include <set>       // std::set
#include <utility>   // std::move, std::to_underlying

#include "compile_helpers.h"
#include "default_compiler_draft3.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft4_validation_required(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &current)
    -> Instructions {
  // Draft 4 asks that `required` name at least one property, and the OpenAPI
  // 3.0 Schema Object, whose meta-schema is a Draft 4 document, asks the same
  using Known = sourcemeta::core::SchemaVocabularies::Known;
  const auto allows_empty{!schema_context.vocabularies.contains_any(
      {Known::JSON_SCHEMA_DRAFT_4, Known::JSON_SCHEMA_DRAFT_4_HYPER,
       Known::OPENAPI_3_0_BASE})};
  if (!is_string_array(schema_context.schema.at(dynamic_context.keyword)) ||
      (!allows_empty &&
       schema_context.schema.at(dynamic_context.keyword).empty())) {
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        EXPECTED_PROPERTY_NAME_ARRAY);
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
  const auto allows_empty{
      in_place_applicators_may_be_empty(schema_context.vocabularies)};
  if (!is_schema_array(schema_context.schema.at(dynamic_context.keyword),
                       booleans_are_schemas(schema_context.vocabularies),
                       allows_empty)) {
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        allows_empty ? EXPECTED_SCHEMA_ARRAY_ANY_SIZE : EXPECTED_SCHEMA_ARRAY);
  }

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
  }
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    for (auto &&step : compile(
             context, schema_context, relative_dynamic_context(),
             {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})) {
      children.push_back(std::move(step));
    }
  }

  // Every branch imposed no constraints, so neither does the conjunction
  if (children.empty()) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LogicalAnd, context,
               schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

auto compiler_draft4_applicator_anyof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  const auto allows_empty{
      in_place_applicators_may_be_empty(schema_context.vocabularies)};
  if (!is_schema_array(schema_context.schema.at(dynamic_context.keyword),
                       booleans_are_schemas(schema_context.vocabularies),
                       allows_empty)) {
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        allows_empty ? EXPECTED_SCHEMA_ARRAY_ANY_SIZE : EXPECTED_SCHEMA_ARRAY);
  }

  // A disjunction over no branches can never be satisfied. Only a dialect that
  // permits the empty array gets here, as the shape check above refuses it
  // everywhere else
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                 schema_context, dynamic_context, ValueNone{})};
  }

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
      std::ranges::all_of(disjunctors, [](const auto &instruction) -> auto {
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
    }
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

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 context.collects_annotations ||
                                 requires_evaluation(context, schema_context)};

  return {make(sourcemeta::blaze::InstructionIndex::LogicalOr, context,
               schema_context, dynamic_context,
               ValueBoolean{requires_exhaustive}, std::move(disjunctors))};
}

// A disjunction branch is usually ruled out by a single discriminating
// property, as a tagged union does, yet the branch spends its first
// instructions on the wider checks that the tag makes irrelevant. Running the
// tag comparison first lets every branch but the matching one fail on one
// property lookup. The comparison is only sound ahead of the check that proves
// the property is there, so it becomes the variant that holds vacuously when
// the property is missing, leaving that proof to the check it now precedes
auto hoist_discriminator(Instructions &branch) -> void {
  ValueStringSet guaranteed;
  for (const auto &step : branch) {
    if (!step.relative_instance_location.empty()) {
      continue;
    }

    if (step.type == InstructionIndex::AssertionDefinesStrict) {
      const auto &property{std::get<ValueProperty>(step.value)};
      guaranteed.insert(property.first, property.second);
    } else if (step.type == InstructionIndex::AssertionDefinesAllStrict) {
      for (const auto &property : std::get<ValueStringSet>(step.value)) {
        guaranteed.insert(property.first, property.second);
      }
    }
  }

  if (guaranteed.empty()) {
    return;
  }

  const auto match{std::find_if(
      branch.begin(), branch.end(), [&guaranteed](const auto &step) -> bool {
        return step.type == InstructionIndex::AssertionEqual &&
               step.relative_instance_location.size() == 1 &&
               step.relative_instance_location.back().is_property() &&
               guaranteed.contains(
                   step.relative_instance_location.back().to_property());
      })};

  if (match == branch.cend() || match == branch.cbegin()) {
    return;
  }

  match->type = InstructionIndex::AssertionPropertyEqual;
  std::rotate(branch.begin(), match, std::next(match));
}

auto compiler_draft4_applicator_oneof(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  const auto allows_empty{
      in_place_applicators_may_be_empty(schema_context.vocabularies)};
  if (!is_schema_array(schema_context.schema.at(dynamic_context.keyword),
                       booleans_are_schemas(schema_context.vocabularies),
                       allows_empty)) {
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        allows_empty ? EXPECTED_SCHEMA_ARRAY_ANY_SIZE : EXPECTED_SCHEMA_ARRAY);
  }

  // A disjunction over no branches can never be satisfied. Only a dialect that
  // permits the empty array gets here, as the shape check above refuses it
  // everywhere else
  if (schema_context.schema.at(dynamic_context.keyword).empty()) {
    return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                 schema_context, dynamic_context, ValueNone{})};
  }

  Instructions disjunctors;
  for (std::uint64_t index = 0;
       index < schema_context.schema.at(dynamic_context.keyword).size();
       index++) {
    auto branch{
        compile(context, schema_context, relative_dynamic_context(),
                {static_cast<sourcemeta::core::Pointer::Token::Index>(index)})};
    if (context.mode == Mode::FastValidation && !context.collects_annotations &&
        !requires_evaluation(context, schema_context)) {
      hoist_discriminator(branch);
    }

    disjunctors.push_back(
        make(sourcemeta::blaze::InstructionIndex::ControlGroup, context,
             schema_context, relative_dynamic_context(), ValueNone{},
             std::move(branch)));
  }

  const auto requires_exhaustive{context.mode == Mode::Exhaustive ||
                                 context.collects_annotations ||
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
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  // TODO: Be smarter about how we treat `unevaluatedItems` like how we do for
  // `unevaluatedProperties`
  const bool track_items{std::ranges::any_of(
      context.unevaluated, [](const auto &dependency) -> auto {
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
  }
  return {make(sourcemeta::blaze::InstructionIndex::LogicalNot, context,
               schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

auto compiler_draft4_validation_maxproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compile_size_upper_bound(
      context, schema_context, dynamic_context,
      {.type = sourcemeta::core::JSON::Type::Object,
       .name = "object",
       .instruction =
           sourcemeta::blaze::InstructionIndex::AssertionObjectSizeLess});
}

auto compiler_draft4_validation_minproperties(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  return compile_size_lower_bound(
      context, schema_context, dynamic_context,
      {.type = sourcemeta::core::JSON::Type::Object,
       .name = "object",
       .instruction =
           sourcemeta::blaze::InstructionIndex::AssertionObjectSizeGreater});
}

} // namespace internal
#endif
