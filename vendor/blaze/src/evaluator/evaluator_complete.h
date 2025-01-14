#ifndef SOURCEMETA_BLAZE_EVALUATOR_COMPLETE_H_
#define SOURCEMETA_BLAZE_EVALUATOR_COMPLETE_H_

#define EVALUATE_BEGIN(instruction_type, precondition)                         \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{                                                          \
      resolve_target(property_target,                                          \
                     sourcemeta::jsontoolkit::get(                             \
                         instance, instruction.relative_instance_location))};  \
  if (!(precondition)) {                                                       \
    return true;                                                               \
  }                                                                            \
  const auto track{schema.track || callback};                                  \
  if (track) {                                                                 \
    evaluator.evaluate_path.push_back(instruction.relative_schema_location);   \
    evaluator.instance_location.push_back(                                     \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (schema.dynamic) {                                                        \
    evaluator.resources.push_back(instruction.schema_resource);                \
  }                                                                            \
  if (callback) {                                                              \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             evaluator.instance_location, Evaluator::null);                    \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NON_STRING(instruction_type, precondition)              \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{sourcemeta::jsontoolkit::get(                             \
      instance, instruction.relative_instance_location)};                      \
  if (!(precondition)) {                                                       \
    return true;                                                               \
  }                                                                            \
  const auto track{schema.track || callback};                                  \
  if (track) {                                                                 \
    evaluator.evaluate_path.push_back(instruction.relative_schema_location);   \
    evaluator.instance_location.push_back(                                     \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (schema.dynamic) {                                                        \
    evaluator.resources.push_back(instruction.schema_resource);                \
  }                                                                            \
  if (callback) {                                                              \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             evaluator.instance_location, Evaluator::null);                    \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_IF_STRING(instruction_type)                             \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto *maybe_target{resolve_string_target(                              \
      property_target, instance, instruction.relative_instance_location)};     \
  if (!maybe_target) {                                                         \
    return true;                                                               \
  }                                                                            \
  const auto track{schema.track || callback};                                  \
  if (track) {                                                                 \
    evaluator.evaluate_path.push_back(instruction.relative_schema_location);   \
    evaluator.instance_location.push_back(                                     \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (schema.dynamic) {                                                        \
    evaluator.resources.push_back(instruction.schema_resource);                \
  }                                                                            \
  if (callback) {                                                              \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             evaluator.instance_location, Evaluator::null);                    \
  }                                                                            \
  const auto &target{*maybe_target};                                           \
  bool result{false};

// This is a slightly complicated dance to avoid traversing the relative
// instance location twice.
#define EVALUATE_BEGIN_TRY_TARGET(instruction_type, precondition)              \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{instance};                                                \
  if (!(precondition)) {                                                       \
    return true;                                                               \
  }                                                                            \
  const auto target_check{                                                     \
      try_get(target, instruction.relative_instance_location)};                \
  if (!target_check) {                                                         \
    return true;                                                               \
  }                                                                            \
  const auto track{schema.track || callback};                                  \
  if (track) {                                                                 \
    evaluator.evaluate_path.push_back(instruction.relative_schema_location);   \
    evaluator.instance_location.push_back(                                     \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (schema.dynamic) {                                                        \
    evaluator.resources.push_back(instruction.schema_resource);                \
  }                                                                            \
  assert(!instruction.relative_instance_location.empty());                     \
  if (callback) {                                                              \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             evaluator.instance_location, Evaluator::null);                    \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION(instruction_type)                       \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto track{schema.track || callback};                                  \
  if (track) {                                                                 \
    evaluator.evaluate_path.push_back(instruction.relative_schema_location);   \
    evaluator.instance_location.push_back(                                     \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if (schema.dynamic) {                                                        \
    evaluator.resources.push_back(instruction.schema_resource);                \
  }                                                                            \
  if (callback) {                                                              \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             evaluator.instance_location, Evaluator::null);                    \
  }                                                                            \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH(instruction_type)           \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  if (callback) {                                                              \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             evaluator.instance_location, Evaluator::null);                    \
  }                                                                            \
  bool result{true};

#define EVALUATE_BEGIN_PASS_THROUGH(instruction_type)                          \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  bool result{true};

#define EVALUATE_END(instruction_type)                                         \
  if (callback) {                                                              \
    callback(EvaluationType::Post, result, instruction,                        \
             evaluator.evaluate_path, evaluator.instance_location,             \
             Evaluator::null);                                                 \
  }                                                                            \
  if (track) {                                                                 \
    evaluator.evaluate_path.pop_back(                                          \
        instruction.relative_schema_location.size());                          \
    evaluator.instance_location.pop_back(                                      \
        instruction.relative_instance_location.size());                        \
  }                                                                            \
  if (schema.dynamic) {                                                        \
    evaluator.resources.pop_back();                                            \
  }                                                                            \
  return result;

#define EVALUATE_END_NO_POP(instruction_type)                                  \
  if (callback) {                                                              \
    callback(EvaluationType::Post, result, instruction,                        \
             evaluator.evaluate_path, evaluator.instance_location,             \
             Evaluator::null);                                                 \
  }                                                                            \
  return result;

#define EVALUATE_END_PASS_THROUGH(instruction_type) return result;

#define EVALUATE_ANNOTATION(instruction_type, destination, annotation_value)   \
  if (callback) {                                                              \
    evaluator.evaluate_path.push_back(instruction.relative_schema_location);   \
    evaluator.instance_location.push_back(                                     \
        instruction.relative_instance_location);                               \
    callback(EvaluationType::Pre, true, instruction, evaluator.evaluate_path,  \
             destination, Evaluator::null);                                    \
    callback(EvaluationType::Post, true, instruction, evaluator.evaluate_path, \
             destination, annotation_value);                                   \
    evaluator.evaluate_path.pop_back(                                          \
        instruction.relative_schema_location.size());                          \
    evaluator.instance_location.pop_back(                                      \
        instruction.relative_instance_location.size());                        \
  }                                                                            \
  return true;

#define EVALUATE_RECURSE(child, target)                                        \
  evaluate_instruction(child, schema, callback, target, property_target,       \
                       depth + 1, evaluator)
#define EVALUATE_RECURSE_ON_PROPERTY_NAME(child, target, name)                 \
  evaluate_instruction(child, schema, callback, target, &name, depth + 1,      \
                       evaluator)

#define SOURCEMETA_EVALUATOR_COMPLETE

namespace sourcemeta::blaze::complete {

#include "dispatch.inc.h"

inline auto evaluate(const sourcemeta::jsontoolkit::JSON &instance,
                     sourcemeta::blaze::Evaluator &evaluator,
                     const sourcemeta::blaze::Template &schema,
                     const sourcemeta::blaze::Callback &callback) -> bool {
  bool overall{true};
  for (const auto &instruction : schema.instructions) {
    if (!evaluate_instruction(instruction, schema, callback, instance, nullptr,
                              0, evaluator)) {
      overall = false;
      break;
    }
  }

  // The evaluation path and instance location must be empty by the time
  // we are done, otherwise there was a frame push/pop mismatch
  assert(evaluator.evaluate_path.empty());
  assert(evaluator.instance_location.empty());
  assert(evaluator.resources.empty());
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
