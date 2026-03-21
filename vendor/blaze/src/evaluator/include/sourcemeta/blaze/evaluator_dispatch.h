#ifndef SOURCEMETA_BLAZE_EVALUATOR_DISPATCH_H_
#define SOURCEMETA_BLAZE_EVALUATOR_DISPATCH_H_

#include <sourcemeta/blaze/evaluator.h>

#define SOURCEMETA_STRINGIFY(x) #x

#define EVALUATE_PUSH()                                                        \
  if constexpr (Track) {                                                       \
    context.evaluator->evaluate_path.push_back(                                \
        context.schema->extra[instruction.extra_index]                         \
            .relative_schema_location);                                        \
  }                                                                            \
  if constexpr (HasCallback) {                                                 \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
  }                                                                            \
  if constexpr (Dynamic) {                                                     \
    context.evaluator->resources.push_back(                                    \
        context.schema->extra[instruction.extra_index].schema_resource);       \
  }                                                                            \
  if constexpr (HasCallback) {                                                 \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.schema->extra[instruction.extra_index],        \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }

#define EVALUATE_POP()                                                         \
  if constexpr (HasCallback) {                                                 \
    (*context.callback)(EvaluationType::Post, result, instruction,             \
                        context.schema->extra[instruction.extra_index],        \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  if constexpr (Track) {                                                       \
    context.evaluator->evaluate_path.pop_back(                                 \
        context.schema->extra[instruction.extra_index]                         \
            .relative_schema_location.size());                                 \
  }                                                                            \
  if constexpr (HasCallback) {                                                 \
    context.evaluator->instance_location.pop_back(                             \
        instruction.relative_instance_location.size());                        \
  }                                                                            \
  if constexpr (Dynamic) {                                                     \
    context.evaluator->resources.pop_back();                                   \
  }

#define EVALUATE_BEGIN(instruction_type, precondition)                         \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{resolve_target(                                           \
      context.property_target,                                                 \
      resolve_instance(instance, instruction.relative_instance_location))};    \
  if (!(precondition)) [[unlikely]] {                                          \
    return true;                                                               \
  }                                                                            \
  [[maybe_unused]] constexpr bool track{Track || HasCallback};                 \
  EVALUATE_PUSH()                                                              \
  bool result{false};

#define EVALUATE_BEGIN_NON_STRING(instruction_type, precondition)              \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto &target{                                                          \
      resolve_instance(instance, instruction.relative_instance_location)};     \
  if (!(precondition)) [[unlikely]] {                                          \
    return true;                                                               \
  }                                                                            \
  [[maybe_unused]] constexpr bool track{Track || HasCallback};                 \
  EVALUATE_PUSH()                                                              \
  bool result{false};

#define EVALUATE_BEGIN_IF_STRING(instruction_type)                             \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  const auto *maybe_target{                                                    \
      resolve_string_target(context.property_target, instance,                 \
                            instruction.relative_instance_location)};          \
  if (!maybe_target) [[unlikely]] {                                            \
    return true;                                                               \
  }                                                                            \
  EVALUATE_PUSH()                                                              \
  const auto &target{*maybe_target};                                           \
  bool result{false};

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
  EVALUATE_PUSH()                                                              \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION(instruction_type)                       \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  [[maybe_unused]] constexpr bool track{Track || HasCallback};                 \
  EVALUATE_PUSH()                                                              \
  bool result{false};

#define EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH(instruction_type)           \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  if constexpr (HasCallback) {                                                 \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.schema->extra[instruction.extra_index],        \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  bool result{true};

#define EVALUATE_BEGIN_PASS_THROUGH(instruction_type)                          \
  assert(instruction.type == InstructionIndex::instruction_type);              \
  bool result{true};

#define EVALUATE_END(instruction_type)                                         \
  EVALUATE_POP()                                                               \
  return result;

#define EVALUATE_END_NO_POP(instruction_type)                                  \
  if constexpr (HasCallback) {                                                 \
    (*context.callback)(EvaluationType::Post, result, instruction,             \
                        context.schema->extra[instruction.extra_index],        \
                        context.evaluator->evaluate_path,                      \
                        context.evaluator->instance_location,                  \
                        Evaluator::null);                                      \
  }                                                                            \
  return result;

#define EVALUATE_END_PASS_THROUGH(instruction_type) return result;

#define EVALUATE_ANNOTATION(instruction_type, destination, annotation_value)   \
  if constexpr (HasCallback) {                                                 \
    context.evaluator->evaluate_path.push_back(                                \
        context.schema->extra[instruction.extra_index]                         \
            .relative_schema_location);                                        \
    context.evaluator->instance_location.push_back(                            \
        instruction.relative_instance_location);                               \
    (*context.callback)(EvaluationType::Pre, true, instruction,                \
                        context.schema->extra[instruction.extra_index],        \
                        context.evaluator->evaluate_path, destination,         \
                        Evaluator::null);                                      \
    (*context.callback)(EvaluationType::Post, true, instruction,               \
                        context.schema->extra[instruction.extra_index],        \
                        context.evaluator->evaluate_path, destination,         \
                        annotation_value);                                     \
    context.evaluator->evaluate_path.pop_back(                                 \
        context.schema->extra[instruction.extra_index]                         \
            .relative_schema_location.size());                                 \
    context.evaluator->instance_location.pop_back(                             \
        instruction.relative_instance_location.size());                        \
  }                                                                            \
  return true;

#define EVALUATE_RECURSE(child, target)                                        \
  evaluate_instruction(child, target, depth + 1, context)
#define EVALUATE_RECURSE_ON_PROPERTY_NAME(child, target, name)                 \
  evaluate_instruction_with_property(child, target, depth + 1, context, name)

namespace sourcemeta::blaze::dispatch {
using namespace sourcemeta::core;

inline auto resolve_target(const JSON::String *property_target,
                           const JSON &instance) noexcept -> const JSON & {
  if (property_target) [[unlikely]] {
    return Evaluator::empty_string;
  }

  // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
  return instance;
}

inline auto resolve_instance(const JSON &instance,
                             const Pointer &relative_instance_location)
    -> const JSON & {
  if (relative_instance_location.empty()) {
    // NOLINTNEXTLINE(bugprone-return-const-ref-from-parameter)
    return instance;
  }

  return get(instance, relative_instance_location);
}

inline auto
resolve_string_target(const JSON::String *property_target, const JSON &instance,
                      const Pointer &relative_instance_location) noexcept
    -> const JSON::String * {
  if (property_target) [[unlikely]] {
    return property_target;
  }

  const auto &target{resolve_instance(instance, relative_instance_location)};
  if (!target.is_string()) [[unlikely]] {
    return nullptr;
  } else {
    return &target.to_string();
  }
}

inline auto effective_type_strict_real(const JSON &instance) noexcept
    -> JSON::Type {
  const auto real_type{instance.type()};
  switch (real_type) {
    case JSON::Type::Decimal:
      return instance.to_decimal().is_integer() ? JSON::Type::Integer
                                                : JSON::Type::Real;
    default:
      return real_type;
  }
}

template <bool Track, bool Dynamic, bool HasCallback> struct DispatchContext {
  const sourcemeta::blaze::Template *schema;
  const sourcemeta::blaze::Callback *callback;
  sourcemeta::blaze::Evaluator *evaluator;
  const sourcemeta::core::JSON::String *property_target;
};

template <bool Track, bool Dynamic, bool HasCallback>
inline auto
evaluate_instruction(const sourcemeta::blaze::Instruction &instruction,
                     const sourcemeta::core::JSON &instance,
                     const std::uint64_t depth,
                     DispatchContext<Track, Dynamic, HasCallback> &context)
    -> bool;

template <bool Track, bool Dynamic, bool HasCallback>
inline auto evaluate_instruction_with_property(
    const sourcemeta::blaze::Instruction &instruction,
    const sourcemeta::core::JSON &instance, const std::uint64_t depth,
    DispatchContext<Track, Dynamic, HasCallback> &context,
    const sourcemeta::core::JSON::String &name) -> bool;

#define INSTRUCTION_HANDLER(name)                                              \
  template <bool Track, bool Dynamic, bool HasCallback>                        \
  static inline auto name(                                                     \
      [[maybe_unused]] const sourcemeta::blaze::Instruction &instruction,      \
      [[maybe_unused]] const sourcemeta::core::JSON &instance,                 \
      [[maybe_unused]] const std::uint64_t depth,                              \
      [[maybe_unused]] DispatchContext<Track, Dynamic, HasCallback> &context)  \
      -> bool

INSTRUCTION_HANDLER(AssertionFail) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionFail);
  EVALUATE_END(AssertionFail);
}

INSTRUCTION_HANDLER(AssertionDefines) {
  EVALUATE_BEGIN_NON_STRING(AssertionDefines, target.is_object());
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  result = target.defines(value.first, value.second);
  EVALUATE_END(AssertionDefines);
}

INSTRUCTION_HANDLER(AssertionDefinesStrict) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesStrict);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  result = target.is_object() && target.defines(value.first, value.second);
  EVALUATE_END(AssertionDefinesStrict);
}

INSTRUCTION_HANDLER(AssertionDefinesAll) {
  EVALUATE_BEGIN_NON_STRING(AssertionDefinesAll, target.is_object());
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);

  // Otherwise there is no way the instance can satisfy it anyway
  if (value.size() <= target.object_size()) [[likely]] {
    result = true;
    const auto &object{target.as_object()};
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesAll);
}

INSTRUCTION_HANDLER(AssertionDefinesAllStrict) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesAllStrict);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);

  // Otherwise there is no way the instance can satisfy it anyway
  if (target.is_object() && value.size() <= target.object_size()) [[likely]] {
    result = true;
    const auto &object{target.as_object()};
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesAllStrict);
}

INSTRUCTION_HANDLER(AssertionDefinesExactly) {
  EVALUATE_BEGIN_NON_STRING(AssertionDefinesExactly, target.is_object());
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);
  const auto &object{target.as_object()};

  if (value.size() == object.size()) [[likely]] {
    result = true;
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesExactly);
}

INSTRUCTION_HANDLER(AssertionDefinesExactlyStrict) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesExactlyStrict);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  if (target.is_object()) [[likely]] {
    const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
    // Otherwise we are we even emitting this instruction?
    assert(value.size() > 1);
    const auto &object{target.as_object()};

    if (value.size() == object.size()) [[likely]] {
      result = true;
      for (const auto &property : value) {
        if (!object.defines(property.first, property.second)) [[unlikely]] {
          result = false;
          break;
        }
      }
    }
  }

  EVALUATE_END(AssertionDefinesExactlyStrict);
}

INSTRUCTION_HANDLER(AssertionDefinesExactlyStrictHash3) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesExactlyStrictHash3);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  if (target.is_object()) [[likely]] {
    // TODO: Take advantage of the table of contents structure to speed up
    // checks
    const auto &value{*std::get_if<ValueStringHashes>(&instruction.value)};
    assert(value.first.size() == 3);
    const auto &object{target.as_object()};

    result = object.size() == 3 && ((value.first.at(0) == object.at(0).hash &&
                                     value.first.at(1) == object.at(1).hash &&
                                     value.first.at(2) == object.at(2).hash) ||
                                    (value.first.at(0) == object.at(0).hash &&
                                     value.first.at(1) == object.at(2).hash &&
                                     value.first.at(2) == object.at(1).hash) ||
                                    (value.first.at(0) == object.at(1).hash &&
                                     value.first.at(1) == object.at(0).hash &&
                                     value.first.at(2) == object.at(2).hash) ||
                                    (value.first.at(0) == object.at(1).hash &&
                                     value.first.at(1) == object.at(2).hash &&
                                     value.first.at(2) == object.at(0).hash) ||
                                    (value.first.at(0) == object.at(2).hash &&
                                     value.first.at(1) == object.at(0).hash &&
                                     value.first.at(2) == object.at(1).hash) ||
                                    (value.first.at(0) == object.at(2).hash &&
                                     value.first.at(1) == object.at(1).hash &&
                                     value.first.at(2) == object.at(0).hash));
  }

  EVALUATE_END(AssertionDefinesExactlyStrictHash3);
}

INSTRUCTION_HANDLER(AssertionPropertyDependencies) {
  EVALUATE_BEGIN_NON_STRING(AssertionPropertyDependencies, target.is_object());
  const auto &value{*std::get_if<ValueStringMap>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(!value.empty());
  result = true;
  const auto &object{target.as_object()};
  const sourcemeta::core::PropertyHashJSON<ValueString> hasher;
  for (const auto &entry : value) {
    if (!object.defines(entry.first, entry.hash)) {
      continue;
    }

    assert(!entry.second.empty());
    for (const auto &dependency : entry.second) {
      if (!object.defines(dependency, hasher(dependency))) [[unlikely]] {
        result = false;
        EVALUATE_END(AssertionPropertyDependencies);
      }
    }
  }

  EVALUATE_END(AssertionPropertyDependencies);
}

INSTRUCTION_HANDLER(AssertionType) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionType);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueType>(&instruction.value)};

  // TODO: Maybe make this instruction about integers, as it is the
  // only where where it is actually useful?
  assert(value == JSON::Type::Integer);

  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  result = target.type() == value ||
           (value == JSON::Type::Integer && target.is_integral());
  EVALUATE_END(AssertionType);
}

INSTRUCTION_HANDLER(AssertionTypeAny) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeAny);
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  const auto type_index{static_cast<std::uint8_t>(target.type())};
  // NOLINTNEXTLINE(bugprone-branch-clone)
  if (value.test(type_index)) {
    result = true;
  } else if (value.test(static_cast<std::uint8_t>(JSON::Type::Integer)) &&
             target.is_integral()) {
    result = true;
  }

  EVALUATE_END(AssertionTypeAny);
}

INSTRUCTION_HANDLER(AssertionTypeStrict) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStrict);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  result = effective_type_strict_real(target) == value;
  EVALUATE_END(AssertionTypeStrict);
}

INSTRUCTION_HANDLER(AssertionTypeStrictAny) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStrictAny);
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto type_index{
      static_cast<std::uint8_t>(effective_type_strict_real(target))};
  result = value.test(type_index);
  EVALUATE_END(AssertionTypeStrictAny);
}

INSTRUCTION_HANDLER(AssertionTypeStringBounded) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStringBounded);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueRange>(&instruction.value)};
  const auto minimum{std::get<0>(value)};
  const auto maximum{std::get<1>(value)};
  assert(!maximum.has_value() || maximum.value() >= minimum);
  // Require early breaking
  assert(!std::get<2>(value));
  result = target.type() == JSON::Type::String &&
           target.string_size() >= minimum &&
           (!maximum.has_value() || target.string_size() <= maximum.value());
  EVALUATE_END(AssertionTypeStringBounded);
}

INSTRUCTION_HANDLER(AssertionTypeStringUpper) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStringUpper);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = target.is_string() && target.string_size() <= value;
  EVALUATE_END(AssertionTypeStringUpper);
}

INSTRUCTION_HANDLER(AssertionTypeArrayBounded) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeArrayBounded);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueRange>(&instruction.value)};
  const auto minimum{std::get<0>(value)};
  const auto maximum{std::get<1>(value)};
  assert(!maximum.has_value() || maximum.value() >= minimum);
  // Require early breaking
  assert(!std::get<2>(value));
  result = target.type() == JSON::Type::Array &&
           target.array_size() >= minimum &&
           (!maximum.has_value() || target.array_size() <= maximum.value());
  EVALUATE_END(AssertionTypeArrayBounded);
}

INSTRUCTION_HANDLER(AssertionTypeArrayUpper) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeArrayUpper);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = target.is_array() && target.array_size() <= value;
  EVALUATE_END(AssertionTypeArrayUpper);
}

INSTRUCTION_HANDLER(AssertionTypeObjectBounded) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeObjectBounded);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueRange>(&instruction.value)};
  const auto minimum{std::get<0>(value)};
  const auto maximum{std::get<1>(value)};
  assert(!maximum.has_value() || maximum.value() >= minimum);
  // Require early breaking
  assert(!std::get<2>(value));
  result = target.type() == JSON::Type::Object &&
           target.object_size() >= minimum &&
           (!maximum.has_value() || target.object_size() <= maximum.value());
  EVALUATE_END(AssertionTypeObjectBounded);
}

INSTRUCTION_HANDLER(AssertionTypeObjectUpper) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeObjectUpper);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = target.is_object() && target.object_size() <= value;
  EVALUATE_END(AssertionTypeObjectUpper);
}

INSTRUCTION_HANDLER(AssertionRegex) {
  EVALUATE_BEGIN_IF_STRING(AssertionRegex);
  const auto &value{*std::get_if<ValueRegex>(&instruction.value)};
  result = matches(value.first, target);
  EVALUATE_END(AssertionRegex);
}

INSTRUCTION_HANDLER(AssertionStringSizeLess) {
  EVALUATE_BEGIN_IF_STRING(AssertionStringSizeLess);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (JSON::size(target) < value);
  EVALUATE_END(AssertionStringSizeLess);
}

INSTRUCTION_HANDLER(AssertionStringSizeGreater) {
  EVALUATE_BEGIN_IF_STRING(AssertionStringSizeGreater);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (JSON::size(target) > value);
  EVALUATE_END(AssertionStringSizeGreater);
}

INSTRUCTION_HANDLER(AssertionArraySizeLess) {
  EVALUATE_BEGIN_NON_STRING(AssertionArraySizeLess, target.is_array());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.array_size() < value);
  EVALUATE_END(AssertionArraySizeLess);
}

INSTRUCTION_HANDLER(AssertionArraySizeGreater) {
  EVALUATE_BEGIN_NON_STRING(AssertionArraySizeGreater, target.is_array());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.array_size() > value);
  EVALUATE_END(AssertionArraySizeGreater);
}

INSTRUCTION_HANDLER(AssertionObjectSizeLess) {
  EVALUATE_BEGIN_NON_STRING(AssertionObjectSizeLess, target.is_object());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.object_size() < value);
  EVALUATE_END(AssertionObjectSizeLess);
}

INSTRUCTION_HANDLER(AssertionObjectSizeGreater) {
  EVALUATE_BEGIN_NON_STRING(AssertionObjectSizeGreater, target.is_object());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.object_size() > value);
  EVALUATE_END(AssertionObjectSizeGreater);
}

INSTRUCTION_HANDLER(AssertionEqual) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionEqual);
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};

  if (context.property_target) [[unlikely]] {
    result = value.is_string() && value.to_string() == *context.property_target;
  } else {
    const auto &target{
        resolve_instance(instance, instruction.relative_instance_location)};
    result = (target == value);
  }

  EVALUATE_END(AssertionEqual);
}

INSTRUCTION_HANDLER(AssertionEqualsAny) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionEqualsAny);
  const auto &value{*std::get_if<ValueSet>(&instruction.value)};

  if (context.property_target) [[unlikely]] {
    // TODO: This involves a string copy
    result = value.contains(JSON{*context.property_target});
  } else {
    const auto &target{
        resolve_instance(instance, instruction.relative_instance_location)};
    result = value.contains(target);
  }

  EVALUATE_END(AssertionEqualsAny);
}

INSTRUCTION_HANDLER(AssertionEqualsAnyStringHash) {
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionEqualsAnyStringHash);
  const auto &value{*std::get_if<ValueStringHashes>(&instruction.value)};

  const sourcemeta::core::JSON::String *target_string = nullptr;
  if (context.property_target) [[unlikely]] {
    target_string = context.property_target;
  } else {
    const auto &target{
        resolve_instance(instance, instruction.relative_instance_location)};
    if (target.is_string()) [[likely]] {
      target_string = &target.to_string();
    } else {
      EVALUATE_END(AssertionEqualsAnyStringHash);
    }
  }

  const auto string_size{target_string->size()};
  // TODO: Put this on the evaluator to re-use it everywhere
  const sourcemeta::core::PropertyHashJSON<ValueString> hasher;
  const auto value_hash{hasher(*target_string)};
  if (string_size < value.second.size()) [[likely]] {
    const auto &hint{value.second[string_size]};
    assert(hint.first <= hint.second);
    if (hint.second != 0) [[likely]] {
      for (std::size_t index = hint.first - 1; index < hint.second; index++) {
        assert(hasher.is_perfect(value.first[index]));
        if (value.first[index] == value_hash) {
          result = true;
          break;
        }
      }
    }
  }

  EVALUATE_END(AssertionEqualsAnyStringHash);
}

INSTRUCTION_HANDLER(AssertionGreaterEqual) {
  EVALUATE_BEGIN_NON_STRING(AssertionGreaterEqual, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target >= value;
  EVALUATE_END(AssertionGreaterEqual);
}

INSTRUCTION_HANDLER(AssertionLessEqual) {
  EVALUATE_BEGIN_NON_STRING(AssertionLessEqual, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target <= value;
  EVALUATE_END(AssertionLessEqual);
}

INSTRUCTION_HANDLER(AssertionGreater) {
  EVALUATE_BEGIN_NON_STRING(AssertionGreater, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target > value;
  EVALUATE_END(AssertionGreater);
}

INSTRUCTION_HANDLER(AssertionLess) {
  EVALUATE_BEGIN_NON_STRING(AssertionLess, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target < value;
  EVALUATE_END(AssertionLess);
}

INSTRUCTION_HANDLER(AssertionUnique) {
  EVALUATE_BEGIN_NON_STRING(AssertionUnique, target.is_array());
  result = target.unique();
  EVALUATE_END(AssertionUnique);
}

INSTRUCTION_HANDLER(AssertionDivisible) {
  EVALUATE_BEGIN_NON_STRING(AssertionDivisible, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  assert(value.is_number());
  result = target.divisible_by(value);
  EVALUATE_END(AssertionDivisible);
}

INSTRUCTION_HANDLER(AssertionStringType) {
  EVALUATE_BEGIN_IF_STRING(AssertionStringType);
  const auto value{*std::get_if<ValueStringType>(&instruction.value)};
  switch (value) {
    case ValueStringType::URI:
      result = URI::is_uri(target);
      break;
    default:
      // We should never get here
      assert(false);
  }

  EVALUATE_END(AssertionStringType);
}

INSTRUCTION_HANDLER(AssertionPropertyType) {
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyType);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  result = target_check->type() == value ||
           (value == JSON::Type::Integer && target_check->is_integral());
  EVALUATE_END(AssertionPropertyType);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeEvaluate) {
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeEvaluate);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  result = target_check->type() == value ||
           (value == JSON::Type::Integer && target_check->is_integral());

  if (result) {
    context.evaluator->evaluate(target_check);
  }

  EVALUATE_END(AssertionPropertyTypeEvaluate);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrict) {
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrict);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  result = target_check->type() == value ||
           (value == JSON::Type::Integer && target_check->is_decimal() &&
            target_check->to_decimal().is_integer());
  EVALUATE_END(AssertionPropertyTypeStrict);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrictEvaluate) {
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrictEvaluate);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  result = target_check->type() == value;

  if (result) {
    context.evaluator->evaluate(target_check);
  }

  EVALUATE_END(AssertionPropertyTypeStrictEvaluate);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrictAny) {
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrictAny);
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());
  // Now here we refer to the actual property
  const auto type_index{static_cast<std::uint8_t>(target_check->type())};
  result =
      value.test(type_index) ||
      (value.test(static_cast<std::uint8_t>(JSON::Type::Integer)) &&
       target_check->is_decimal() && target_check->to_decimal().is_integer());
  EVALUATE_END(AssertionPropertyTypeStrictAny);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrictAnyEvaluate) {
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrictAnyEvaluate);
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());
  // Now here we refer to the actual property
  const auto type_index{static_cast<std::uint8_t>(target_check->type())};
  result = value.test(type_index);

  if (result) {
    context.evaluator->evaluate(target_check);
  }

  EVALUATE_END(AssertionPropertyTypeStrictAnyEvaluate);
}

INSTRUCTION_HANDLER(AssertionArrayPrefix) {
  EVALUATE_BEGIN_NON_STRING(AssertionArrayPrefix, target.is_array());
  // Otherwise there is no point in emitting this instruction
  assert(!instruction.children.empty());
  result = target.empty();
  const auto prefixes{instruction.children.size() - 1};
  const auto array_size{target.array_size()};
  if (!result) [[likely]] {
    const auto pointer{
        array_size == prefixes ? prefixes : std::min(array_size, prefixes) - 1};
    const auto &entry{instruction.children[pointer]};
    result = true;
    assert(entry.type == sourcemeta::blaze::InstructionIndex::ControlGroup);
    for (const auto &child : entry.children) {
      if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionArrayPrefix);
}

INSTRUCTION_HANDLER(AssertionArrayPrefixEvaluate) {
  EVALUATE_BEGIN_NON_STRING(AssertionArrayPrefixEvaluate, target.is_array());
  // Otherwise there is no point in emitting this instruction
  assert(!instruction.children.empty());
  result = target.empty();
  const auto prefixes{instruction.children.size() - 1};
  const auto array_size{target.array_size()};
  if (!result) [[likely]] {
    const auto pointer{
        array_size == prefixes ? prefixes : std::min(array_size, prefixes) - 1};
    const auto &entry{instruction.children[pointer]};
    result = true;
    assert(entry.type == sourcemeta::blaze::InstructionIndex::ControlGroup);
    for (const auto &child : entry.children) {
      if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
        result = false;
        EVALUATE_END(AssertionArrayPrefixEvaluate);
      }
    }

    assert(result);
    if (array_size == prefixes) {
      context.evaluator->evaluate(&target);
    } else {
      for (std::size_t cursor = 0; cursor <= pointer; cursor++) {
        context.evaluator->evaluate(&target.at(cursor));
      }
    }
  }

  EVALUATE_END(AssertionArrayPrefixEvaluate);
}

INSTRUCTION_HANDLER(LogicalOr) {
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalOr);
  result = instruction.children.empty();
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueBoolean>(&instruction.value)};

  // This boolean value controls whether we should be exhaustive
  if (value) {
    for (const auto &child : instruction.children) {
      if (EVALUATE_RECURSE(child, target)) {
        result = true;
      }
    }
  } else {
    for (const auto &child : instruction.children) {
      if (EVALUATE_RECURSE(child, target)) {
        result = true;
        break;
      }
    }
  }

  EVALUATE_END(LogicalOr);
}

INSTRUCTION_HANDLER(LogicalAnd) {
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalAnd);
  result = true;
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalAnd);
}

INSTRUCTION_HANDLER(LogicalWhenType) {
  const auto value{*std::get_if<ValueType>(&instruction.value)};

  // Not having to worry about numbers in this instruction
  // makes things a lot simpler
  assert(value != JSON::Type::Integer);
  assert(value != JSON::Type::Real);
  assert(value != JSON::Type::Decimal);

  EVALUATE_BEGIN(LogicalWhenType, target.type() == value);
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalWhenType);
}

INSTRUCTION_HANDLER(LogicalWhenDefines) {
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  EVALUATE_BEGIN_NON_STRING(LogicalWhenDefines,
                            target.is_object() &&
                                target.defines(value.first, value.second));
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalWhenDefines);
}

INSTRUCTION_HANDLER(LogicalWhenArraySizeGreater) {
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  EVALUATE_BEGIN_NON_STRING(LogicalWhenArraySizeGreater,
                            target.is_array() && target.array_size() > value);
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalWhenArraySizeGreater);
}

INSTRUCTION_HANDLER(LogicalXor) {
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalXor);
  result = true;
  bool has_matched{false};
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueBoolean>(&instruction.value)};
  for (const auto &child : instruction.children) {
    if (EVALUATE_RECURSE(child, target)) {
      if (has_matched) [[unlikely]] {
        result = false;
        // This boolean value controls whether we should be exhaustive
        if (!value) {
          break;
        }
      } else {
        has_matched = true;
      }
    }
  }

  result = result && has_matched;
  EVALUATE_END(LogicalXor);
}

INSTRUCTION_HANDLER(LogicalCondition) {
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalCondition);
  result = true;
  const auto value{*std::get_if<ValueIndexPair>(&instruction.value)};
  const auto children_size{instruction.children.size()};
  assert(children_size >= value.first);
  assert(children_size >= value.second);

  auto condition_end{children_size};
  if (value.first > 0) {
    condition_end = value.first;
  } else if (value.second > 0) {
    condition_end = value.second;
  }

  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  for (std::size_t cursor = 0; cursor < condition_end; cursor++) {
    if (!EVALUATE_RECURSE(instruction.children[cursor], target)) {
      result = false;
      break;
    }
  }

  const auto consequence_start{result ? value.first : value.second};
  const auto consequence_end{(result && value.second > 0) ? value.second
                                                          : children_size};
  result = true;
  if (consequence_start > 0) {
    if constexpr (Track || HasCallback) {
      if (track) {
        context.evaluator->evaluate_path.pop_back(
            context.schema->extra[instruction.extra_index]
                .relative_schema_location.size());
      }
    }

    for (auto cursor = consequence_start; cursor < consequence_end; cursor++) {
      if (!EVALUATE_RECURSE(instruction.children[cursor], target)) {
        result = false;
        break;
      }
    }

    if constexpr (Track || HasCallback) {
      if (track) {
        context.evaluator->evaluate_path.push_back(
            context.schema->extra[instruction.extra_index]
                .relative_schema_location);
      }
    }
  }

  EVALUATE_END(LogicalCondition);
}

INSTRUCTION_HANDLER(ControlGroup) {
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroup);
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, instance)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroup);
}

INSTRUCTION_HANDLER(ControlGroupWhenDefines) {
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroupWhenDefines);
  assert(!instruction.children.empty());
  // Otherwise why are we emitting this property?
  assert(!instruction.relative_instance_location.empty());
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  if (target.is_object() && target.defines(value.first, value.second)) {
    for (const auto &child : instruction.children) {
      // Note that in this control instruction, we purposely
      // don't navigate into the target
      if (!EVALUATE_RECURSE(child, instance)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroupWhenDefines);
}

INSTRUCTION_HANDLER(ControlGroupWhenDefinesDirect) {
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroupWhenDefinesDirect);
  assert(!instruction.children.empty());
  assert(instruction.relative_instance_location.empty());
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};

  if (instance.is_object() && instance.defines(value.first, value.second)) {
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, instance)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroupWhenDefinesDirect);
}

INSTRUCTION_HANDLER(ControlGroupWhenType) {
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroupWhenType);
  assert(!instruction.children.empty());
  assert(instruction.relative_instance_location.empty());
  const auto value{*std::get_if<ValueType>(&instruction.value)};

  // Not having to worry about numbers in this instruction
  // makes things a lot simpler
  assert(value != JSON::Type::Integer);
  assert(value != JSON::Type::Real);
  assert(value != JSON::Type::Decimal);

  if (instance.type() == value) [[likely]] {
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, instance)) [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroupWhenType);
}

INSTRUCTION_HANDLER(ControlEvaluate) {
  EVALUATE_BEGIN_PASS_THROUGH(ControlEvaluate);
  const auto &value{*std::get_if<ValuePointer>(&instruction.value)};
  context.evaluator->evaluate(&get(instance, value));
  EVALUATE_END_PASS_THROUGH(ControlEvaluate);
}

INSTRUCTION_HANDLER(ControlDynamicAnchorJump) {
  EVALUATE_BEGIN_NO_PRECONDITION(ControlDynamicAnchorJump);
  result = false;
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueString>(&instruction.value)};
  for (const auto &resource : context.evaluator->resources) {
    const auto label{Evaluator::hash(resource, value)};
    const auto match{std::ranges::find_if(
        context.schema->labels,
        [&label](const auto &entry) { return entry.first == label; })};
    if (match != context.schema->labels.cend()) [[likely]] {
      result = true;
      assert(match->second < context.schema->targets.size());
      for (const auto &child : context.schema->targets[match->second]) {
        if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
          result = false;
          EVALUATE_END(ControlDynamicAnchorJump);
        }
      }

      break;
    }
  }

  EVALUATE_END(ControlDynamicAnchorJump);
}

INSTRUCTION_HANDLER(ControlJump) {
  EVALUATE_BEGIN_NO_PRECONDITION(ControlJump);
  result = true;
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  assert(context.schema->targets.size() > value);
  const auto &target{resolve_target(
      context.property_target,
      resolve_instance(instance, instruction.relative_instance_location))};
  for (const auto &child : context.schema->targets[value]) {
    if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(ControlJump);
}

INSTRUCTION_HANDLER(AnnotationEmit) {
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  EVALUATE_ANNOTATION(AnnotationEmit, context.evaluator->instance_location,
                      value);
}

INSTRUCTION_HANDLER(AnnotationToParent) {
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  EVALUATE_ANNOTATION(
      AnnotationToParent,
      // TODO: Can we avoid a copy of the instance location here?
      context.evaluator->instance_location.initial(), value);
}

INSTRUCTION_HANDLER(AnnotationBasenameToParent) {
  EVALUATE_ANNOTATION(
      AnnotationBasenameToParent,
      // TODO: Can we avoid a copy of the instance location here?
      context.evaluator->instance_location.initial(),
      context.evaluator->instance_location.back().to_json());
}

INSTRUCTION_HANDLER(Evaluate) {
  EVALUATE_BEGIN_NO_PRECONDITION(Evaluate);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  context.evaluator->evaluate(&target);
  result = true;
  EVALUATE_END(Evaluate);
}

INSTRUCTION_HANDLER(LogicalNot) {
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalNot);

  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) [[likely]] {
      result = true;
      break;
    }
  }

  EVALUATE_END(LogicalNot);
}

INSTRUCTION_HANDLER(LogicalNotEvaluate) {
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalNotEvaluate);

  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) [[likely]] {
      result = true;
      break;
    }
  }

  context.evaluator->unevaluate();

  EVALUATE_END(LogicalNotEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesUnevaluated) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesUnevaluated, target.is_object());
  assert(!instruction.children.empty());
  result = true;

  if (!context.evaluator->is_evaluated(&target)) {
    for (const auto &entry : target.as_object()) {
      if (context.evaluator->is_evaluated(&entry.second)) {
        continue;
      }

      if constexpr (HasCallback) {
        context.evaluator->instance_location.push_back(entry.first);
      }
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
          result = false;
          if constexpr (HasCallback) {
            context.evaluator->instance_location.pop_back();
          }
          EVALUATE_END(LoopPropertiesUnevaluated);
        }
      }

      if constexpr (HasCallback) {
        context.evaluator->instance_location.pop_back();
      }
    }

    context.evaluator->evaluate(&target);
  }

  EVALUATE_END(LoopPropertiesUnevaluated);
}

INSTRUCTION_HANDLER(LoopPropertiesUnevaluatedExcept) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesUnevaluatedExcept,
                            target.is_object());
  assert(!instruction.children.empty());
  const auto &value{*std::get_if<ValuePropertyFilter>(&instruction.value)};
  result = true;
  // Otherwise why emit this instruction?
  assert(!std::get<0>(value).empty() || !std::get<1>(value).empty() ||
         !std::get<2>(value).empty());

  if (!context.evaluator->is_evaluated(&target)) {
    for (const auto &entry : target.as_object()) {
      if (std::get<0>(value).contains(entry.first, entry.hash)) {
        continue;
      }

      if (std::any_of(std::get<1>(value).cbegin(), std::get<1>(value).cend(),
                      [&entry](const auto &prefix) {
                        return entry.first.starts_with(prefix);
                      })) {
        continue;
      }

      if (std::any_of(std::get<2>(value).cbegin(), std::get<2>(value).cend(),
                      [&entry](const auto &pattern) {
                        return matches(pattern.first, entry.first);
                      })) {
        continue;
      }

      if (context.evaluator->is_evaluated(&entry.second)) {
        continue;
      }

      if constexpr (HasCallback) {
        context.evaluator->instance_location.push_back(entry.first);
      }
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
          result = false;
          if constexpr (HasCallback) {
            context.evaluator->instance_location.pop_back();
          }
          EVALUATE_END(LoopPropertiesUnevaluatedExcept);
        }
      }

      if constexpr (HasCallback) {
        context.evaluator->instance_location.pop_back();
      }
    }

    context.evaluator->evaluate(&target);
  }

  EVALUATE_END(LoopPropertiesUnevaluatedExcept);
}

INSTRUCTION_HANDLER(LoopPropertiesMatch) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesMatch, target.is_object());
  const auto &value{*std::get_if<ValueNamedIndexes>(&instruction.value)};
  assert(!value.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
    const auto *index{value.try_at(entry.first, entry.hash)};
    if (!index) {
      continue;
    }

    const auto &subinstruction{instruction.children[*index]};
    assert(subinstruction.type ==
           sourcemeta::blaze::InstructionIndex::ControlGroup);
    for (const auto &child : subinstruction.children) {
      if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
        result = false;
        EVALUATE_END(LoopPropertiesMatch);
      }
    }
  }

  EVALUATE_END(LoopPropertiesMatch);
}

INSTRUCTION_HANDLER(LoopPropertiesMatchClosed) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesMatchClosed, target.is_object());
  const auto &value{*std::get_if<ValueNamedIndexes>(&instruction.value)};
  assert(!value.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
    const auto *index{value.try_at(entry.first, entry.hash)};
    if (!index) [[unlikely]] {
      result = false;
      break;
    }

    const auto &subinstruction{instruction.children[*index]};
    assert(subinstruction.type ==
           sourcemeta::blaze::InstructionIndex::ControlGroup);
    for (const auto &child : subinstruction.children) {
      if (!EVALUATE_RECURSE(child, target)) [[unlikely]] {
        result = false;
        EVALUATE_END(LoopPropertiesMatchClosed);
      }
    }
  }

  EVALUATE_END(LoopPropertiesMatchClosed);
}

INSTRUCTION_HANDLER(LoopProperties) {
  EVALUATE_BEGIN_NON_STRING(LoopProperties, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopProperties);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopProperties);
}

INSTRUCTION_HANDLER(LoopPropertiesEvaluate) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesEvaluate, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopPropertiesEvaluate);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  context.evaluator->evaluate(&target);
  EVALUATE_END(LoopPropertiesEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesRegex) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesRegex, target.is_object());
  assert(!instruction.children.empty());
  const auto &value{*std::get_if<ValueRegex>(&instruction.value)};
  result = true;
  for (const auto &entry : target.as_object()) {
    if (!matches(value.first, entry.first)) {
      continue;
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopPropertiesRegex);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopPropertiesRegex);
}

INSTRUCTION_HANDLER(LoopPropertiesRegexClosed) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesRegexClosed, target.is_object());
  result = true;
  const auto &value{*std::get_if<ValueRegex>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (!matches(value.first, entry.first)) [[unlikely]] {
      result = false;
      break;
    }

    if (instruction.children.empty()) {
      continue;
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopPropertiesRegexClosed);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopPropertiesRegexClosed);
}

INSTRUCTION_HANDLER(LoopPropertiesStartsWith) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesStartsWith, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  const auto &value{*std::get_if<ValueString>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (!entry.first.starts_with(value)) {
      continue;
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopPropertiesStartsWith);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopPropertiesStartsWith);
}

INSTRUCTION_HANDLER(LoopPropertiesExcept) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesExcept, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  const auto &value{*std::get_if<ValuePropertyFilter>(&instruction.value)};
  // Otherwise why emit this instruction?
  assert(!std::get<0>(value).empty() || !std::get<1>(value).empty() ||
         !std::get<2>(value).empty());

  for (const auto &entry : target.as_object()) {
    if (std::get<0>(value).contains(entry.first, entry.hash)) {
      continue;
    }

    if (std::any_of(std::get<1>(value).cbegin(), std::get<1>(value).cend(),
                    [&entry](const auto &prefix) {
                      return entry.first.starts_with(prefix);
                    })) {
      continue;
    }

    if (std::any_of(std::get<2>(value).cbegin(), std::get<2>(value).cend(),
                    [&entry](const auto &pattern) {
                      return matches(pattern.first, entry.first);
                    })) {
      continue;
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopPropertiesExcept);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopPropertiesExcept);
}

INSTRUCTION_HANDLER(LoopPropertiesType) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesType, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (entry.second.type() != value &&
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        (value != JSON::Type::Integer || !entry.second.is_integral()))
        [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopPropertiesType);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeEvaluate) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeEvaluate, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (entry.second.type() != value &&
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        (value != JSON::Type::Integer || !entry.second.is_integral()))
        [[unlikely]] {
      result = false;
      EVALUATE_END(LoopPropertiesTypeEvaluate);
    }
  }

  context.evaluator->evaluate(&target);
  EVALUATE_END(LoopPropertiesTypeEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesExactlyTypeStrict) {
  EVALUATE_BEGIN_NO_PRECONDITION(LoopPropertiesExactlyTypeStrict);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueTypedProperties>(&instruction.value)};
  if (!target.is_object()) [[unlikely]] {
    EVALUATE_END(LoopPropertiesExactlyTypeStrict);
  }

  const auto &object{target.as_object()};
  if (object.size() == value.second.size()) [[likely]] {
    // Otherwise why emit this instruction?
    assert(!value.second.empty());
    result = true;
    for (const auto &entry : object) {
      if (effective_type_strict_real(entry.second) != value.first)
          [[unlikely]] {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(LoopPropertiesExactlyTypeStrict);
}

INSTRUCTION_HANDLER(LoopPropertiesExactlyTypeStrictHash) {
  EVALUATE_BEGIN_NO_PRECONDITION(LoopPropertiesExactlyTypeStrictHash);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueTypedHashes>(&instruction.value)};

  if (!target.is_object()) [[unlikely]] {
    EVALUATE_END(LoopPropertiesExactlyTypeStrictHash);
  }

  const auto &object{target.as_object()};
  const auto size{object.size()};
  if (size == value.second.first.size()) [[likely]] {
    // Otherwise why emit this instruction?
    assert(!value.second.first.empty());

    // The idea is to first assume the object property ordering and the
    // hashes collection aligns. If they don't we do a full comparison
    // from where we left of.

    std::size_t index{0};
    for (const auto &entry : object) {
      if (effective_type_strict_real(entry.second) != value.first) {
        EVALUATE_END(LoopPropertiesExactlyTypeStrictHash);
      }

      if (entry.hash != value.second.first[index]) {
        break;
      }

      index += 1;
    }

    result = true;
    if (index < size) {
      auto iterator = object.cbegin();
      // Continue where we left
      std::advance(iterator, index);
      for (; iterator != object.cend(); ++iterator) {
        // NOLINTNEXTLINE(modernize-use-ranges)
        if (std::none_of(value.second.first.cbegin(), value.second.first.cend(),
                         [&iterator](const auto hash) {
                           return hash == iterator->hash;
                         })) {
          result = false;
          break;
        }
      }
    }
  }

  EVALUATE_END(LoopPropertiesExactlyTypeStrictHash);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrict) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrict, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (effective_type_strict_real(entry.second) != value) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopPropertiesTypeStrict);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrictEvaluate) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrictEvaluate,
                            target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (effective_type_strict_real(entry.second) != value) [[unlikely]] {
      result = false;
      EVALUATE_END(LoopPropertiesTypeStrictEvaluate);
    }
  }

  context.evaluator->evaluate(&target);
  EVALUATE_END(LoopPropertiesTypeStrictEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrictAny) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrictAny, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());
  for (const auto &entry : target.as_object()) {
    const auto type_index{
        static_cast<std::uint8_t>(effective_type_strict_real(entry.second))};
    if (!value.test(type_index)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopPropertiesTypeStrictAny);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrictAnyEvaluate) {
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrictAnyEvaluate,
                            target.is_object());
  result = true;
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());
  for (const auto &entry : target.as_object()) {
    const auto type_index{
        static_cast<std::uint8_t>(effective_type_strict_real(entry.second))};
    if (!value.test(type_index)) [[unlikely]] {
      result = false;
      EVALUATE_END(LoopPropertiesTypeStrictAnyEvaluate);
    }
  }

  context.evaluator->evaluate(&target);
  EVALUATE_END(LoopPropertiesTypeStrictAnyEvaluate);
}

INSTRUCTION_HANDLER(LoopKeys) {
  EVALUATE_BEGIN_NON_STRING(LoopKeys, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(entry.first);
      }
    }

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE_ON_PROPERTY_NAME(child, Evaluator::null,
                                             entry.first)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopKeys);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopKeys);
}

INSTRUCTION_HANDLER(LoopItems) {
  EVALUATE_BEGIN_NON_STRING(LoopItems, target.is_array());
  assert(!instruction.children.empty());
  result = true;

  // To avoid index lookups and unnecessary conditionals
  if constexpr (!Track && !HasCallback) {
    for (const auto &new_instance : target.as_array()) {
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, new_instance)) [[unlikely]] {
          result = false;
          EVALUATE_END(LoopItems);
        }
      }
    }
  } else {
    for (std::size_t index = 0; index < target.array_size(); index++) {
      if constexpr (HasCallback) {
        if (track) {
          context.evaluator->instance_location.push_back(index);
        }
      }

      const auto &new_instance{target.at(index)};
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, new_instance)) [[unlikely]] {
          result = false;

          if constexpr (HasCallback) {
            if (track) {
              context.evaluator->instance_location.pop_back();
            }
          }

          EVALUATE_END(LoopItems);
        }
      }

      if constexpr (HasCallback) {
        if (track) {
          context.evaluator->instance_location.pop_back();
        }
      }
    }
  }

  EVALUATE_END(LoopItems);
}

INSTRUCTION_HANDLER(LoopItemsFrom) {
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  EVALUATE_BEGIN_NON_STRING(LoopItemsFrom,
                            target.is_array() && value < target.array_size());
  assert(!instruction.children.empty());
  result = true;
  for (std::size_t index = value; index < target.array_size(); index++) {
    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(index);
      }
    }

    const auto &new_instance{target.at(index)};
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, new_instance)) [[unlikely]] {
        result = false;

        if constexpr (HasCallback) {
          if (track) {
            context.evaluator->instance_location.pop_back();
          }
        }

        EVALUATE_END(LoopItemsFrom);
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }
  }

  EVALUATE_END(LoopItemsFrom);
}

INSTRUCTION_HANDLER(LoopItemsUnevaluated) {
  EVALUATE_BEGIN_NON_STRING(LoopItemsUnevaluated, target.is_array());
  assert(!instruction.children.empty());
  result = true;

  if (!context.evaluator->is_evaluated(&target)) {
    for (std::size_t index = 0; index < target.array_size(); index++) {
      const auto &new_instance{target.at(index)};
      if (context.evaluator->is_evaluated(&new_instance)) {
        continue;
      }

      if constexpr (HasCallback) {
        context.evaluator->instance_location.push_back(index);
      }
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, new_instance)) [[unlikely]] {
          result = false;
          if constexpr (HasCallback) {
            context.evaluator->instance_location.pop_back();
          }
          EVALUATE_END(LoopItemsUnevaluated);
        }
      }

      if constexpr (HasCallback) {
        context.evaluator->instance_location.pop_back();
      }
    }

    context.evaluator->evaluate(&target);
  }

  EVALUATE_END(LoopItemsUnevaluated);
}

INSTRUCTION_HANDLER(LoopItemsType) {
  EVALUATE_BEGIN_NON_STRING(LoopItemsType, target.is_array());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_array()) {
    if (entry.type() != value &&
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        (value != JSON::Type::Integer || !entry.is_integral())) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopItemsType);
}

INSTRUCTION_HANDLER(LoopItemsTypeStrict) {
  EVALUATE_BEGIN_NON_STRING(LoopItemsTypeStrict, target.is_array());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_array()) {
    if (effective_type_strict_real(entry) != value) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopItemsTypeStrict);
}

INSTRUCTION_HANDLER(LoopItemsTypeStrictAny) {
  EVALUATE_BEGIN_NON_STRING(LoopItemsTypeStrictAny, target.is_array());
  const auto value{*std::get_if<ValueTypes>(&instruction.value)};
  assert(value.any());

  result = true;
  for (const auto &entry : target.as_array()) {
    const auto type_index{
        static_cast<std::uint8_t>(effective_type_strict_real(entry))};
    if (!value.test(type_index)) [[unlikely]] {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopItemsTypeStrictAny);
}

INSTRUCTION_HANDLER(LoopItemsPropertiesExactlyTypeStrictHash) {
  EVALUATE_BEGIN_NO_PRECONDITION(LoopItemsPropertiesExactlyTypeStrictHash);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueTypedHashes>(&instruction.value)};
  // Otherwise why emit this instruction?
  assert(!value.second.first.empty());

  if (!target.is_array()) [[unlikely]] {
    EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
  }

  result = true;

  const auto hashes_size{value.second.first.size()};
  for (const auto &item : target.as_array()) {
    if (!item.is_object()) [[unlikely]] {
      result = false;
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
    }

    const auto &object{item.as_object()};
    const auto size{object.size()};
    if (size != hashes_size) [[unlikely]] {
      result = false;
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
    }

    // Unroll, for performance reasons, for small collections
    if (hashes_size == 3) {
      for (const auto &entry : object) {
        if (effective_type_strict_real(entry.second) != value.first)
            // NOLINTNEXTLINE(bugprone-branch-clone)
            [[unlikely]] {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        } else if (entry.hash != value.second.first[0] &&
                   entry.hash != value.second.first[1] &&
                   entry.hash != value.second.first[2]) {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        }
      }
    } else if (hashes_size == 2) {
      for (const auto &entry : object) {
        if (effective_type_strict_real(entry.second) != value.first)
            // NOLINTNEXTLINE(bugprone-branch-clone)
            [[unlikely]] {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        } else if (entry.hash != value.second.first[0] &&
                   entry.hash != value.second.first[1]) {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        }
      }
    } else if (hashes_size == 1) {
      const auto &entry{*object.cbegin()};
      if (effective_type_strict_real(entry.second) != value.first ||
          entry.hash != value.second.first[0]) [[unlikely]] {
        result = false;
        EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
      }
    } else {
      std::size_t index{0};
      for (const auto &entry : object) {
        if (effective_type_strict_real(entry.second) != value.first)
            // NOLINTNEXTLINE(bugprone-branch-clone)
            [[unlikely]] {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        } else if (entry.hash == value.second.first[index]) {
          index += 1;
          continue;
          // NOLINTNEXTLINE(modernize-use-ranges)
        } else if (std::find(value.second.first.cbegin(),
                             value.second.first.cend(),
                             entry.hash) == value.second.first.cend()) {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        } else {
          index += 1;
        }
      }
    }
  }

  EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
}

INSTRUCTION_HANDLER(LoopItemsPropertiesExactlyTypeStrictHash3) {
  EVALUATE_BEGIN_NO_PRECONDITION(LoopItemsPropertiesExactlyTypeStrictHash3);
  const auto &target{
      resolve_instance(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueTypedHashes>(&instruction.value)};
  assert(value.second.first.size() == 3);
  // Otherwise why emit this instruction?
  assert(!value.second.first.empty());

  if (!target.is_array()) [[unlikely]] {
    EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
  }

  for (const auto &item : target.as_array()) {
    if (!item.is_object()) [[unlikely]] {
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
    }

    const auto &object{item.as_object()};
    if (object.size() != 3) [[unlikely]] {
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
    }

    const auto &value_1{object.at(0)};
    const auto &value_2{object.at(1)};
    const auto &value_3{object.at(2)};

    if (effective_type_strict_real(value_1.second) != value.first ||
        effective_type_strict_real(value_2.second) != value.first ||
        effective_type_strict_real(value_3.second) != value.first)
        [[unlikely]] {
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
    }

    if ((value_1.hash == value.second.first[0] &&
         value_2.hash == value.second.first[1] &&
         value_3.hash == value.second.first[2]) ||
        (value_1.hash == value.second.first[0] &&
         value_2.hash == value.second.first[2] &&
         value_3.hash == value.second.first[1]) ||
        (value_1.hash == value.second.first[1] &&
         value_2.hash == value.second.first[0] &&
         value_3.hash == value.second.first[2]) ||
        (value_1.hash == value.second.first[1] &&
         value_2.hash == value.second.first[2] &&
         value_3.hash == value.second.first[0]) ||
        (value_1.hash == value.second.first[2] &&
         value_2.hash == value.second.first[0] &&
         value_3.hash == value.second.first[1]) ||
        (value_1.hash == value.second.first[2] &&
         value_2.hash == value.second.first[1] &&
         value_3.hash == value.second.first[0])) {
      continue;
    } else {
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
    }
  }

  result = true;
  EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
}

INSTRUCTION_HANDLER(LoopContains) {
  EVALUATE_BEGIN_NON_STRING(LoopContains, target.is_array());
  assert(!instruction.children.empty());
  const auto &value{*std::get_if<ValueRange>(&instruction.value)};
  const auto minimum{std::get<0>(value)};
  const auto &maximum{std::get<1>(value)};
  assert(!maximum.has_value() || maximum.value() >= minimum);
  const auto is_exhaustive{std::get<2>(value)};
  result = minimum == 0 && target.empty();
  auto match_count{std::numeric_limits<decltype(minimum)>::min()};

  for (std::size_t index = 0; index < target.array_size(); index++) {
    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.push_back(index);
      }
    }

    const auto &new_instance{target.at(index)};
    bool subresult{true};
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, new_instance)) {
        subresult = false;
        break;
      }
    }

    if constexpr (HasCallback) {
      if (track) {
        context.evaluator->instance_location.pop_back();
      }
    }

    if (subresult) {
      match_count += 1;

      // Exceeding the upper bound is definitely a failure
      if (maximum.has_value() && match_count > maximum.value()) {
        result = false;

        // Note that here we don't want to consider whether to run
        // exhaustively or not. At this point, its already a failure,
        // and anything that comes after would not run at all anyway
        break;
      }

      if (match_count >= minimum) {
        result = true;

        // Exceeding the lower bound when there is no upper bound
        // is definitely a success
        if (!maximum.has_value() && !is_exhaustive) {
          break;
        }
      }
    }
  }

  EVALUATE_END(LoopContains);
}

#undef INSTRUCTION_HANDLER

template <bool Track, bool Dynamic, bool HasCallback>
using DispatchHandler = bool (*)(
    const sourcemeta::blaze::Instruction &, const sourcemeta::core::JSON &,
    std::uint64_t, DispatchContext<Track, Dynamic, HasCallback> &);

template <bool Track, bool Dynamic, bool HasCallback>
// Must have same order as InstructionIndex
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
static constexpr DispatchHandler<Track, Dynamic, HasCallback> handlers[95] = {
    AssertionFail,
    AssertionDefines,
    AssertionDefinesStrict,
    AssertionDefinesAll,
    AssertionDefinesAllStrict,
    AssertionDefinesExactly,
    AssertionDefinesExactlyStrict,
    AssertionDefinesExactlyStrictHash3,
    AssertionPropertyDependencies,
    AssertionType,
    AssertionTypeAny,
    AssertionTypeStrict,
    AssertionTypeStrictAny,
    AssertionTypeStringBounded,
    AssertionTypeStringUpper,
    AssertionTypeArrayBounded,
    AssertionTypeArrayUpper,
    AssertionTypeObjectBounded,
    AssertionTypeObjectUpper,
    AssertionRegex,
    AssertionStringSizeLess,
    AssertionStringSizeGreater,
    AssertionArraySizeLess,
    AssertionArraySizeGreater,
    AssertionObjectSizeLess,
    AssertionObjectSizeGreater,
    AssertionEqual,
    AssertionEqualsAny,
    AssertionEqualsAnyStringHash,
    AssertionGreaterEqual,
    AssertionLessEqual,
    AssertionGreater,
    AssertionLess,
    AssertionUnique,
    AssertionDivisible,
    AssertionStringType,
    AssertionPropertyType,
    AssertionPropertyTypeEvaluate,
    AssertionPropertyTypeStrict,
    AssertionPropertyTypeStrictEvaluate,
    AssertionPropertyTypeStrictAny,
    AssertionPropertyTypeStrictAnyEvaluate,
    AssertionArrayPrefix,
    AssertionArrayPrefixEvaluate,
    AnnotationEmit,
    AnnotationToParent,
    AnnotationBasenameToParent,
    Evaluate,
    LogicalNot,
    LogicalNotEvaluate,
    LogicalOr,
    LogicalAnd,
    LogicalXor,
    LogicalCondition,
    LogicalWhenType,
    LogicalWhenDefines,
    LogicalWhenArraySizeGreater,
    LoopPropertiesUnevaluated,
    LoopPropertiesUnevaluatedExcept,
    LoopPropertiesMatch,
    LoopPropertiesMatchClosed,
    LoopProperties,
    LoopPropertiesEvaluate,
    LoopPropertiesRegex,
    LoopPropertiesRegexClosed,
    LoopPropertiesStartsWith,
    LoopPropertiesExcept,
    LoopPropertiesType,
    LoopPropertiesTypeEvaluate,
    LoopPropertiesExactlyTypeStrict,
    LoopPropertiesExactlyTypeStrictHash,
    LoopPropertiesTypeStrict,
    LoopPropertiesTypeStrictEvaluate,
    LoopPropertiesTypeStrictAny,
    LoopPropertiesTypeStrictAnyEvaluate,
    LoopKeys,
    LoopItems,
    LoopItemsFrom,
    LoopItemsUnevaluated,
    LoopItemsType,
    LoopItemsTypeStrict,
    LoopItemsTypeStrictAny,
    LoopItemsPropertiesExactlyTypeStrictHash,
    LoopItemsPropertiesExactlyTypeStrictHash3,
    LoopContains,
    ControlGroup,
    ControlGroupWhenDefines,
    ControlGroupWhenDefinesDirect,
    ControlGroupWhenType,
    ControlEvaluate,
    ControlDynamicAnchorJump,
    ControlJump};

template <bool Track, bool Dynamic, bool HasCallback>
inline auto
evaluate_instruction(const sourcemeta::blaze::Instruction &instruction,
                     const sourcemeta::core::JSON &instance,
                     const std::uint64_t depth,
                     DispatchContext<Track, Dynamic, HasCallback> &context)
    -> bool {
  constexpr auto DEPTH_LIMIT{300};
  if (depth > DEPTH_LIMIT) [[unlikely]] {
    throw EvaluationError("The evaluation path depth limit was reached "
                          "likely due to infinite recursion");
  }

  return handlers<Track, Dynamic, HasCallback>[static_cast<
      std::underlying_type_t<InstructionIndex>>(instruction.type)](
      instruction, instance, depth, context);
}

template <bool Track, bool Dynamic, bool HasCallback>
inline auto evaluate_instruction_with_property(
    const sourcemeta::blaze::Instruction &instruction,
    const sourcemeta::core::JSON &instance, const std::uint64_t depth,
    DispatchContext<Track, Dynamic, HasCallback> &context,
    const sourcemeta::core::JSON::String &name) -> bool {
  const auto *previous = context.property_target;
  context.property_target = &name;
  const auto result =
      evaluate_instruction(instruction, instance, depth, context);
  context.property_target = previous;
  return result;
}

} // namespace sourcemeta::blaze::dispatch

#undef EVALUATE_PUSH
#undef EVALUATE_POP
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
#undef SOURCEMETA_STRINGIFY

#endif
