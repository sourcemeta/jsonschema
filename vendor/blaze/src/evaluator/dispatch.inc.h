inline auto
evaluate_instruction(const sourcemeta::blaze::Instruction &instruction,
                     const sourcemeta::blaze::Template &schema,
                     const sourcemeta::blaze::Callback &callback,
                     const sourcemeta::core::JSON &instance,
                     const sourcemeta::core::JSON::String *property_target,
                     const std::uint64_t depth,
                     sourcemeta::blaze::Evaluator &evaluator) -> bool;

#define INSTRUCTION_HANDLER(name)                                              \
  static inline auto name(                                                     \
      const sourcemeta::blaze::Instruction &instruction,                       \
      const sourcemeta::blaze::Template &schema,                               \
      const sourcemeta::blaze::Callback &callback,                             \
      const sourcemeta::core::JSON &instance,                                  \
      const sourcemeta::core::JSON::String *property_target,                   \
      const std::uint64_t depth, sourcemeta::blaze::Evaluator &evaluator)      \
      -> bool

// TODO: Cleanup this file, mainly its MAYBE_UNUSED macros

INSTRUCTION_HANDLER(AssertionFail) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionFail);
  EVALUATE_END(AssertionFail);
}

INSTRUCTION_HANDLER(AssertionDefines) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionDefines, target.is_object());
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  result = target.defines(value.first, value.second);
  EVALUATE_END(AssertionDefines);
}

INSTRUCTION_HANDLER(AssertionDefinesStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesStrict);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  result = target.is_object() && target.defines(value.first, value.second);
  EVALUATE_END(AssertionDefinesStrict);
}

INSTRUCTION_HANDLER(AssertionDefinesAll) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionDefinesAll, target.is_object());
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);

  // Otherwise there is no way the instance can satisfy it anyway
  if (value.size() <= target.object_size()) {
    result = true;
    const auto &object{target.as_object()};
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesAll);
}

INSTRUCTION_HANDLER(AssertionDefinesAllStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesAllStrict);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);

  // Otherwise there is no way the instance can satisfy it anyway
  if (target.is_object() && value.size() <= target.object_size()) {
    result = true;
    const auto &object{target.as_object()};
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesAllStrict);
}

INSTRUCTION_HANDLER(AssertionDefinesExactly) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionDefinesExactly, target.is_object());
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);
  const auto &object{target.as_object()};

  if (value.size() == object.size()) {
    result = true;
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesExactly);
}

INSTRUCTION_HANDLER(AssertionDefinesExactlyStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesExactlyStrict);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);
  assert(target.is_object());
  const auto &object{target.as_object()};

  if (value.size() == object.size()) {
    result = true;
    for (const auto &property : value) {
      if (!object.defines(property.first, property.second)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionDefinesExactlyStrict);
}

INSTRUCTION_HANDLER(AssertionDefinesExactlyStrictHash3) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionDefinesExactlyStrictHash3);
  const auto &target{get(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueStringHashes>(&instruction.value)};
  assert(value.first.size() == 3);
  assert(target.is_object());
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

  EVALUATE_END(AssertionDefinesExactlyStrictHash3);
}

INSTRUCTION_HANDLER(AssertionPropertyDependencies) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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
      if (!object.defines(dependency, hasher(dependency))) {
        result = false;
        EVALUATE_END(AssertionPropertyDependencies);
      }
    }
  }

  EVALUATE_END(AssertionPropertyDependencies);
}

INSTRUCTION_HANDLER(AssertionType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionType);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueType>(&instruction.value)};

  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  result = target.type() == value ||
           (value == JSON::Type::Integer && target.is_integer_real());
  EVALUATE_END(AssertionType);
}

INSTRUCTION_HANDLER(AssertionTypeAny) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeAny);
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);
  const auto &target{get(instance, instruction.relative_instance_location)};
  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  for (const auto type : value) {
    if (type == JSON::Type::Integer && target.is_integer_real()) {
      result = true;
      break;
    } else if (type == target.type()) {
      result = true;
      break;
    }
  }

  EVALUATE_END(AssertionTypeAny);
}

INSTRUCTION_HANDLER(AssertionTypeStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStrict);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  result = target.type() == value;
  EVALUATE_END(AssertionTypeStrict);
}

INSTRUCTION_HANDLER(AssertionTypeStrictAny) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStrictAny);
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);
  const auto &target{get(instance, instruction.relative_instance_location)};
  result =
      (std::find(value.cbegin(), value.cend(), target.type()) != value.cend());
  EVALUATE_END(AssertionTypeStrictAny);
}

INSTRUCTION_HANDLER(AssertionTypeStringBounded) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStringBounded);
  const auto &target{get(instance, instruction.relative_instance_location)};
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeStringUpper);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = target.is_string() && target.string_size() <= value;
  EVALUATE_END(AssertionTypeStringUpper);
}

INSTRUCTION_HANDLER(AssertionTypeArrayBounded) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeArrayBounded);
  const auto &target{get(instance, instruction.relative_instance_location)};
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeArrayUpper);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = target.is_array() && target.array_size() <= value;
  EVALUATE_END(AssertionTypeArrayUpper);
}

INSTRUCTION_HANDLER(AssertionTypeObjectBounded) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeObjectBounded);
  const auto &target{get(instance, instruction.relative_instance_location)};
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionTypeObjectUpper);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = target.is_object() && target.object_size() <= value;
  EVALUATE_END(AssertionTypeObjectUpper);
}

INSTRUCTION_HANDLER(AssertionRegex) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_IF_STRING(AssertionRegex);
  const auto &value{*std::get_if<ValueRegex>(&instruction.value)};
  result = matches(value.first, target);
  EVALUATE_END(AssertionRegex);
}

INSTRUCTION_HANDLER(AssertionStringSizeLess) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_IF_STRING(AssertionStringSizeLess);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (JSON::size(target) < value);
  EVALUATE_END(AssertionStringSizeLess);
}

INSTRUCTION_HANDLER(AssertionStringSizeGreater) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_IF_STRING(AssertionStringSizeGreater);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (JSON::size(target) > value);
  EVALUATE_END(AssertionStringSizeGreater);
}

INSTRUCTION_HANDLER(AssertionArraySizeLess) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionArraySizeLess, target.is_array());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.array_size() < value);
  EVALUATE_END(AssertionArraySizeLess);
}

INSTRUCTION_HANDLER(AssertionArraySizeGreater) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionArraySizeGreater, target.is_array());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.array_size() > value);
  EVALUATE_END(AssertionArraySizeGreater);
}

INSTRUCTION_HANDLER(AssertionObjectSizeLess) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionObjectSizeLess, target.is_object());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.object_size() < value);
  EVALUATE_END(AssertionObjectSizeLess);
}

INSTRUCTION_HANDLER(AssertionObjectSizeGreater) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionObjectSizeGreater, target.is_object());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  result = (target.object_size() > value);
  EVALUATE_END(AssertionObjectSizeGreater);
}

INSTRUCTION_HANDLER(AssertionEqual) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionEqual);
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};

  if (property_target) [[unlikely]] {
    result = value.is_string() && value.to_string() == *property_target;
  } else {
    const auto &target{get(instance, instruction.relative_instance_location)};
    result = (target == value);
  }

  EVALUATE_END(AssertionEqual);
}

INSTRUCTION_HANDLER(AssertionEqualsAny) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionEqualsAny);
  const auto &value{*std::get_if<ValueSet>(&instruction.value)};

  if (property_target) [[unlikely]] {
    // TODO: This involves a string copy
    result = value.contains(JSON{*property_target});
  } else {
    const auto &target{get(instance, instruction.relative_instance_location)};
    result = value.contains(target);
  }

  EVALUATE_END(AssertionEqualsAny);
}

INSTRUCTION_HANDLER(AssertionEqualsAnyStringHash) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(AssertionEqualsAnyStringHash);
  const auto &value{*std::get_if<ValueStringHashes>(&instruction.value)};

  const sourcemeta::core::JSON::String *target_string = nullptr;
  if (property_target) [[unlikely]] {
    target_string = property_target;
  } else {
    const auto &target{get(instance, instruction.relative_instance_location)};
    if (target.is_string()) {
      target_string = &target.to_string();
    } else {
      EVALUATE_END(AssertionEqualsAnyStringHash);
    }
  }

  const auto string_size{target_string->size()};
  // TODO: Put this on the evaluator to re-use it everywhere
  const sourcemeta::core::PropertyHashJSON<ValueString> hasher;
  const auto value_hash{hasher(*target_string)};
  if (string_size < value.second.size()) {
    const auto &hint{value.second[string_size]};
    assert(hint.first <= hint.second);
    if (hint.second != 0) {
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionGreaterEqual, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target >= value;
  EVALUATE_END(AssertionGreaterEqual);
}

INSTRUCTION_HANDLER(AssertionLessEqual) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionLessEqual, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target <= value;
  EVALUATE_END(AssertionLessEqual);
}

INSTRUCTION_HANDLER(AssertionGreater) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionGreater, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target > value;
  EVALUATE_END(AssertionGreater);
}

INSTRUCTION_HANDLER(AssertionLess) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionLess, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  result = target < value;
  EVALUATE_END(AssertionLess);
}

INSTRUCTION_HANDLER(AssertionUnique) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionUnique, target.is_array());
  result = target.unique();
  EVALUATE_END(AssertionUnique);
}

INSTRUCTION_HANDLER(AssertionDivisible) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(AssertionDivisible, target.is_number());
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
  assert(value.is_number());
  result = target.divisible_by(value);
  EVALUATE_END(AssertionDivisible);
}

INSTRUCTION_HANDLER(AssertionStringType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_IF_STRING(AssertionStringType);
  const auto value{*std::get_if<ValueStringType>(&instruction.value)};
  switch (value) {
    case ValueStringType::URI:
      try {
        // TODO: This implies a string copy
        result = URI{target}.is_absolute();
      } catch (const URIParseError &) {
        result = false;
      }

      break;
    default:
      // We should never get here
      assert(false);
  }

  EVALUATE_END(AssertionStringType);
}

INSTRUCTION_HANDLER(AssertionPropertyType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyType);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  result = target_check->type() == value ||
           (value == JSON::Type::Integer && target_check->is_integer_real());
  EVALUATE_END(AssertionPropertyType);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeEvaluate);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  // In non-strict mode, we consider a real number that represents an
  // integer to be an integer
  result = target_check->type() == value ||
           (value == JSON::Type::Integer && target_check->is_integer_real());

  if (result) {
    evaluator.evaluate(target_check);
  }

  EVALUATE_END(AssertionPropertyTypeEvaluate);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrict);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  result = target_check->type() == value;
  EVALUATE_END(AssertionPropertyTypeStrict);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrictEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrictEvaluate);
  // Now here we refer to the actual property
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  result = target_check->type() == value;

  if (result) {
    evaluator.evaluate(target_check);
  }

  EVALUATE_END(AssertionPropertyTypeStrictEvaluate);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrictAny) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrictAny);
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  // Now here we refer to the actual property
  result = (std::find(value.cbegin(), value.cend(), target_check->type()) !=
            value.cend());
  EVALUATE_END(AssertionPropertyTypeStrictAny);
}

INSTRUCTION_HANDLER(AssertionPropertyTypeStrictAnyEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_TRY_TARGET(AssertionPropertyTypeStrictAnyEvaluate);
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  // Now here we refer to the actual property
  result = (std::find(value.cbegin(), value.cend(), target_check->type()) !=
            value.cend());

  if (result) {
    evaluator.evaluate(target_check);
  }

  EVALUATE_END(AssertionPropertyTypeStrictAnyEvaluate);
}

INSTRUCTION_HANDLER(AssertionArrayPrefix) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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
      if (!EVALUATE_RECURSE(child, target)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(AssertionArrayPrefix);
}

INSTRUCTION_HANDLER(AssertionArrayPrefixEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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
      if (!EVALUATE_RECURSE(child, target)) {
        result = false;
        EVALUATE_END(AssertionArrayPrefixEvaluate);
      }
    }

    assert(result);
    if (array_size == prefixes) {
      evaluator.evaluate(&target);
    } else {
      for (std::size_t cursor = 0; cursor <= pointer; cursor++) {
        evaluator.evaluate(&target.at(cursor));
      }
    }
  }

  EVALUATE_END(AssertionArrayPrefixEvaluate);
}

INSTRUCTION_HANDLER(LogicalOr) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalOr);
  result = instruction.children.empty();
  const auto &target{get(instance, instruction.relative_instance_location)};
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalAnd);
  result = true;
  const auto &target{get(instance, instruction.relative_instance_location)};
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalAnd);
}

INSTRUCTION_HANDLER(LogicalWhenType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  EVALUATE_BEGIN(LogicalWhenType, target.type() == value);
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalWhenType);
}

INSTRUCTION_HANDLER(LogicalWhenDefines) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  EVALUATE_BEGIN_NON_STRING(LogicalWhenDefines,
                            target.is_object() &&
                                target.defines(value.first, value.second));
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalWhenDefines);
}

INSTRUCTION_HANDLER(LogicalWhenArraySizeGreater) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  EVALUATE_BEGIN_NON_STRING(LogicalWhenArraySizeGreater,
                            target.is_array() && target.array_size() > value);
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LogicalWhenArraySizeGreater);
}

INSTRUCTION_HANDLER(LogicalXor) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalXor);
  result = true;
  bool has_matched{false};
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto value{*std::get_if<ValueBoolean>(&instruction.value)};
  for (const auto &child : instruction.children) {
    if (EVALUATE_RECURSE(child, target)) {
      if (has_matched) {
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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

  const auto &target{get(instance, instruction.relative_instance_location)};
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
#if defined(SOURCEMETA_EVALUATOR_COMPLETE) ||                                  \
    defined(SOURCEMETA_EVALUATOR_TRACK)
    if (track) {
      evaluator.evaluate_path.pop_back(
          instruction.relative_schema_location.size());
    }
#endif

    for (auto cursor = consequence_start; cursor < consequence_end; cursor++) {
      if (!EVALUATE_RECURSE(instruction.children[cursor], target)) {
        result = false;
        break;
      }
    }

#if defined(SOURCEMETA_EVALUATOR_COMPLETE) ||                                  \
    defined(SOURCEMETA_EVALUATOR_TRACK)
    if (track) {
      evaluator.evaluate_path.push_back(instruction.relative_schema_location);
    }
#endif
  }

  EVALUATE_END(LogicalCondition);
}

INSTRUCTION_HANDLER(ControlGroup) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroup);
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, instance)) {
      result = false;
      break;
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroup);
}

INSTRUCTION_HANDLER(ControlGroupWhenDefines) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroupWhenDefines);
  assert(!instruction.children.empty());
  // Otherwise why are we emitting this property?
  assert(!instruction.relative_instance_location.empty());
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};
  if (target.is_object() && target.defines(value.first, value.second)) {
    for (const auto &child : instruction.children) {
      // Note that in this control instruction, we purposely
      // don't navigate into the target
      if (!EVALUATE_RECURSE(child, instance)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroupWhenDefines);
}

INSTRUCTION_HANDLER(ControlGroupWhenDefinesDirect) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroupWhenDefinesDirect);
  assert(!instruction.children.empty());
  assert(instruction.relative_instance_location.empty());
  const auto &value{*std::get_if<ValueProperty>(&instruction.value)};

  if (instance.is_object() && instance.defines(value.first, value.second)) {
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, instance)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroupWhenDefinesDirect);
}

INSTRUCTION_HANDLER(ControlGroupWhenType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_PASS_THROUGH(ControlGroupWhenType);
  assert(!instruction.children.empty());
  assert(instruction.relative_instance_location.empty());
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  if (instance.type() == value) {
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, instance)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END_PASS_THROUGH(ControlGroupWhenType);
}

INSTRUCTION_HANDLER(ControlLabel) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(ControlLabel);
  assert(!instruction.children.empty());
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  evaluator.labels.try_emplace(value, instruction.children);
  const auto &target{get(instance, instruction.relative_instance_location)};
  result = true;
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = false;
      break;
    }
  }

  EVALUATE_END(ControlLabel);
}

INSTRUCTION_HANDLER(ControlMark) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION_AND_NO_PUSH(ControlMark);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  evaluator.labels.try_emplace(value, instruction.children);
  EVALUATE_END_NO_POP(ControlMark);
}

INSTRUCTION_HANDLER(ControlEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  EVALUATE_BEGIN_PASS_THROUGH(ControlEvaluate);
  const auto &value{*std::get_if<ValuePointer>(&instruction.value)};
  evaluator.evaluate(&get(instance, value));
  EVALUATE_END_PASS_THROUGH(ControlEvaluate);
}

INSTRUCTION_HANDLER(ControlJump) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(ControlJump);
  result = true;
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  assert(evaluator.labels.contains(value));
  const auto &target{get(instance, instruction.relative_instance_location)};
  for (const auto &child : evaluator.labels.at(value).get()) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = false;
      break;
    }
  }

  EVALUATE_END(ControlJump);
}

INSTRUCTION_HANDLER(ControlDynamicAnchorJump) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(ControlDynamicAnchorJump);
  result = false;
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueString>(&instruction.value)};
  for (const auto &resource : evaluator.resources) {
    const auto label{evaluator.hash(resource, value)};
    const auto match{evaluator.labels.find(label)};
    if (match != evaluator.labels.cend()) {
      result = true;
      for (const auto &child : match->second.get()) {
        if (!EVALUATE_RECURSE(child, target)) {
          result = false;
          EVALUATE_END(ControlDynamicAnchorJump);
        }
      }

      break;
    }
  }

  EVALUATE_END(ControlDynamicAnchorJump);
}

INSTRUCTION_HANDLER(AnnotationEmit) {
  SOURCEMETA_MAYBE_UNUSED(instruction);
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
#endif
  EVALUATE_ANNOTATION(AnnotationEmit, evaluator.instance_location, value);
}

INSTRUCTION_HANDLER(AnnotationToParent) {
  SOURCEMETA_MAYBE_UNUSED(instruction);
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
  const auto &value{*std::get_if<ValueJSON>(&instruction.value)};
#endif
  EVALUATE_ANNOTATION(
      AnnotationToParent,
      // TODO: Can we avoid a copy of the instance location here?
      evaluator.instance_location.initial(), value);
}

INSTRUCTION_HANDLER(AnnotationBasenameToParent) {
  SOURCEMETA_MAYBE_UNUSED(instruction);
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_ANNOTATION(
      AnnotationBasenameToParent,
      // TODO: Can we avoid a copy of the instance location here?
      evaluator.instance_location.initial(),
      evaluator.instance_location.back().to_json());
}

INSTRUCTION_HANDLER(Evaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  EVALUATE_BEGIN_NO_PRECONDITION(Evaluate);
  const auto &target{get(instance, instruction.relative_instance_location)};
  evaluator.evaluate(&target);
  result = true;
  EVALUATE_END(Evaluate);
}

INSTRUCTION_HANDLER(LogicalNot) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalNot);

  const auto &target{get(instance, instruction.relative_instance_location)};
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = true;
      break;
    }
  }

  EVALUATE_END(LogicalNot);
}

INSTRUCTION_HANDLER(LogicalNotEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LogicalNotEvaluate);

  const auto &target{get(instance, instruction.relative_instance_location)};
  for (const auto &child : instruction.children) {
    if (!EVALUATE_RECURSE(child, target)) {
      result = true;
      break;
    }
  }

  evaluator.unevaluate();

  EVALUATE_END(LogicalNotEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesUnevaluated) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesUnevaluated, target.is_object());
  assert(!instruction.children.empty());
  result = true;

  if (!evaluator.is_evaluated(&target)) {
    for (const auto &entry : target.as_object()) {
      if (evaluator.is_evaluated(&entry.second)) {
        continue;
      }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
      evaluator.instance_location.push_back(entry.first);
#endif
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, entry.second)) {
          result = false;
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
          evaluator.instance_location.pop_back();
#endif
          EVALUATE_END(LoopPropertiesUnevaluated);
        }
      }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
      evaluator.instance_location.pop_back();
#endif
    }

    evaluator.evaluate(&target);
  }

  EVALUATE_END(LoopPropertiesUnevaluated);
}

INSTRUCTION_HANDLER(LoopPropertiesUnevaluatedExcept) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesUnevaluatedExcept,
                            target.is_object());
  assert(!instruction.children.empty());
  const auto &value{*std::get_if<ValuePropertyFilter>(&instruction.value)};
  result = true;
  // Otherwise why emit this instruction?
  assert(!std::get<0>(value).empty() || !std::get<1>(value).empty() ||
         !std::get<2>(value).empty());

  if (!evaluator.is_evaluated(&target)) {
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

      if (evaluator.is_evaluated(&entry.second)) {
        continue;
      }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
      evaluator.instance_location.push_back(entry.first);
#endif
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, entry.second)) {
          result = false;
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
          evaluator.instance_location.pop_back();
#endif
          EVALUATE_END(LoopPropertiesUnevaluatedExcept);
        }
      }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
      evaluator.instance_location.pop_back();
#endif
    }

    evaluator.evaluate(&target);
  }

  EVALUATE_END(LoopPropertiesUnevaluatedExcept);
}

INSTRUCTION_HANDLER(LoopPropertiesMatch) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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
      if (!EVALUATE_RECURSE(child, target)) {
        result = false;
        EVALUATE_END(LoopPropertiesMatch);
      }
    }
  }

  EVALUATE_END(LoopPropertiesMatch);
}

INSTRUCTION_HANDLER(LoopPropertiesMatchClosed) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesMatchClosed, target.is_object());
  const auto &value{*std::get_if<ValueNamedIndexes>(&instruction.value)};
  assert(!value.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
    const auto *index{value.try_at(entry.first, entry.hash)};
    if (!index) {
      result = false;
      break;
    }

    const auto &subinstruction{instruction.children[*index]};
    assert(subinstruction.type ==
           sourcemeta::blaze::InstructionIndex::ControlGroup);
    for (const auto &child : subinstruction.children) {
      if (!EVALUATE_RECURSE(child, target)) {
        result = false;
        EVALUATE_END(LoopPropertiesMatchClosed);
      }
    }
  }

  EVALUATE_END(LoopPropertiesMatchClosed);
}

INSTRUCTION_HANDLER(LoopProperties) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopProperties, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopProperties);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopProperties);
}

INSTRUCTION_HANDLER(LoopPropertiesEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesEvaluate, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopPropertiesEvaluate);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  evaluator.evaluate(&target);
  EVALUATE_END(LoopPropertiesEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesRegex) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesRegex, target.is_object());
  assert(!instruction.children.empty());
  const auto &value{*std::get_if<ValueRegex>(&instruction.value)};
  result = true;
  for (const auto &entry : target.as_object()) {
    if (!matches(value.first, entry.first)) {
      continue;
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopPropertiesRegex);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopPropertiesRegex);
}

INSTRUCTION_HANDLER(LoopPropertiesRegexClosed) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesRegexClosed, target.is_object());
  result = true;
  const auto &value{*std::get_if<ValueRegex>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (!matches(value.first, entry.first)) {
      result = false;
      break;
    }

    if (instruction.children.empty()) {
      continue;
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopPropertiesRegexClosed);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopPropertiesRegexClosed);
}

INSTRUCTION_HANDLER(LoopPropertiesStartsWith) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesStartsWith, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  const auto &value{*std::get_if<ValueString>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (!entry.first.starts_with(value)) {
      continue;
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopPropertiesStartsWith);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopPropertiesStartsWith);
}

INSTRUCTION_HANDLER(LoopPropertiesExcept) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, entry.second)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopPropertiesExcept);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopPropertiesExcept);
}

INSTRUCTION_HANDLER(LoopPropertiesWhitelist) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesWhitelist, target.is_object());
  const auto &value{*std::get_if<ValueStringSet>(&instruction.value)};
  // Otherwise why emit this instruction?
  assert(!value.empty());

  // Otherwise if the number of properties in the instance
  // is larger than the whitelist, then it already violated
  // the whitelist?
  if (target.object_size() <= value.size()) {
    result = true;
    for (const auto &entry : target.as_object()) {
      if (!value.contains(entry.first, entry.hash)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(LoopPropertiesWhitelist);
}

INSTRUCTION_HANDLER(LoopPropertiesType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesType, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (entry.second.type() != value &&
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        (value != JSON::Type::Integer || !entry.second.is_integer_real())) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopPropertiesType);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeEvaluate, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (entry.second.type() != value &&
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        (value != JSON::Type::Integer || !entry.second.is_integer_real())) {
      result = false;
      EVALUATE_END(LoopPropertiesTypeEvaluate);
    }
  }

  evaluator.evaluate(&target);
  EVALUATE_END(LoopPropertiesTypeEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesExactlyTypeStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LoopPropertiesExactlyTypeStrict);
  const auto &target{get(instance, instruction.relative_instance_location)};
  const auto &value{*std::get_if<ValueTypedProperties>(&instruction.value)};
  if (!target.is_object()) {
    EVALUATE_END(LoopPropertiesExactlyTypeStrict);
  }

  const auto &object{target.as_object()};
  if (object.size() == value.second.size()) {
    // Otherwise why emit this instruction?
    assert(!value.second.empty());
    result = true;
    for (const auto &entry : object) {
      if (entry.second.type() != value.first ||
          !value.second.contains(entry.first, entry.hash)) {
        result = false;
        break;
      }
    }
  }

  EVALUATE_END(LoopPropertiesExactlyTypeStrict);
}

INSTRUCTION_HANDLER(LoopPropertiesExactlyTypeStrictHash) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LoopPropertiesExactlyTypeStrictHash);
  const auto &target{get(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueTypedHashes>(&instruction.value)};

  if (!target.is_object()) {
    EVALUATE_END(LoopPropertiesExactlyTypeStrictHash);
  }

  const auto &object{target.as_object()};
  const auto size{object.size()};
  if (size == value.second.first.size()) {
    // Otherwise why emit this instruction?
    assert(!value.second.first.empty());

    // The idea is to first assume the object property ordering and the
    // hashes collection aligns. If they don't we do a full comparison
    // from where we left of.

    std::size_t index{0};
    for (const auto &entry : object) {
      if (entry.second.type() != value.first) {
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrict, target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (entry.second.type() != value) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopPropertiesTypeStrict);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrictEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrictEvaluate,
                            target.is_object());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (entry.second.type() != value) {
      result = false;
      EVALUATE_END(LoopPropertiesTypeStrictEvaluate);
    }
  }

  evaluator.evaluate(&target);
  EVALUATE_END(LoopPropertiesTypeStrictEvaluate);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrictAny) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrictAny, target.is_object());
  result = true;
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (std::find(value.cbegin(), value.cend(), entry.second.type()) ==
        value.cend()) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopPropertiesTypeStrictAny);
}

INSTRUCTION_HANDLER(LoopPropertiesTypeStrictAnyEvaluate) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopPropertiesTypeStrictAnyEvaluate,
                            target.is_object());
  result = true;
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  for (const auto &entry : target.as_object()) {
    if (std::find(value.cbegin(), value.cend(), entry.second.type()) ==
        value.cend()) {
      result = false;
      EVALUATE_END(LoopPropertiesTypeStrictAnyEvaluate);
    }
  }

  evaluator.evaluate(&target);
  EVALUATE_END(LoopPropertiesTypeStrictAnyEvaluate);
}

INSTRUCTION_HANDLER(LoopKeys) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopKeys, target.is_object());
  assert(!instruction.children.empty());
  result = true;
  for (const auto &entry : target.as_object()) {
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(entry.first);
    }
#endif

    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE_ON_PROPERTY_NAME(child, Evaluator::null,
                                             entry.first)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopKeys);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopKeys);
}

INSTRUCTION_HANDLER(LoopItems) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopItems, target.is_array());
  assert(!instruction.children.empty());
  result = true;

  // To avoid index lookups and unnecessary conditionals
#ifdef SOURCEMETA_EVALUATOR_FAST
  for (const auto &new_instance : target.as_array()) {
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, new_instance)) {
        result = false;
        EVALUATE_END(LoopItems);
      }
    }
  }
#else
  for (std::size_t index = 0; index < target.array_size(); index++) {
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(index);
    }
#endif

    const auto &new_instance{target.at(index)};
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, new_instance)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopItems);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }
#endif

  EVALUATE_END(LoopItems);
}

INSTRUCTION_HANDLER(LoopItemsFrom) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  const auto value{*std::get_if<ValueUnsignedInteger>(&instruction.value)};
  EVALUATE_BEGIN_NON_STRING(LoopItemsFrom,
                            target.is_array() && value < target.array_size());
  assert(!instruction.children.empty());
  result = true;
  for (std::size_t index = value; index < target.array_size(); index++) {
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(index);
    }
#endif

    const auto &new_instance{target.at(index)};
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, new_instance)) {
        result = false;

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
        if (track) {
          evaluator.instance_location.pop_back();
        }
#endif

        EVALUATE_END(LoopItemsFrom);
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif
  }

  EVALUATE_END(LoopItemsFrom);
}

INSTRUCTION_HANDLER(LoopItemsUnevaluated) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopItemsUnevaluated, target.is_array());
  assert(!instruction.children.empty());
  result = true;

  if (!evaluator.is_evaluated(&target)) {
    for (std::size_t index = 0; index < target.array_size(); index++) {
      const auto &new_instance{target.at(index)};
      if (evaluator.is_evaluated(&new_instance)) {
        continue;
      }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
      evaluator.instance_location.push_back(index);
#endif
      for (const auto &child : instruction.children) {
        if (!EVALUATE_RECURSE(child, new_instance)) {
          result = false;
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
          evaluator.instance_location.pop_back();
#endif
          EVALUATE_END(LoopItemsUnevaluated);
        }
      }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
      evaluator.instance_location.pop_back();
#endif
    }

    evaluator.evaluate(&target);
  }

  EVALUATE_END(LoopItemsUnevaluated);
}

INSTRUCTION_HANDLER(LoopItemsType) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopItemsType, target.is_array());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_array()) {
    if (entry.type() != value &&
        // In non-strict mode, we consider a real number that represents an
        // integer to be an integer
        (value != JSON::Type::Integer || !entry.is_integer_real())) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopItemsType);
}

INSTRUCTION_HANDLER(LoopItemsTypeStrict) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopItemsTypeStrict, target.is_array());
  result = true;
  const auto value{*std::get_if<ValueType>(&instruction.value)};
  for (const auto &entry : target.as_array()) {
    if (entry.type() != value) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopItemsTypeStrict);
}

INSTRUCTION_HANDLER(LoopItemsTypeStrictAny) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NON_STRING(LoopItemsTypeStrictAny, target.is_array());
  const auto &value{*std::get_if<ValueTypes>(&instruction.value)};
  // Otherwise we are we even emitting this instruction?
  assert(value.size() > 1);
  result = true;
  for (const auto &entry : target.as_array()) {
    if (std::find(value.cbegin(), value.cend(), entry.type()) == value.cend()) {
      result = false;
      break;
    }
  }

  EVALUATE_END(LoopItemsTypeStrictAny);
}

INSTRUCTION_HANDLER(LoopItemsPropertiesExactlyTypeStrictHash) {
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LoopItemsPropertiesExactlyTypeStrictHash);
  const auto &target{get(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueTypedHashes>(&instruction.value)};
  // Otherwise why emit this instruction?
  assert(!value.second.first.empty());

  if (!target.is_array()) {
    EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
  }

  result = true;

  const auto hashes_size{value.second.first.size()};
  for (const auto &item : target.as_array()) {
    if (!item.is_object()) {
      result = false;
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
    }

    const auto &object{item.as_object()};
    const auto size{object.size()};
    if (size != hashes_size) {
      result = false;
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
    }

    // Unroll, for performance reasons, for small collections
    if (hashes_size == 3) {
      for (const auto &entry : object) {
        if (entry.second.type() != value.first) {
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
        if (entry.second.type() != value.first) {
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
      if (entry.second.type() != value.first ||
          entry.hash != value.second.first[0]) {
        result = false;
        EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
      }
    } else {
      std::size_t index{0};
      for (const auto &entry : object) {
        if (entry.second.type() != value.first) {
          result = false;
          EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash);
        } else if (entry.hash == value.second.first[index]) {
          index += 1;
          continue;
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
  EVALUATE_BEGIN_NO_PRECONDITION(LoopItemsPropertiesExactlyTypeStrictHash3);
  const auto &target{get(instance, instruction.relative_instance_location)};
  // TODO: Take advantage of the table of contents structure to speed up checks
  const auto &value{*std::get_if<ValueTypedHashes>(&instruction.value)};
  assert(value.second.first.size() == 3);
  // Otherwise why emit this instruction?
  assert(!value.second.first.empty());

  if (!target.is_array()) {
    EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
  }

  for (const auto &item : target.as_array()) {
    if (!item.is_object()) {
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
    }

    const auto &object{item.as_object()};
    if (object.size() != 3) {
      EVALUATE_END(LoopItemsPropertiesExactlyTypeStrictHash3);
    }

    const auto &value_1{object.at(0)};
    const auto &value_2{object.at(1)};
    const auto &value_3{object.at(2)};

    if (value_1.second.type() != value.first ||
        value_2.second.type() != value.first ||
        value_3.second.type() != value.first) {
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
  SOURCEMETA_MAYBE_UNUSED(depth);
  SOURCEMETA_MAYBE_UNUSED(schema);
  SOURCEMETA_MAYBE_UNUSED(callback);
  SOURCEMETA_MAYBE_UNUSED(instance);
  SOURCEMETA_MAYBE_UNUSED(property_target);
  SOURCEMETA_MAYBE_UNUSED(evaluator);
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
#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.push_back(index);
    }
#endif

    const auto &new_instance{target.at(index)};
    bool subresult{true};
    for (const auto &child : instruction.children) {
      if (!EVALUATE_RECURSE(child, new_instance)) {
        subresult = false;
        break;
      }
    }

#ifdef SOURCEMETA_EVALUATOR_COMPLETE
    if (track) {
      evaluator.instance_location.pop_back();
    }
#endif

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

using DispatchHandler = bool (*)(const sourcemeta::blaze::Instruction &,
                                 const sourcemeta::blaze::Template &,
                                 const sourcemeta::blaze::Callback &,
                                 const sourcemeta::core::JSON &,
                                 const sourcemeta::core::JSON::String *,
                                 const std::uint64_t depth,
                                 sourcemeta::blaze::Evaluator &);

// Must have same order as InstructionIndex
static constexpr DispatchHandler handlers[95] = {
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
    LoopPropertiesWhitelist,
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
    ControlLabel,
    ControlMark,
    ControlEvaluate,
    ControlJump,
    ControlDynamicAnchorJump};

inline auto
evaluate_instruction(const sourcemeta::blaze::Instruction &instruction,
                     const sourcemeta::blaze::Template &schema,
                     const sourcemeta::blaze::Callback &callback,
                     const sourcemeta::core::JSON &instance,
                     const sourcemeta::core::JSON::String *property_target,
                     const std::uint64_t depth,
                     sourcemeta::blaze::Evaluator &evaluator) -> bool {
  // Guard against infinite recursion in a cheap manner, as
  // infinite recursion will manifest itself through huge
  // ever-growing evaluate paths
  constexpr auto DEPTH_LIMIT{300};
  if (depth > DEPTH_LIMIT) [[unlikely]] {
    throw EvaluationError("The evaluation path depth limit was reached "
                          "likely due to infinite recursion");
  }

  return handlers[static_cast<std::underlying_type_t<InstructionIndex>>(
      instruction.type)](instruction, schema, callback, instance,
                         property_target, depth, evaluator);
}
