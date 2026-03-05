#ifndef SOURCEMETA_BLAZE_EVALUATOR_COMPLETE_H_
#define SOURCEMETA_BLAZE_EVALUATOR_COMPLETE_H_

#define EVALUATE_BEGIN(instruction_type, precondition)                         \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{resolve_target(                                           \
      context.property_target,                                                 \
      resolve_instance(instance, instruction.relative_instance_location))};    \
  if (!(precondition)) [[unlikely]] {                                          \
    return true;                                                               \
  }                                                                            \
  const auto track{context.schema->track || *context.callback};                \
  if (track) {                                                                 \
    context.evaluator->evaluate_path.push_back(                                \
        instruction.relative_schema_location);                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (context.schema->dynamic) {                                               \
    context.evaluator->resources.push_back(instruction.schema_resource);       \
  }                                                                            \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NON_STRING(instruction_type, precondition)              \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{                                                          \
      resolve_instance(instance, instruction.relative_instance_location)};     \
  if (!(precondition)) [[unlikely]] {                                          \
    return true;                                                               \
  }                                                                            \
  const auto track{context.schema->track || *context.callback};                \
  if (track) {                                                                 \
    context.evaluator->evaluate_path.push_back(                                \
        instruction.relative_schema_location);                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (context.schema->dynamic) {                                               \
    context.evaluator->resources.push_back(instruction.schema_resource);       \
  }                                                                            \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_IF_STRING(instruction_type)                             \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto *maybe_target{                                                    \
      resolve_string_target(context.property_target, instance,                 \
                            instruction.relative_instance_location)};          \
  if (!maybe_target) [[unlikely]] {                                            \
    return true;                                                               \
  }                                                                            \
  const auto track{context.schema->track || *context.callback};                \
  if (track) {                                                                 \
    context.evaluator->evaluate_path.push_back(                                \
        instruction.relative_schema_location);                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (context.schema->dynamic) {                                               \
    context.evaluator->resources.push_back(instruction.schema_resource);       \
  }                                                                            \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  const auto &target{*maybe_target};                                           \
  bool result{false};

// This is a slightly complicated dance to avoid traversing the relative
// instance location twice.
#define EVALUATE_BEGIN_TRY_TARGET(instruction_type)                            \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{instance};                                                \
  if (!target.is_object()) [[unlikely]] {                                      \
    return true;                                                               \
  }                                                                            \
  assert(!instruction.relative_instance_location.empty());                     \
  const auto *target_check{                                                    \
      instruction.relative_instance_location.size() == 1                       \
          ? target.try_at(                                                     \
                instruction.relative_instance_location.at(0).to_property(),    \
                instruction.relative_instance_location.at(0).property_hash())  \
          : try_get(target, instruction.relative_instance_location)};          \
  if (!target_check) [[unlikely]] {                                            \
    return true;                                                               \
  }                                                                            \
  const auto track{context.schema->track || *context.callback};                \
  if (track) {                                                                 \
    context.evaluator->evaluate_path.push_back(                                \
        instruction.relative_schema_location);                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (context.schema->dynamic) {                                               \
    context.evaluator->resources.push_back(instruction.schema_resource);       \
  }                                                                            \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION(instruction_type)                       \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto track{context.schema->track || *context.callback};                \
  if (track) {                                                                 \
    context.evaluator->evaluate_path.push_back(                                \
        instruction.relative_schema_location);                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (context.schema->dynamic) {                                               \
    context.evaluator->resources.push_back(instruction.schema_resource);       \
  }                                                                            \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH(instruction_type)           \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  bool result{true};

#define EVALUATE_BEGIN_PASS_THROUGH(instruction_type)                          \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  bool result{true};

#define EVALUATE_END(instruction_type)                                         \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Post, result, instruction,             \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  if (track) {                                                                 \
    context.evaluator->evaluate_path.pop_back(                                 \
        instruction.relative_schema_location.size());                          \
    context.evaluator->instance_location.pop_back(                             \
        instruction.relative_instance_location.size());                        \
  }                                                                            \
  if (context.schema->dynamic) {                                               \
    context.evaluator->resources.pop_back();                                   \
  }                                                                            \
  return result;

#define EVALUATE_END_NO_POP(instruction_type)                                  \
  if (*context.callback) {                                                     \
    (*context.callback)(EvaluationType::Post, result, instruction,             \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  return result;

#define EVALUATE_END_PASS_THROUGH(instruction_type) return result;

#define EVALUATE_ANNOTATION(instruction_type, destination, annotation_value)   \
  if (*context.callback) {                                                     \
    context.evaluator->evaluate_path.push_back(                                \
        instruction.relative_schema_location);                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.evaluator->evaluate_path, destination,         \
                        Evaluator::null);                                      \
    (*context.callback)(EvaluationType::Post, true, instruction,               \
                        context.evaluator->evaluate_path, destination,         \
                        annotation_value);                                     \
    context.evaluator->evaluate_path.pop_back(                                 \
        instruction.relative_schema_location.size());                          \
    context.evaluator->instance_location.pop_back(                             \
        instruction.relative_instance_location.size());                        \
  }                                                                            \
  return true;

#define EVALUATE_RECURSE(child, target)                                        \
  evaluate_instruction(child, target, depth + 1, context)
#define EVALUATE_RECURSE_ON_PROPERTY_NAME(child, target, name)                 \
  evaluate_instruction_with_property(child, target, depth + 1, context, name)

#define SOURCEMETA_EVALUATOR_COMPLETE

namespace sourcemeta::blaze::complete {

#include "dispatch.inc.h"

inline auto evaluate(const sourcemeta::core::JSON &instance,
                     sourcemeta::blaze::Evaluator &evaluator,
                     const sourcemeta::blaze::Template &schema,
                     const sourcemeta::blaze::Callback &callback) -> bool {
  assert(!schema.targets.empty());
  DispatchContext context{.schema = &schema,
                          .callback = &callback,
                          .evaluator = &evaluator,
                          .property_target = nullptr};
  bool overall{true};
  for (const auto &instruction : schema.targets[0]) {
    if (!evaluate_instruction(instruction, instance, 0, context)) [[unlikely]] {
      overall = false;
      break;
    }
  }

  // The evaluation path and instance location must be empty by the time
  // we are done, otherwise there was a frame push/pop mismatch
  assert(context.evaluator->evaluate_path.empty());
  assert(context.evaluator->instance_location.empty());
  assert(context.evaluator->resources.empty());
  return overall;
}

} // namespace sourcemeta::blaze::complete

#undef SOURCEMETA_EVALUATOR_COMPLETE

#undef EVALUATE_BEGIN
#undef EVALUATE_BEGIN_NON_STRING
#undef EVALUATE_BEGIN_IF_STRING
#undef EVALUATE_BEGIN_TRY_TARGET
#undef EVALUATE_BEGIN_NO_PRECONDITION
#undef EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH
#undef EVALUATE_BEGIN_PASS_THROUGH
#undef EVALUATE_END
#undef EVALUATE_END_NO_POP
#undef EVALUATE_END_PASS_THROUGH
#undef EVALUATE_ANNOTATION
#undef EVALUATE_RECURSE
#undef EVALUATE_RECURSE_ON_PROPERTY_NAME

#endif
