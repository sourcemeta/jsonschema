#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::ranges::any_of
#include <cassert>   // assert
#include <sstream>   // std::ostringstream
#include <variant>   // std::visit

namespace {
using namespace sourcemeta::blaze;

template <typename X, typename T>
auto instruction_value(const T &step) -> decltype(auto) {
  if constexpr (requires { step.value; }) {
    return std::get<X>(step.value);
  } else {
    return step.id;
  }
}

auto to_string(const sourcemeta::core::JSON::Type type) -> std::string {
  // Otherwise the type "real" might not make a lot
  // of sense to JSON Schema users
  if (type == sourcemeta::core::JSON::Type::Real) {
    return "number";
  } else {
    std::ostringstream result;
    result << type;
    return result.str();
  }
}

auto escape_string(const std::string &input) -> std::string {
  std::ostringstream result;
  result << '"';

  for (const auto character : input) {
    if (character == '"') {
      result << "\\\"";
    } else {
      result << character;
    }
  }

  result << '"';
  return result.str();
}

auto describe_type_check(const bool valid,
                         const sourcemeta::core::JSON::Type current,
                         const sourcemeta::core::JSON::Type expected,
                         std::ostringstream &message) -> void {
  message << "The value was expected to be of type ";
  message << to_string(expected);
  if (!valid) {
    message << " but it was of type ";
    message << to_string(current);
  }
}

auto describe_types_check(
    const bool valid, const sourcemeta::core::JSON::Type current,
    const std::vector<sourcemeta::core::JSON::Type> &expected,
    std::ostringstream &message) -> void {
  assert(expected.size() > 1);
  auto copy = expected;

  const auto match_real{
      std::ranges::find(copy, sourcemeta::core::JSON::Type::Real)};
  const auto match_integer{
      std::ranges::find(copy, sourcemeta::core::JSON::Type::Integer)};
  if (match_real != copy.cend() && match_integer != copy.cend()) {
    copy.erase(match_integer);
  }

  if (copy.size() == 1) {
    describe_type_check(valid, current, *(copy.cbegin()), message);
    return;
  }

  message << "The value was expected to be of type ";
  for (auto iterator = copy.cbegin(); iterator != copy.cend(); ++iterator) {
    if (std::next(iterator) == copy.cend()) {
      message << "or " << to_string(*iterator);
    } else {
      message << to_string(*iterator) << ", ";
    }
  }

  if (valid) {
    message << " and it was of type ";
  } else {
    message << " but it was of type ";
  }

  if (valid && current == sourcemeta::core::JSON::Type::Integer &&
      std::ranges::find(copy, sourcemeta::core::JSON::Type::Real) !=
          copy.cend()) {
    message << "number";
  } else {
    message << to_string(current);
  }
}

auto describe_reference(const sourcemeta::core::JSON &target) -> std::string {
  std::ostringstream message;
  message << "The " << to_string(target.type())
          << " value was expected to validate against the statically "
             "referenced schema";
  return message.str();
}

auto is_within_keyword(const sourcemeta::core::WeakPointer &evaluate_path,
                       const std::string &keyword) -> bool {
  return std::ranges::any_of(evaluate_path, [&keyword](const auto &token) {
    return token.is_property() && token.to_property() == keyword;
  });
}

auto unknown() -> std::string {
  // In theory we should never get here
  assert(false);
  return "<unknown>";
}

} // namespace

namespace sourcemeta::blaze {

// TODO: What will unlock even better error messages is being able to
// get the subschema being evaluated along with the keyword
auto describe(const bool valid, const Instruction &step,
              const sourcemeta::core::WeakPointer &evaluate_path,
              const sourcemeta::core::WeakPointer &instance_location,
              const sourcemeta::core::JSON &instance,
              const sourcemeta::core::JSON &annotation) -> std::string {
  assert(evaluate_path.empty() || evaluate_path.back().is_property());
  const std::string keyword{
      evaluate_path.empty() ? "" : evaluate_path.back().to_property()};
  const sourcemeta::core::JSON &target{get(instance, instance_location)};

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionFail) {
    if (keyword == "contains") {
      return "The constraints declared for this keyword were not satisfiable";
    }

    if (keyword == "additionalProperties" ||
        keyword == "unevaluatedProperties") {
      std::ostringstream message;
      assert(!instance_location.empty());
      assert(instance_location.back().is_property());
      message << "The object value was not expected to define the property "
              << escape_string(instance_location.back().to_property());
      return message.str();
    }

    if (keyword == "unevaluatedItems") {
      std::ostringstream message;
      assert(!instance_location.empty());
      assert(instance_location.back().is_index());
      message << "The array value was not expected to define the item at index "
              << instance_location.back().to_index();
      return message.str();
    }

    return "No instance is expected to succeed against the false schema";
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalOr) {
    assert(!step.children.empty());
    std::ostringstream message;
    message << "The " << to_string(target.type())
            << " value was expected to validate against ";
    if (step.children.size() > 1) {
      message << "at least one of the " << step.children.size()
              << " given subschemas";
    } else {
      message << "the given subschema";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalAnd) {
    if (keyword == "allOf") {
      assert(!step.children.empty());
      std::ostringstream message;
      message << "The " << to_string(target.type())
              << " value was expected to validate against the ";
      if (step.children.size() > 1) {
        message << step.children.size() << " given subschemas";
      } else {
        message << "given subschema";
      }

      return message.str();
    }

    if (keyword == "properties") {
      assert(!step.children.empty());
      if (!target.is_object()) {
        std::ostringstream message;
        describe_type_check(valid, target.type(),
                            sourcemeta::core::JSON::Type::Object, message);
        return message.str();
      }

      std::ostringstream message;
      message << "The object value was expected to validate against the ";
      if (step.children.size() == 1) {
        message << "single defined property subschema";
      } else {
        // We cannot provide the specific number of properties,
        // as the number of children might be flatten out
        // for performance reasons
        message << "defined properties subschemas";
      }

      return message.str();
    }

    if (keyword == "$ref") {
      return describe_reference(target);
    }

    return unknown();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalXor) {
    assert(!step.children.empty());
    std::ostringstream message;

    if (std::ranges::any_of(evaluate_path,
                            [](const auto &token) {
                              return token.is_property() &&
                                     token.to_property() == "propertyNames";
                            }) &&
        !instance_location.empty() && instance_location.back().is_property()) {
      message << "The property name "
              << escape_string(instance_location.back().to_property());
    } else {
      message << "The " << to_string(target.type()) << " value";
    }

    message << " was expected to validate against ";
    if (step.children.size() > 1) {
      message << "one and only one of the " << step.children.size()
              << " given subschemas";
    } else {
      message << "the given subschema";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalCondition) {
    std::ostringstream message;
    message << "The " << to_string(target.type())
            << " value was expected to validate against the given conditional";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalNot) {
    std::ostringstream message;
    message
        << "The " << to_string(target.type())
        << " value was expected to not validate against the given subschema";
    if (!valid) {
      message << ", but it did";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalNotEvaluate) {
    std::ostringstream message;
    message
        << "The " << to_string(target.type())
        << " value was expected to not validate against the given subschema";
    if (!valid) {
      message << ", but it did";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::ControlLabel) {
    return describe_reference(target);
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::ControlMark) {
    return "The schema location was marked for future use";
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::Evaluate) {
    return "The instance location was marked as evaluated";
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::ControlJump) {
    return describe_reference(target);
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::ControlDynamicAnchorJump) {
    if (keyword == "$dynamicRef") {
      const auto &value{instruction_value<ValueString>(step)};
      std::ostringstream message;
      message << "The " << to_string(target.type())
              << " value was expected to validate against the first subschema "
                 "in scope that declared the dynamic anchor "
              << escape_string(value);
      return message.str();
    }

    assert(keyword == "$recursiveRef");
    std::ostringstream message;
    message << "The " << to_string(target.type())
            << " value was expected to validate against the first subschema "
               "in scope that declared a recursive anchor";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AnnotationEmit) {
    if (keyword == "properties") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The object property " << escape_string(annotation.to_string())
              << " successfully validated against its property "
                 "subschema";
      return message.str();
    }

    if ((keyword == "items" || keyword == "additionalItems") &&
        annotation.is_boolean() && annotation.to_boolean()) {
      assert(target.is_array());
      return "Every item in the array value was successfully validated";
    }

    if ((keyword == "prefixItems" || keyword == "items") &&
        annotation.is_integer()) {
      assert(target.is_array());
      assert(annotation.is_positive());
      std::ostringstream message;
      if (annotation.to_integer() == 0) {
        message << "The first item of the array value successfully validated "
                   "against the first "
                   "positional subschema";
      } else {
        message << "The first " << annotation.to_integer() + 1
                << " items of the array value successfully validated against "
                   "the given "
                   "positional subschemas";
      }

      return message.str();
    }

    if (keyword == "prefixItems" && annotation.is_boolean() &&
        annotation.to_boolean()) {
      assert(target.is_array());
      std::ostringstream message;
      message << "Every item of the array value validated against the given "
                 "positional subschemas";
      return message.str();
    }

    if (keyword == "title" || keyword == "description") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The " << keyword << " of the";
      if (instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " was " << escape_string(annotation.to_string());
      return message.str();
    }

    if (keyword == "default") {
      std::ostringstream message;
      message << "The default value of the";
      if (instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " was ";
      stringify(annotation, message);
      return message.str();
    }

    if (keyword == "deprecated" && annotation.is_boolean()) {
      std::ostringstream message;
      if (instance_location.empty()) {
        message << "The instance";
      } else {
        message << "The instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      if (annotation.to_boolean()) {
        message << " was considered deprecated";
      } else {
        message << " was not considered deprecated";
      }

      return message.str();
    }

    if (keyword == "readOnly" && annotation.is_boolean()) {
      std::ostringstream message;
      if (instance_location.empty()) {
        message << "The instance";
      } else {
        message << "The instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      if (annotation.to_boolean()) {
        message << " was considered read-only";
      } else {
        message << " was not considered read-only";
      }

      return message.str();
    }

    if (keyword == "writeOnly" && annotation.is_boolean()) {
      std::ostringstream message;
      if (instance_location.empty()) {
        message << "The instance";
      } else {
        message << "The instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      if (annotation.to_boolean()) {
        message << " was considered write-only";
      } else {
        message << " was not considered write-only";
      }

      return message.str();
    }

    if (keyword == "examples") {
      assert(annotation.is_array());
      std::ostringstream message;
      if (instance_location.empty()) {
        message << "Examples of the instance";
      } else {
        message << "Examples of the instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " were ";
      for (auto iterator = annotation.as_array().cbegin();
           iterator != annotation.as_array().cend(); ++iterator) {
        if (std::next(iterator) == annotation.as_array().cend()) {
          message << "and ";
          stringify(*iterator, message);
        } else {
          stringify(*iterator, message);
          message << ", ";
        }
      }

      return message.str();
    }

    if (keyword == "contentEncoding") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The content encoding of the";
      if (instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " was " << escape_string(annotation.to_string());
      return message.str();
    }

    if (keyword == "contentMediaType") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The content media type of the";
      if (instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " was " << escape_string(annotation.to_string());
      return message.str();
    }

    if (keyword == "contentSchema") {
      std::ostringstream message;
      message << "When decoded, the";
      if (instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " was expected to validate against the schema ";
      stringify(annotation, message);
      return message.str();
    }

    if (keyword == "format") {
      std::ostringstream message;
      message << "The logical type of the";
      if (instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(instance_location, message);
        message << "\"";
      }

      message << " was expected to be ";
      stringify(annotation, message);
      return message.str();
    }

    std::ostringstream message;
    message << "The unrecognized keyword " << escape_string(keyword)
            << " was collected as the annotation ";
    stringify(annotation, message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AnnotationToParent) {
    if (keyword == "unevaluatedItems" && annotation.is_boolean() &&
        annotation.to_boolean()) {
      assert(target.is_array());
      std::ostringstream message;
      message << "At least one item of the array value successfully validated "
                 "against the subschema for unevaluated items";
      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent) {
    if (keyword == "patternProperties") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The object property " << escape_string(annotation.to_string())
              << " successfully validated against its pattern property "
                 "subschema";
      return message.str();
    }

    if (keyword == "additionalProperties") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The object property " << escape_string(annotation.to_string())
              << " successfully validated against the additional properties "
                 "subschema";
      return message.str();
    }

    if (keyword == "unevaluatedProperties") {
      assert(annotation.is_string());
      std::ostringstream message;
      message << "The object property " << escape_string(annotation.to_string())
              << " successfully validated against the subschema for "
                 "unevaluated properties";
      return message.str();
    }

    if (keyword == "contains" && annotation.is_integer()) {
      assert(target.is_array());
      assert(annotation.is_positive());
      std::ostringstream message;
      message << "The item at index " << annotation.to_integer()
              << " of the array value successfully validated against the "
                 "containment check subschema";
      return message.str();
    }

    return unknown();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopProperties) {
    assert(keyword == "additionalProperties" ||
           keyword == "unevaluatedProperties");
    std::ostringstream message;
    if (!step.children.empty() &&
        step.children.front().type == InstructionIndex::AssertionFail) {
      if (keyword == "unevaluatedProperties") {
        message << "The object value was not expected to define unevaluated "
                   "properties";
      } else {
        message << "The object value was not expected to define additional "
                   "properties";
      }
    } else if (keyword == "unevaluatedProperties") {
      message << "The object properties not covered by other object "
                 "keywords were expected to validate against this subschema";
    } else {
      message << "The object properties not covered by other adjacent object "
                 "keywords were expected to validate against this subschema";
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesEvaluate) {
    assert(keyword == "additionalProperties");
    std::ostringstream message;
    if (step.children.size() == 1 &&
        step.children.front().type == InstructionIndex::AssertionFail) {
      message << "The object value was not expected to define additional "
                 "properties";
    } else {
      message << "The object properties not covered by other adjacent object "
                 "keywords were expected to validate against this subschema";
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesUnevaluated) {
    if (keyword == "unevaluatedProperties") {
      std::ostringstream message;
      if (!step.children.empty() &&
          step.children.front().type == InstructionIndex::AssertionFail) {
        message << "The object value was not expected to define unevaluated "
                   "properties";
      } else {
        message << "The object properties not covered by other object "
                   "keywords were expected to validate against this subschema";
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesUnevaluatedExcept) {
    if (keyword == "unevaluatedProperties") {
      std::ostringstream message;
      if (!step.children.empty() &&
          step.children.front().type == InstructionIndex::AssertionFail) {
        message << "The object value was not expected to define unevaluated "
                   "properties";
      } else {
        message << "The object properties not covered by other object "
                   "keywords were expected to validate against this subschema";
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesExcept) {
    assert(keyword == "additionalProperties" ||
           keyword == "unevaluatedProperties");
    std::ostringstream message;
    if (!step.children.empty() &&
        step.children.front().type == InstructionIndex::AssertionFail) {
      if (keyword == "unevaluatedProperties") {
        message << "The object value was not expected to define unevaluated "
                   "properties";
      } else {
        message << "The object value was not expected to define additional "
                   "properties";
      }
    } else if (keyword == "unevaluatedProperties") {
      message << "The object properties not covered by other object "
                 "keywords were expected to validate against this subschema";
    } else {
      message << "The object properties not covered by other adjacent object "
                 "keywords were expected to validate against this subschema";
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesWhitelist) {
    assert(keyword == "additionalProperties");
    return "The object value was not expected to define additional properties";
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesExactlyTypeStrict) {
    std::ostringstream message;
    message << "The required object properties were expected to be of type "
            << to_string(instruction_value<ValueTypedProperties>(step).first);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesExactlyTypeStrictHash) {
    std::ostringstream message;
    message << "The required object properties were expected to be of type "
            << to_string(instruction_value<ValueTypedHashes>(step).first);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::
                       LoopItemsPropertiesExactlyTypeStrictHash ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopItemsPropertiesExactlyTypeStrictHash3) {
    std::ostringstream message;
    message << "Every item in the array was expected to be an object whose "
               "required properties were of type "
            << to_string(instruction_value<ValueTypedHashes>(step).first);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesType) {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(instruction_value<ValueType>(step));
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeEvaluate) {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(instruction_value<ValueType>(step));
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrict) {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(instruction_value<ValueType>(step));
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrictEvaluate) {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(instruction_value<ValueType>(step));
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrictAny) {
    std::ostringstream message;
    message << "The object properties were expected to be of type ";
    const auto &types{instruction_value<ValueTypes>(step)};
    for (auto iterator = types.cbegin(); iterator != types.cend(); ++iterator) {
      if (std::next(iterator) == types.cend()) {
        message << "or " << to_string(*iterator);
      } else {
        message << to_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesTypeStrictAnyEvaluate) {
    std::ostringstream message;
    message << "The object properties were expected to be of type ";
    const auto &types{instruction_value<ValueTypes>(step)};
    for (auto iterator = types.cbegin(); iterator != types.cend(); ++iterator) {
      if (std::next(iterator) == types.cend()) {
        message << "or " << to_string(*iterator);
      } else {
        message << to_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopKeys) {
    assert(keyword == "propertyNames");
    assert(target.is_object());
    std::ostringstream message;

    if (target.size() == 0) {
      assert(valid);
      message << "The object is empty and no properties were expected to "
                 "validate against the given subschema";
    } else if (target.size() == 1) {
      message << "The object property ";
      message << escape_string(target.as_object().cbegin()->first);
      message << " was expected to validate against the given subschema";
    } else {
      message << "The object properties ";
      for (auto iterator = target.as_object().cbegin();
           iterator != target.as_object().cend(); ++iterator) {
        if (std::next(iterator) == target.as_object().cend()) {
          message << "and " << escape_string(iterator->first);
        } else {
          message << escape_string(iterator->first) << ", ";
        }
      }

      message << " were expected to validate against the given subschema";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopItems) {
    assert(target.is_array());
    return "Every item in the array value was expected to validate against the "
           "given subschema";
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopItemsFrom) {
    assert(target.is_array());
    const auto &value{instruction_value<ValueUnsignedInteger>(step)};
    std::ostringstream message;
    message << "Every item in the array value";
    if (value == 1) {
      message << " except for the first one";
    } else if (value > 0) {
      message << " except for the first " << value;
    }

    message << " was expected to validate against the given subschema";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopItemsUnevaluated) {
    assert(keyword == "unevaluatedItems");
    std::ostringstream message;
    message << "The array items not covered by other array keywords, if any, "
               "were expected to validate against this subschema";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopItemsType) {
    std::ostringstream message;
    message << "The array items were expected to be of type "
            << to_string(instruction_value<ValueType>(step));
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopItemsTypeStrict) {
    std::ostringstream message;
    message << "The array items were expected to be of type "
            << to_string(instruction_value<ValueType>(step));
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopItemsTypeStrictAny) {
    std::ostringstream message;
    message << "The array items were expected to be of type ";
    const auto &types{instruction_value<ValueTypes>(step)};
    for (auto iterator = types.cbegin(); iterator != types.cend(); ++iterator) {
      if (std::next(iterator) == types.cend()) {
        message << "or " << to_string(*iterator);
      } else {
        message << to_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopContains) {
    assert(target.is_array());
    std::ostringstream message;
    const auto &value{instruction_value<ValueRange>(step)};
    const auto minimum{std::get<0>(value)};
    const auto maximum{std::get<1>(value)};
    bool plural{true};

    message << "The array value was expected to contain ";
    if (maximum.has_value()) {
      if (minimum == maximum.value() && minimum == 0) {
        message << "any number of";
      } else if (minimum == maximum.value()) {
        message << "exactly " << minimum;
        if (minimum == 1) {
          plural = false;
        }
      } else if (minimum == 0) {
        message << "up to " << maximum.value();
        if (maximum.value() == 1) {
          plural = false;
        }
      } else {
        message << minimum << " to " << maximum.value();
        if (maximum.value() == 1) {
          plural = false;
        }
      }
    } else {
      message << "at least " << minimum;
      if (minimum == 1) {
        plural = false;
      }
    }

    if (plural) {
      message << " items that validate against the given subschema";
    } else {
      message << " item that validates against the given subschema";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionDefines) {
    std::ostringstream message;
    message << "The object value was expected to define the property "
            << escape_string(instruction_value<ValueProperty>(step).first);
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesStrict) {
    std::ostringstream message;
    message
        << "The value was expected to be an object that defines the property "
        << escape_string(instruction_value<ValueProperty>(step).first);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionDefinesAll) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);

    std::vector<ValueString> value_vector;
    for (const auto &entry : value) {
      value_vector.push_back(entry.first);
    }

    std::ostringstream message;
    message << "The object value was expected to define properties ";
    for (auto iterator = value_vector.cbegin(); iterator != value_vector.cend();
         ++iterator) {
      if (std::next(iterator) == value_vector.cend()) {
        message << "and " << escape_string(*iterator);
      } else {
        message << escape_string(*iterator) << ", ";
      }
    }

    if (valid) {
      return message.str();
    }

    assert(target.is_object());
    std::set<std::string> missing;
    for (const auto &property : value) {
      if (!target.defines(property.first, property.second)) {
        missing.insert(property.first);
      }
    }

    assert(!missing.empty());
    if (missing.size() == 1) {
      message << " but did not define the property "
              << escape_string(*(missing.cbegin()));
    } else {
      message << " but did not define properties ";
      for (auto iterator = missing.cbegin(); iterator != missing.cend();
           ++iterator) {
        if (std::next(iterator) == missing.cend()) {
          message << "and " << escape_string(*iterator);
        } else {
          message << escape_string(*iterator) << ", ";
        }
      }
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesAllStrict) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);

    std::vector<ValueString> value_vector;
    for (const auto &entry : value) {
      value_vector.push_back(entry.first);
    }

    std::ostringstream message;
    message
        << "The value was expected to be an object that defines properties ";
    for (auto iterator = value_vector.cbegin(); iterator != value_vector.cend();
         ++iterator) {
      if (std::next(iterator) == value_vector.cend()) {
        message << "and " << escape_string(*iterator);
      } else {
        message << escape_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesExactly) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);
    std::vector<ValueString> value_vector;
    for (const auto &entry : value) {
      value_vector.push_back(entry.first);
    }

    std::ranges::sort(value_vector);
    std::ostringstream message;
    message << "The object value was expected to only define properties ";
    for (auto iterator = value_vector.cbegin(); iterator != value_vector.cend();
         ++iterator) {
      if (std::next(iterator) == value_vector.cend()) {
        message << "and " << escape_string(*iterator);
      } else {
        message << escape_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesExactlyStrict) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);
    std::vector<ValueString> value_vector;
    for (const auto &entry : value) {
      value_vector.push_back(entry.first);
    }

    std::ranges::sort(value_vector);
    std::ostringstream message;
    message << "The value was expected to be an object that only defines "
               "properties ";
    for (auto iterator = value_vector.cbegin(); iterator != value_vector.cend();
         ++iterator) {
      if (std::next(iterator) == value_vector.cend()) {
        message << "and " << escape_string(*iterator);
      } else {
        message << escape_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesExactlyStrictHash3) {
    const auto &value{instruction_value<ValueStringHashes>(step).first};
    std::ostringstream message;
    message << "The value was expected to be an object that only defines "
               "the "
            << value.size() << " given properties";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionType) {
    std::ostringstream message;
    describe_type_check(valid, target.type(),
                        instruction_value<ValueType>(step), message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionTypeStrict) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueType>(step)};
    if (!valid && value == sourcemeta::core::JSON::Type::Real &&
        target.type() == sourcemeta::core::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!valid && value == sourcemeta::core::JSON::Type::Integer &&
               target.type() == sourcemeta::core::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(valid, target.type(), value, message);
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionTypeAny) {
    std::ostringstream message;
    describe_types_check(valid, target.type(),
                         instruction_value<ValueTypes>(step), message);
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny) {
    std::ostringstream message;
    describe_types_check(valid, target.type(),
                         instruction_value<ValueTypes>(step), message);
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeStringBounded) {
    std::ostringstream message;

    const auto minimum{std::get<0>(instruction_value<ValueRange>(step))};
    const auto maximum{std::get<1>(instruction_value<ValueRange>(step))};
    if (minimum == 0 && maximum.has_value()) {
      message << "The value was expected to consist of a string of at most "
              << maximum.value()
              << (maximum.value() == 1 ? " character" : " characters");
    } else if (maximum.has_value()) {
      message << "The value was expected to consist of a string of " << minimum
              << " to " << maximum.value()
              << (maximum.value() == 1 ? " character" : " characters");
    } else {
      message << "The value was expected to consist of a string of at least "
              << minimum << (minimum == 1 ? " character" : " characters");
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeStringUpper) {
    std::ostringstream message;
    message << "The value was expected to consist of a string of at most "
            << instruction_value<ValueUnsignedInteger>(step)
            << (instruction_value<ValueUnsignedInteger>(step) == 1
                    ? " character"
                    : " characters");
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeArrayBounded) {
    std::ostringstream message;

    const auto minimum{std::get<0>(instruction_value<ValueRange>(step))};
    const auto maximum{std::get<1>(instruction_value<ValueRange>(step))};
    if (minimum == 0 && maximum.has_value()) {
      message << "The value was expected to consist of an array of at most "
              << maximum.value() << (maximum.value() == 1 ? " item" : " items");
    } else if (maximum.has_value()) {
      message << "The value was expected to consist of an array of " << minimum
              << " to " << maximum.value()
              << (maximum.value() == 1 ? " item" : " items");
    } else {
      message << "The value was expected to consist of an array of at least "
              << minimum << (minimum == 1 ? " item" : " items");
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeArrayUpper) {
    std::ostringstream message;
    message << "The value was expected to consist of an array of at most "
            << instruction_value<ValueUnsignedInteger>(step)
            << (instruction_value<ValueUnsignedInteger>(step) == 1 ? " item"
                                                                   : " items");
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeObjectBounded) {
    std::ostringstream message;

    const auto minimum{std::get<0>(instruction_value<ValueRange>(step))};
    const auto maximum{std::get<1>(instruction_value<ValueRange>(step))};
    if (minimum == 0 && maximum.has_value()) {
      message << "The value was expected to consist of an object of at most "
              << maximum.value()
              << (maximum.value() == 1 ? " property" : " properties");
    } else if (maximum.has_value()) {
      message << "The value was expected to consist of an object of " << minimum
              << " to " << maximum.value()
              << (maximum.value() == 1 ? " property" : " properties");
    } else {
      message << "The value was expected to consist of an object of at least "
              << minimum << (minimum == 1 ? " property" : " properties");
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionTypeObjectUpper) {
    std::ostringstream message;
    message << "The value was expected to consist of an object of at most "
            << instruction_value<ValueUnsignedInteger>(step)
            << (instruction_value<ValueUnsignedInteger>(step) == 1
                    ? " property"
                    : " properties");
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionRegex) {
    if (std::ranges::any_of(evaluate_path,
                            [](const auto &token) {
                              return token.is_property() &&
                                     token.to_property() == "propertyNames";
                            }) &&
        !instance_location.empty() && instance_location.back().is_property()) {
      std::ostringstream message;
      message << "The property name "
              << escape_string(instance_location.back().to_property())
              << " was expected to match the regular expression "
              << escape_string(instruction_value<ValueRegex>(step).second);
      return message.str();
    }

    assert(target.is_string());
    std::ostringstream message;
    message << "The string value " << escape_string(target.to_string())
            << " was expected to match the regular expression "
            << escape_string(instruction_value<ValueRegex>(step).second);
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionStringSizeLess) {
    if (keyword == "maxLength") {
      std::ostringstream message;
      const auto maximum{instruction_value<ValueUnsignedInteger>(step) - 1};

      if (is_within_keyword(evaluate_path, "propertyNames")) {
        assert(instance_location.back().is_property());
        message << "The object property name "
                << escape_string(instance_location.back().to_property());
      } else {
        message << "The string value ";
        stringify(target, message);
      }

      message << " was expected to consist of at most " << maximum
              << (maximum == 1 ? " character" : " characters");

      if (valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it consisted of ";

      if (is_within_keyword(evaluate_path, "propertyNames")) {
        message << instance_location.back().to_property().size();
        message << (instance_location.back().to_property().size() == 1
                        ? " character"
                        : " characters");
      } else {
        message << target.size();
        message << (target.size() == 1 ? " character" : " characters");
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionStringSizeGreater) {
    if (keyword == "minLength") {
      std::ostringstream message;
      const auto minimum{instruction_value<ValueUnsignedInteger>(step) + 1};

      if (is_within_keyword(evaluate_path, "propertyNames")) {
        assert(instance_location.back().is_property());
        message << "The object property name "
                << escape_string(instance_location.back().to_property());
      } else {
        message << "The string value ";
        stringify(target, message);
      }

      message << " was expected to consist of at least " << minimum
              << (minimum == 1 ? " character" : " characters");

      if (valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it consisted of ";

      if (is_within_keyword(evaluate_path, "propertyNames")) {
        message << instance_location.back().to_property().size();
        message << (instance_location.back().to_property().size() == 1
                        ? " character"
                        : " characters");
      } else {
        message << target.size();
        message << (target.size() == 1 ? " character" : " characters");
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionArraySizeLess) {
    if (keyword == "maxItems") {
      assert(target.is_array());
      std::ostringstream message;
      const auto maximum{instruction_value<ValueUnsignedInteger>(step) - 1};
      message << "The array value was expected to contain at most " << maximum;
      if (maximum == 1) {
        message << " item";
      } else {
        message << " items";
      }

      if (valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it contained " << target.size();
      if (target.size() == 1) {
        message << " item";
      } else {
        message << " items";
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionArraySizeGreater) {
    assert(target.is_array());
    std::ostringstream message;
    const auto minimum{instruction_value<ValueUnsignedInteger>(step) + 1};
    message << "The array value was expected to contain at least " << minimum;
    if (minimum == 1) {
      message << " item";
    } else {
      message << " items";
    }

    if (valid) {
      message << " and";
    } else {
      message << " but";
    }

    message << " it contained " << target.size();
    if (target.size() == 1) {
      message << " item";
    } else {
      message << " items";
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeLess) {
    if (keyword == "additionalProperties") {
      return "The object value was not expected to define additional "
             "properties";
    }

    if (keyword == "maxProperties") {
      assert(target.is_object());
      std::ostringstream message;
      const auto maximum{instruction_value<ValueUnsignedInteger>(step) - 1};
      message << "The object value was expected to contain at most " << maximum;
      if (maximum == 1) {
        message << " property";
      } else {
        message << " properties";
      }

      if (valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it contained " << target.size();
      if (target.size() == 0) {
        message << " properties";
      } else if (target.size() == 1) {
        message << " property: ";
        message << escape_string(target.as_object().cbegin()->first);
      } else {
        message << " properties: ";

        std::vector<std::string> properties;
        for (const auto &entry : target.as_object()) {
          properties.push_back(entry.first);
        }
        std::ranges::sort(properties);

        for (auto iterator = properties.cbegin(); iterator != properties.cend();
             ++iterator) {
          if (std::next(iterator) == properties.cend()) {
            message << "and " << escape_string(*iterator);
          } else {
            message << escape_string(*iterator) << ", ";
          }
        }
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeGreater) {
    if (keyword == "minProperties") {
      assert(target.is_object());
      std::ostringstream message;
      const auto minimum{instruction_value<ValueUnsignedInteger>(step) + 1};
      message << "The object value was expected to contain at least "
              << minimum;
      if (minimum == 1) {
        message << " property";
      } else {
        message << " properties";
      }

      if (valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it contained " << target.size();
      if (target.size() == 1) {
        message << " property: ";
        message << escape_string(target.as_object().cbegin()->first);
      } else {
        message << " properties: ";
        std::vector<std::string> properties;
        for (const auto &entry : target.as_object()) {
          properties.push_back(entry.first);
        }
        std::ranges::sort(properties);

        for (auto iterator = properties.cbegin(); iterator != properties.cend();
             ++iterator) {
          if (std::next(iterator) == properties.cend()) {
            message << "and " << escape_string(*iterator);
          } else {
            message << escape_string(*iterator) << ", ";
          }
        }
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionEqual) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueJSON>(step)};

    if (std::ranges::any_of(evaluate_path,
                            [](const auto &token) {
                              return token.is_property() &&
                                     token.to_property() == "propertyNames";
                            }) &&
        !instance_location.empty() && instance_location.back().is_property()) {
      message << "The property name "
              << escape_string(instance_location.back().to_property());
    } else {
      message << "The " << to_string(target.type()) << " value ";
      stringify(target, message);
    }

    message << " was expected to equal the " << to_string(value.type())
            << " constant ";
    stringify(value, message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionGreaterEqual) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueJSON>(step)};
    message << "The " << to_string(target.type()) << " value ";
    stringify(target, message);
    message << " was expected to be greater than or equal to the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionLessEqual) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueJSON>(step)};
    message << "The " << to_string(target.type()) << " value ";
    stringify(target, message);
    message << " was expected to be less than or equal to the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionGreater) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueJSON>(step)};
    message << "The " << to_string(target.type()) << " value ";
    stringify(target, message);
    message << " was expected to be greater than the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    if (!valid && value == target) {
      message << ", but they were equal";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionLess) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueJSON>(step)};
    message << "The " << to_string(target.type()) << " value ";
    stringify(target, message);
    message << " was expected to be less than the " << to_string(value.type())
            << " ";
    stringify(value, message);
    if (!valid && value == target) {
      message << ", but they were equal";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionUnique) {
    assert(target.is_array());
    auto array{target.as_array()};
    std::ostringstream message;
    if (valid) {
      message << "The array value was expected to not contain duplicate items";
    } else {
      std::set<sourcemeta::core::JSON> duplicates;
      for (auto iterator = array.cbegin(); iterator != array.cend();
           ++iterator) {
        for (auto subiterator = std::next(iterator);
             subiterator != array.cend(); ++subiterator) {
          if (*iterator == *subiterator) {
            duplicates.insert(*iterator);
          }
        }
      }

      assert(!duplicates.empty());
      message << "The array value contained the following duplicate";
      if (duplicates.size() == 1) {
        message << " item: ";
        stringify(*(duplicates.cbegin()), message);
      } else {
        message << " items: ";
        for (auto subiterator = duplicates.cbegin();
             subiterator != duplicates.cend(); ++subiterator) {
          if (std::next(subiterator) == duplicates.cend()) {
            message << "and ";
            stringify(*subiterator, message);
          } else {
            stringify(*subiterator, message);
            message << ", ";
          }
        }
      }
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionDivisible) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueJSON>(step)};
    message << "The " << to_string(target.type()) << " value ";
    stringify(target, message);
    message << " was expected to be divisible by the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionEqualsAny) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueSet>(step)};
    assert(!value.empty());

    if (std::ranges::any_of(evaluate_path,
                            [](const auto &token) {
                              return token.is_property() &&
                                     token.to_property() == "propertyNames";
                            }) &&
        !instance_location.empty() && instance_location.back().is_property()) {
      message << "The property name "
              << escape_string(instance_location.back().to_property());
    } else {
      message << "The " << to_string(target.type()) << " value ";
      stringify(target, message);
    }

    if (value.size() == 1) {
      message << " was expected to equal the "
              << to_string(value.cbegin()->type()) << " constant ";
      stringify(*(value.cbegin()), message);
    } else {
      if (valid) {
        message << " was expected to equal one of the " << value.size()
                << " declared values";
      } else {
        message << " was expected to equal one of the following values: ";
        std::vector<sourcemeta::core::JSON> copy{value.cbegin(), value.cend()};
        std::ranges::sort(copy);
        for (auto iterator = copy.cbegin(); iterator != copy.cend();
             ++iterator) {
          if (std::next(iterator) == copy.cend()) {
            message << "and ";
            stringify(*iterator, message);
          } else {
            stringify(*iterator, message);
            message << ", ";
          }
        }
      }
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionEqualsAnyStringHash) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueStringHashes>(step).first};
    assert(!value.empty());

    if (std::ranges::any_of(evaluate_path,
                            [](const auto &token) {
                              return token.is_property() &&
                                     token.to_property() == "propertyNames";
                            }) &&
        !instance_location.empty() && instance_location.back().is_property()) {
      message << "The property name "
              << escape_string(instance_location.back().to_property());
    } else {
      message << "The " << to_string(target.type()) << " value ";
      stringify(target, message);
    }

    if (value.size() == 1) {
      message << " was expected to equal the given constant";
    } else {
      message << " was expected to equal one of the given declared values";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionStringType) {
    assert(target.is_string());
    std::ostringstream message;
    message << "The string value " << escape_string(target.to_string())
            << " was expected to represent a valid";
    switch (instruction_value<ValueStringType>(step)) {
      case ValueStringType::URI:
        message << " URI";
        break;
      default:
        return unknown();
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionPropertyType) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueType>(step)};
    if (!valid && value == sourcemeta::core::JSON::Type::Real &&
        target.type() == sourcemeta::core::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!valid && value == sourcemeta::core::JSON::Type::Integer &&
               target.type() == sourcemeta::core::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(valid, target.type(), value, message);
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeEvaluate) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueType>(step)};
    if (!valid && value == sourcemeta::core::JSON::Type::Real &&
        target.type() == sourcemeta::core::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!valid && value == sourcemeta::core::JSON::Type::Integer &&
               target.type() == sourcemeta::core::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(valid, target.type(), value, message);
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeStrict) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueType>(step)};
    if (!valid && value == sourcemeta::core::JSON::Type::Real &&
        target.type() == sourcemeta::core::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!valid && value == sourcemeta::core::JSON::Type::Integer &&
               target.type() == sourcemeta::core::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(valid, target.type(), value, message);
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionPropertyTypeStrictEvaluate) {
    std::ostringstream message;
    const auto &value{instruction_value<ValueType>(step)};
    if (!valid && value == sourcemeta::core::JSON::Type::Real &&
        target.type() == sourcemeta::core::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!valid && value == sourcemeta::core::JSON::Type::Integer &&
               target.type() == sourcemeta::core::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(valid, target.type(), value, message);
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeStrictAny) {
    std::ostringstream message;
    describe_types_check(valid, target.type(),
                         instruction_value<ValueTypes>(step), message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionPropertyTypeStrictAnyEvaluate) {
    std::ostringstream message;
    describe_types_check(valid, target.type(),
                         instruction_value<ValueTypes>(step), message);
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionArrayPrefix) {
    assert(keyword == "items" || keyword == "prefixItems");
    assert(!step.children.empty());
    assert(target.is_array());

    std::ostringstream message;
    message << "The first ";
    if (step.children.size() <= 2) {
      message << "item of the array value was";
    } else {
      message << (step.children.size() - 1) << " items of the array value were";
    }

    message << " expected to validate against the corresponding subschemas";
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionArrayPrefixEvaluate) {
    assert(keyword == "items" || keyword == "prefixItems");
    assert(!step.children.empty());
    assert(target.is_array());

    std::ostringstream message;
    message << "The first ";
    if (step.children.size() <= 2) {
      message << "item of the array value was";
    } else {
      message << (step.children.size() - 1) << " items of the array value were";
    }

    message << " expected to validate against the corresponding subschemas";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesMatch) {
    assert(!step.children.empty());
    assert(target.is_object());
    std::ostringstream message;
    message << "The object value was expected to validate against the ";
    if (step.children.size() == 1) {
      message << "single defined property subschema";
    } else {
      message << step.children.size() << " defined properties subschemas";
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesMatchClosed) {
    assert(!step.children.empty());
    assert(target.is_object());
    std::ostringstream message;
    if (step.children.size() == 1) {
      message << "The object value was expected to validate against the ";
      message << "single defined property subschema";
    } else {
      message
          << "Every object value was expected to validate against one of the ";
      message << step.children.size() << " defined properties subschemas";
    }

    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalWhenDefines) {
    std::ostringstream message;
    message << "The object value defined the property \""
            << instruction_value<ValueProperty>(step).first << "\"";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesRegex) {
    assert(target.is_object());
    std::ostringstream message;
    message << "The object properties that match the regular expression \""
            << instruction_value<ValueRegex>(step).second
            << "\" were expected to validate against the defined pattern "
               "property subschema";
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesRegexClosed) {
    assert(target.is_object());
    std::ostringstream message;
    message << "The object properties were expected to match the regular "
               "expression \""
            << instruction_value<ValueRegex>(step).second
            << "\" and validate against the defined pattern "
               "property subschema";
    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesStartsWith) {
    assert(target.is_object());
    std::ostringstream message;
    message << "The object properties that start with the string \""
            << instruction_value<ValueString>(step)
            << "\" were expected to validate against the defined pattern "
               "property subschema";
    return message.str();
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalWhenType) {
    if (keyword == "items") {
      std::ostringstream message;
      describe_type_check(valid, target.type(),
                          instruction_value<ValueType>(step), message);
      return message.str();
    }

    if (keyword == "dependencies") {
      assert(target.is_object());
      assert(!step.children.empty());

      std::set<std::string> present;
      std::set<std::string> present_with_schemas;
      std::set<std::string> present_with_properties;
      std::set<std::string> all_dependencies;
      std::set<std::string> required_properties;

      for (const auto &child : step.children) {
        // Schema
        if (child.type == InstructionIndex::LogicalWhenDefines) {
          const auto &substep{child};
          const auto &property{instruction_value<ValueProperty>(substep).first};
          all_dependencies.insert(property);
          if (!target.defines(property)) {
            continue;
          }

          present.insert(property);
          present_with_schemas.insert(property);

          // Properties
        } else {
          assert(child.type == InstructionIndex::AssertionPropertyDependencies);
          const auto &substep{child};

          for (const auto &entry : std::get<ValueStringMap>(substep.value)) {
            all_dependencies.insert(entry.first);
            if (target.defines(entry.first)) {
              present.insert(entry.first);
              present_with_properties.insert(entry.first);
              for (const auto &dependency : entry.second) {
                if (valid || !target.defines(dependency)) {
                  required_properties.insert(dependency);
                }
              }
            }
          }
        }
      }

      std::ostringstream message;

      if (present_with_schemas.empty() && present_with_properties.empty()) {
        message << "The object value did not define the";
        assert(!all_dependencies.empty());
        if (all_dependencies.size() == 1) {
          message << " property "
                  << escape_string(*(all_dependencies.cbegin()));
        } else {
          message << " properties ";
          for (auto iterator = all_dependencies.cbegin();
               iterator != all_dependencies.cend(); ++iterator) {
            if (std::next(iterator) == all_dependencies.cend()) {
              message << "or " << escape_string(*iterator);
            } else {
              message << escape_string(*iterator) << ", ";
            }
          }
        }

        return message.str();
      }

      if (present.size() == 1) {
        message << "Because the object value defined the";
        message << " property " << escape_string(*(present.cbegin()));
      } else {
        message << "Because the object value defined the";
        message << " properties ";
        for (auto iterator = present.cbegin(); iterator != present.cend();
             ++iterator) {
          if (std::next(iterator) == present.cend()) {
            message << "and " << escape_string(*iterator);
          } else {
            message << escape_string(*iterator) << ", ";
          }
        }
      }

      if (!required_properties.empty()) {
        message << ", it was also expected to define the";
        if (required_properties.size() == 1) {
          message << " property "
                  << escape_string(*(required_properties.cbegin()));
        } else {
          message << " properties ";
          for (auto iterator = required_properties.cbegin();
               iterator != required_properties.cend(); ++iterator) {
            if (std::next(iterator) == required_properties.cend()) {
              message << "and " << escape_string(*iterator);
            } else {
              message << escape_string(*iterator) << ", ";
            }
          }
        }
      }

      if (!present_with_schemas.empty()) {
        message << ", ";
        if (!required_properties.empty()) {
          message << "and ";
        }

        message << "it was also expected to successfully validate against the "
                   "corresponding ";
        if (present_with_schemas.size() == 1) {
          message << escape_string(*(present_with_schemas.cbegin()));
          message << " subschema";
        } else {
          for (auto iterator = present_with_schemas.cbegin();
               iterator != present_with_schemas.cend(); ++iterator) {
            if (std::next(iterator) == present_with_schemas.cend()) {
              message << "and " << escape_string(*iterator);
            } else {
              message << escape_string(*iterator) << ", ";
            }
          }

          message << " subschemas";
        }
      }

      return message.str();
    }

    if (keyword == "dependentSchemas") {
      assert(target.is_object());
      assert(!step.children.empty());
      std::set<std::string> present;
      std::set<std::string> all_dependencies;
      for (const auto &child : step.children) {
        assert(child.type == InstructionIndex::LogicalWhenDefines);
        const auto &substep{child};
        const auto &property{instruction_value<ValueProperty>(substep).first};
        all_dependencies.insert(property);
        if (!target.defines(property)) {
          continue;
        }

        present.insert(property);
      }

      std::ostringstream message;

      if (present.empty()) {
        message << "The object value did not define the";
        assert(!all_dependencies.empty());
        if (all_dependencies.size() == 1) {
          message << " property "
                  << escape_string(*(all_dependencies.cbegin()));
        } else {
          message << " properties ";
          for (auto iterator = all_dependencies.cbegin();
               iterator != all_dependencies.cend(); ++iterator) {
            if (std::next(iterator) == all_dependencies.cend()) {
              message << "or " << escape_string(*iterator);
            } else {
              message << escape_string(*iterator) << ", ";
            }
          }
        }
      } else if (present.size() == 1) {
        message << "Because the object value defined the";
        message << " property " << escape_string(*(present.cbegin()));
        message
            << ", it was also expected to validate against the corresponding "
               "subschema";
      } else {
        message << "Because the object value defined the";
        message << " properties ";
        for (auto iterator = present.cbegin(); iterator != present.cend();
             ++iterator) {
          if (std::next(iterator) == present.cend()) {
            message << "and " << escape_string(*iterator);
          } else {
            message << escape_string(*iterator) << ", ";
          }
        }

        message
            << ", it was also expected to validate against the corresponding "
               "subschemas";
      }

      return message.str();
    }

    return unknown();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionPropertyDependencies) {
    assert(target.is_object());
    std::set<std::string> present;
    std::set<std::string> all_dependencies;
    std::set<std::string> required;

    for (const auto &entry : instruction_value<ValueStringMap>(step)) {
      all_dependencies.insert(entry.first);
      if (target.defines(entry.first)) {
        present.insert(entry.first);
        for (const auto &dependency : entry.second) {
          if (valid || !target.defines(dependency)) {
            required.insert(dependency);
          }
        }
      }
    }

    std::ostringstream message;

    if (present.empty()) {
      message << "The object value did not define the";
      assert(!all_dependencies.empty());
      if (all_dependencies.size() == 1) {
        message << " property " << escape_string(*(all_dependencies.cbegin()));
      } else {
        message << " properties ";
        for (auto iterator = all_dependencies.cbegin();
             iterator != all_dependencies.cend(); ++iterator) {
          if (std::next(iterator) == all_dependencies.cend()) {
            message << "or " << escape_string(*iterator);
          } else {
            message << escape_string(*iterator) << ", ";
          }
        }
      }

      return message.str();
    } else if (present.size() == 1) {
      message << "Because the object value defined the";
      message << " property " << escape_string(*(present.cbegin()));
    } else {
      message << "Because the object value defined the";
      message << " properties ";
      for (auto iterator = present.cbegin(); iterator != present.cend();
           ++iterator) {
        if (std::next(iterator) == present.cend()) {
          message << "and " << escape_string(*iterator);
        } else {
          message << escape_string(*iterator) << ", ";
        }
      }
    }

    assert(!required.empty());
    message << ", it was also expected to define the";
    if (required.size() == 1) {
      message << " property " << escape_string(*(required.cbegin()));
    } else {
      message << " properties ";
      for (auto iterator = required.cbegin(); iterator != required.cend();
           ++iterator) {
        if (std::next(iterator) == required.cend()) {
          message << "and " << escape_string(*iterator);
        } else {
          message << escape_string(*iterator) << ", ";
        }
      }
    }

    return message.str();
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LogicalWhenArraySizeGreater) {
    if (keyword == "additionalItems" || keyword == "items") {
      assert(target.is_array());
      std::ostringstream message;

      if (target.size() > instruction_value<ValueUnsignedInteger>(step)) {
        const auto rest{target.size() -
                        instruction_value<ValueUnsignedInteger>(step)};
        message << "The array value contains " << rest << " additional"
                << (rest == 1 ? " item" : " items")
                << " not described by related keywords";
      } else {
        message << "The array value does not contain additional items not "
                   "described by related keywords";
      }

      return message.str();
    }

    return unknown();
  }

  return unknown();
}

} // namespace sourcemeta::blaze
