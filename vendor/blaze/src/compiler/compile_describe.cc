#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::any_of
#include <cassert>   // assert
#include <sstream>   // std::ostringstream
#include <variant>   // std::visit

namespace {
using namespace sourcemeta::blaze;

template <typename T> auto step_value(const T &step) -> decltype(auto) {
  if constexpr (requires { step.value; }) {
    return step.value;
  } else {
    return step.id;
  }
}

auto to_string(const sourcemeta::jsontoolkit::JSON::Type type) -> std::string {
  // Otherwise the type "real" might not make a lot
  // of sense to JSON Schema users
  if (type == sourcemeta::jsontoolkit::JSON::Type::Real) {
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
                         const sourcemeta::jsontoolkit::JSON::Type current,
                         const sourcemeta::jsontoolkit::JSON::Type expected,
                         std::ostringstream &message) -> void {
  message << "The value was expected to be of type ";
  message << to_string(expected);
  if (!valid) {
    message << " but it was of type ";
    message << to_string(current);
  }
}

auto describe_types_check(
    const bool valid, const sourcemeta::jsontoolkit::JSON::Type current,
    const std::vector<sourcemeta::jsontoolkit::JSON::Type> &expected,
    std::ostringstream &message) -> void {
  assert(expected.size() > 1);
  auto copy = expected;

  const auto match_real{std::find(copy.cbegin(), copy.cend(),
                                  sourcemeta::jsontoolkit::JSON::Type::Real)};
  const auto match_integer{
      std::find(copy.cbegin(), copy.cend(),
                sourcemeta::jsontoolkit::JSON::Type::Integer)};
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

  if (valid && current == sourcemeta::jsontoolkit::JSON::Type::Integer &&
      std::find(copy.cbegin(), copy.cend(),
                sourcemeta::jsontoolkit::JSON::Type::Real) != copy.cend()) {
    message << "number";
  } else {
    message << to_string(current);
  }
}

auto describe_reference(const sourcemeta::jsontoolkit::JSON &target)
    -> std::string {
  std::ostringstream message;
  message << "The " << to_string(target.type())
          << " value was expected to validate against the statically "
             "referenced schema";
  return message.str();
}

auto is_within_keyword(
    const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
    const std::string &keyword) -> bool {
  return std::any_of(evaluate_path.cbegin(), evaluate_path.cend(),
                     [&keyword](const auto &token) {
                       return token.is_property() &&
                              token.to_property() == keyword;
                     });
}

auto unknown() -> std::string {
  // In theory we should never get here
  assert(false);
  return "<unknown>";
}

struct DescribeVisitor {
  const bool valid;
  const sourcemeta::jsontoolkit::WeakPointer &evaluate_path;
  const std::string &keyword;
  const sourcemeta::jsontoolkit::WeakPointer &instance_location;
  const sourcemeta::jsontoolkit::JSON &target;
  const sourcemeta::jsontoolkit::JSON &annotation;

  auto operator()(const AssertionFail &) const -> std::string {
    if (this->keyword == "contains") {
      return "The constraints declared for this keyword were not satisfiable";
    }

    if (this->keyword == "additionalProperties" ||
        this->keyword == "unevaluatedProperties") {
      std::ostringstream message;
      assert(!this->instance_location.empty());
      assert(this->instance_location.back().is_property());
      message << "The object value was not expected to define the property "
              << escape_string(this->instance_location.back().to_property());
      return message.str();
    }

    if (this->keyword == "unevaluatedItems") {
      std::ostringstream message;
      assert(!this->instance_location.empty());
      assert(this->instance_location.back().is_index());
      message << "The array value was not expected to define the item at index "
              << this->instance_location.back().to_index();
      return message.str();
    }

    return "No instance is expected to succeed against the false schema";
  }

  auto operator()(const LogicalOr &step) const -> std::string {
    assert(!step.children.empty());
    std::ostringstream message;
    message << "The " << to_string(this->target.type())
            << " value was expected to validate against ";
    if (step.children.size() > 1) {
      message << "at least one of the " << step.children.size()
              << " given subschemas";
    } else {
      message << "the given subschema";
    }

    return message.str();
  }

  auto operator()(const LogicalAnd &step) const -> std::string {
    if (this->keyword == "allOf") {
      assert(!step.children.empty());
      std::ostringstream message;
      message << "The " << to_string(this->target.type())
              << " value was expected to validate against the ";
      if (step.children.size() > 1) {
        message << step.children.size() << " given subschemas";
      } else {
        message << "given subschema";
      }

      return message.str();
    }

    if (this->keyword == "properties") {
      assert(!step.children.empty());
      if (!this->target.is_object()) {
        std::ostringstream message;
        describe_type_check(this->valid, this->target.type(),
                            sourcemeta::jsontoolkit::JSON::Type::Object,
                            message);
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

    if (this->keyword == "$ref") {
      return describe_reference(this->target);
    }

    return unknown();
  }

  auto operator()(const LogicalXor &step) const -> std::string {
    assert(!step.children.empty());
    std::ostringstream message;
    message << "The " << to_string(this->target.type())
            << " value was expected to validate against ";
    if (step.children.size() > 1) {
      message << "one and only one of the " << step.children.size()
              << " given subschemas";
    } else {
      message << "the given subschema";
    }

    return message.str();
  }

  auto operator()(const LogicalCondition &) const -> std::string {
    std::ostringstream message;
    message << "The " << to_string(this->target.type())
            << " value was expected to validate against the given conditional";
    return message.str();
  }

  auto operator()(const LogicalNot &) const -> std::string {
    std::ostringstream message;
    message
        << "The " << to_string(this->target.type())
        << " value was expected to not validate against the given subschema";
    if (!this->valid) {
      message << ", but it did";
    }

    return message.str();
  }

  auto operator()(const LogicalNotEvaluate &) const -> std::string {
    std::ostringstream message;
    message
        << "The " << to_string(this->target.type())
        << " value was expected to not validate against the given subschema";
    if (!this->valid) {
      message << ", but it did";
    }

    return message.str();
  }

  auto operator()(const ControlLabel &) const -> std::string {
    return describe_reference(this->target);
  }

  auto operator()(const ControlMark &) const -> std::string {
    return "The schema location was marked for future use";
  }

  auto operator()(const ControlEvaluate &) const -> std::string {
    return "The instance location was marked as evaluated";
  }

  auto operator()(const ControlJump &) const -> std::string {
    return describe_reference(this->target);
  }

  auto operator()(const ControlDynamicAnchorJump &step) const -> std::string {
    if (this->keyword == "$dynamicRef") {
      const auto &value{step_value(step)};
      std::ostringstream message;
      message << "The " << to_string(target.type())
              << " value was expected to validate against the first subschema "
                 "in scope that declared the dynamic anchor "
              << escape_string(value);
      return message.str();
    }

    assert(this->keyword == "$recursiveRef");
    std::ostringstream message;
    message << "The " << to_string(target.type())
            << " value was expected to validate against the first subschema "
               "in scope that declared a recursive anchor";
    return message.str();
  }

  auto operator()(const AnnotationEmit &) const -> std::string {
    if (this->keyword == "properties") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The object property "
              << escape_string(this->annotation.to_string())
              << " successfully validated against its property "
                 "subschema";
      return message.str();
    }

    if ((this->keyword == "items" || this->keyword == "additionalItems") &&
        this->annotation.is_boolean() && this->annotation.to_boolean()) {
      assert(this->target.is_array());
      return "Every item in the array value was successfully validated";
    }

    if ((this->keyword == "prefixItems" || this->keyword == "items") &&
        this->annotation.is_integer()) {
      assert(this->target.is_array());
      assert(this->annotation.is_positive());
      std::ostringstream message;
      if (this->annotation.to_integer() == 0) {
        message << "The first item of the array value successfully validated "
                   "against the first "
                   "positional subschema";
      } else {
        message << "The first " << this->annotation.to_integer() + 1
                << " items of the array value successfully validated against "
                   "the given "
                   "positional subschemas";
      }

      return message.str();
    }

    if (this->keyword == "prefixItems" && this->annotation.is_boolean() &&
        this->annotation.to_boolean()) {
      assert(this->target.is_array());
      std::ostringstream message;
      message << "Every item of the array value validated against the given "
                 "positional subschemas";
      return message.str();
    }

    if (this->keyword == "title" || this->keyword == "description") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The " << this->keyword << " of the";
      if (this->instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      message << " was " << escape_string(this->annotation.to_string());
      return message.str();
    }

    if (this->keyword == "default") {
      std::ostringstream message;
      message << "The default value of the";
      if (this->instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      message << " was ";
      stringify(this->annotation, message);
      return message.str();
    }

    if (this->keyword == "deprecated" && this->annotation.is_boolean()) {
      std::ostringstream message;
      if (this->instance_location.empty()) {
        message << "The instance";
      } else {
        message << "The instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      if (this->annotation.to_boolean()) {
        message << " was considered deprecated";
      } else {
        message << " was not considered deprecated";
      }

      return message.str();
    }

    if (this->keyword == "readOnly" && this->annotation.is_boolean()) {
      std::ostringstream message;
      if (this->instance_location.empty()) {
        message << "The instance";
      } else {
        message << "The instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      if (this->annotation.to_boolean()) {
        message << " was considered read-only";
      } else {
        message << " was not considered read-only";
      }

      return message.str();
    }

    if (this->keyword == "writeOnly" && this->annotation.is_boolean()) {
      std::ostringstream message;
      if (this->instance_location.empty()) {
        message << "The instance";
      } else {
        message << "The instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      if (this->annotation.to_boolean()) {
        message << " was considered write-only";
      } else {
        message << " was not considered write-only";
      }

      return message.str();
    }

    if (this->keyword == "examples") {
      assert(this->annotation.is_array());
      std::ostringstream message;
      if (this->instance_location.empty()) {
        message << "Examples of the instance";
      } else {
        message << "Examples of the instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      message << " were ";
      for (auto iterator = this->annotation.as_array().cbegin();
           iterator != this->annotation.as_array().cend(); ++iterator) {
        if (std::next(iterator) == this->annotation.as_array().cend()) {
          message << "and ";
          stringify(*iterator, message);
        } else {
          stringify(*iterator, message);
          message << ", ";
        }
      }

      return message.str();
    }

    if (this->keyword == "contentEncoding") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The content encoding of the";
      if (this->instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      message << " was " << escape_string(this->annotation.to_string());
      return message.str();
    }

    if (this->keyword == "contentMediaType") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The content media type of the";
      if (this->instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      message << " was " << escape_string(this->annotation.to_string());
      return message.str();
    }

    if (this->keyword == "contentSchema") {
      std::ostringstream message;
      message << "When decoded, the";
      if (this->instance_location.empty()) {
        message << " instance";
      } else {
        message << " instance location \"";
        stringify(this->instance_location, message);
        message << "\"";
      }

      message << " was expected to validate against the schema ";
      stringify(this->annotation, message);
      return message.str();
    }

    std::ostringstream message;
    message << "The unrecognized keyword " << escape_string(this->keyword)
            << " was collected as the annotation ";
    stringify(this->annotation, message);
    return message.str();
  }

  auto operator()(const AnnotationToParent &) const -> std::string {
    if (this->keyword == "unevaluatedItems" && this->annotation.is_boolean() &&
        this->annotation.to_boolean()) {
      assert(this->target.is_array());
      std::ostringstream message;
      message << "At least one item of the array value successfully validated "
                 "against the subschema for unevaluated items";
      return message.str();
    }

    return unknown();
  }

  auto operator()(const AnnotationBasenameToParent &) const -> std::string {
    if (this->keyword == "patternProperties") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The object property "
              << escape_string(this->annotation.to_string())
              << " successfully validated against its pattern property "
                 "subschema";
      return message.str();
    }

    if (this->keyword == "additionalProperties") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The object property "
              << escape_string(this->annotation.to_string())
              << " successfully validated against the additional properties "
                 "subschema";
      return message.str();
    }

    if (this->keyword == "unevaluatedProperties") {
      assert(this->annotation.is_string());
      std::ostringstream message;
      message << "The object property "
              << escape_string(this->annotation.to_string())
              << " successfully validated against the subschema for "
                 "unevaluated properties";
      return message.str();
    }

    if (this->keyword == "contains" && this->annotation.is_integer()) {
      assert(this->target.is_array());
      assert(this->annotation.is_positive());
      std::ostringstream message;
      message << "The item at index " << this->annotation.to_integer()
              << " of the array value successfully validated against the "
                 "containment check subschema";
      return message.str();
    }

    return unknown();
  }

  auto operator()(const LoopProperties &step) const -> std::string {
    assert(this->keyword == "additionalProperties");
    std::ostringstream message;
    if (step.children.size() == 1 &&
        std::holds_alternative<AssertionFail>(step.children.front())) {
      message << "The object value was not expected to define additional "
                 "properties";
    } else {
      message << "The object properties not covered by other adjacent object "
                 "keywords were expected to validate against this subschema";
    }

    return message.str();
  }

  auto operator()(const LoopPropertiesEvaluate &step) const -> std::string {
    assert(this->keyword == "additionalProperties");
    std::ostringstream message;
    if (step.children.size() == 1 &&
        std::holds_alternative<AssertionFail>(step.children.front())) {
      message << "The object value was not expected to define additional "
                 "properties";
    } else {
      message << "The object properties not covered by other adjacent object "
                 "keywords were expected to validate against this subschema";
    }

    return message.str();
  }

  auto operator()(const LoopPropertiesUnevaluated &step) const -> std::string {
    if (this->keyword == "unevaluatedProperties") {
      std::ostringstream message;
      if (!step.children.empty() &&
          std::holds_alternative<AssertionFail>(step.children.front())) {
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

  auto operator()(const LoopPropertiesUnevaluatedExcept &step) const
      -> std::string {
    if (this->keyword == "unevaluatedProperties") {
      std::ostringstream message;
      if (!step.children.empty() &&
          std::holds_alternative<AssertionFail>(step.children.front())) {
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

  auto operator()(const LoopPropertiesExcept &step) const -> std::string {
    assert(this->keyword == "additionalProperties");
    std::ostringstream message;
    if (step.children.size() == 1 &&
        std::holds_alternative<AssertionFail>(step.children.front())) {
      message << "The object value was not expected to define additional "
                 "properties";
    } else {
      message << "The object properties not covered by other adjacent object "
                 "keywords were expected to validate against this subschema";
    }

    return message.str();
  }

  auto operator()(const LoopPropertiesWhitelist &) const -> std::string {
    assert(this->keyword == "additionalProperties");
    return "The object value was not expected to define additional properties";
  }

  auto operator()(const LoopPropertiesType &step) const -> std::string {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(step.value);
    return message.str();
  }

  auto operator()(const LoopPropertiesTypeEvaluate &step) const -> std::string {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(step.value);
    return message.str();
  }

  auto operator()(const LoopPropertiesTypeStrict &step) const -> std::string {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(step.value);
    return message.str();
  }

  auto operator()(const LoopPropertiesTypeStrictEvaluate &step) const
      -> std::string {
    std::ostringstream message;
    message << "The object properties were expected to be of type "
            << to_string(step.value);
    return message.str();
  }

  auto operator()(const LoopPropertiesTypeStrictAny &step) const
      -> std::string {
    std::ostringstream message;
    message << "The object properties were expected to be of type ";
    const auto &types{step_value(step)};
    for (auto iterator = types.cbegin(); iterator != types.cend(); ++iterator) {
      if (std::next(iterator) == types.cend()) {
        message << "or " << to_string(*iterator);
      } else {
        message << to_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  auto operator()(const LoopPropertiesTypeStrictAnyEvaluate &step) const
      -> std::string {
    std::ostringstream message;
    message << "The object properties were expected to be of type ";
    const auto &types{step_value(step)};
    for (auto iterator = types.cbegin(); iterator != types.cend(); ++iterator) {
      if (std::next(iterator) == types.cend()) {
        message << "or " << to_string(*iterator);
      } else {
        message << to_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  auto operator()(const LoopKeys &) const -> std::string {
    assert(this->keyword == "propertyNames");
    assert(this->target.is_object());
    std::ostringstream message;

    if (this->target.size() == 0) {
      assert(this->valid);
      message << "The object is empty and no properties were expected to "
                 "validate against the given subschema";
    } else if (this->target.size() == 1) {
      message << "The object property ";
      message << escape_string(this->target.as_object().cbegin()->first);
      message << " was expected to validate against the given subschema";
    } else {
      message << "The object properties ";
      for (auto iterator = this->target.as_object().cbegin();
           iterator != this->target.as_object().cend(); ++iterator) {
        if (std::next(iterator) == this->target.as_object().cend()) {
          message << "and " << escape_string(iterator->first);
        } else {
          message << escape_string(iterator->first) << ", ";
        }
      }

      message << " were expected to validate against the given subschema";
    }

    return message.str();
  }

  auto operator()(const LoopItems &step) const -> std::string {
    assert(this->target.is_array());
    const auto &value{step_value(step)};
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

  auto operator()(const LoopItemsUnevaluated &) const -> std::string {
    assert(this->keyword == "unevaluatedItems");
    std::ostringstream message;
    message << "The array items not covered by other array keywords, if any, "
               "were expected to validate against this subschema";
    return message.str();
  }

  auto operator()(const LoopItemsType &step) const -> std::string {
    std::ostringstream message;
    message << "The array items were expected to be of type "
            << to_string(step.value);
    return message.str();
  }

  auto operator()(const LoopItemsTypeStrict &step) const -> std::string {
    std::ostringstream message;
    message << "The array items were expected to be of type "
            << to_string(step.value);
    return message.str();
  }

  auto operator()(const LoopItemsTypeStrictAny &step) const -> std::string {
    std::ostringstream message;
    message << "The array items were expected to be of type ";
    const auto &types{step_value(step)};
    for (auto iterator = types.cbegin(); iterator != types.cend(); ++iterator) {
      if (std::next(iterator) == types.cend()) {
        message << "or " << to_string(*iterator);
      } else {
        message << to_string(*iterator) << ", ";
      }
    }

    return message.str();
  }

  auto operator()(const LoopContains &step) const -> std::string {
    assert(this->target.is_array());
    std::ostringstream message;
    const auto &value{step_value(step)};
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

  auto operator()(const AssertionDefines &step) const -> std::string {
    std::ostringstream message;
    message << "The object value was expected to define the property "
            << escape_string(step_value(step));
    return message.str();
  }

  auto operator()(const AssertionDefinesAll &step) const -> std::string {
    const auto &value{step_value(step)};
    assert(value.size() > 1);
    std::ostringstream message;
    message << "The object value was expected to define properties ";
    for (auto iterator = value.cbegin(); iterator != value.cend(); ++iterator) {
      if (std::next(iterator) == value.cend()) {
        message << "and " << escape_string(*iterator);
      } else {
        message << escape_string(*iterator) << ", ";
      }
    }

    if (this->valid) {
      return message.str();
    }

    assert(this->target.is_object());
    std::set<std::string> missing;
    for (const auto &property : value) {
      if (!this->target.defines(property)) {
        missing.insert(property);
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

  auto operator()(const AssertionType &step) const -> std::string {
    std::ostringstream message;
    describe_type_check(this->valid, this->target.type(), step_value(step),
                        message);
    return message.str();
  }

  auto operator()(const AssertionTypeStrict &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    if (!this->valid && value == sourcemeta::jsontoolkit::JSON::Type::Real &&
        this->target.type() == sourcemeta::jsontoolkit::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!this->valid &&
               value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
               this->target.type() ==
                   sourcemeta::jsontoolkit::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(this->valid, this->target.type(), value, message);
    }

    return message.str();
  }

  auto operator()(const AssertionTypeAny &step) const -> std::string {
    std::ostringstream message;
    describe_types_check(this->valid, this->target.type(), step_value(step),
                         message);
    return message.str();
  }

  auto operator()(const AssertionTypeStrictAny &step) const -> std::string {
    std::ostringstream message;
    describe_types_check(this->valid, this->target.type(), step_value(step),
                         message);
    return message.str();
  }

  auto operator()(const AssertionTypeStringBounded &step) const -> std::string {
    std::ostringstream message;

    const auto minimum{std::get<0>(step.value)};
    const auto maximum{std::get<1>(step.value)};
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

  auto operator()(const AssertionTypeArrayBounded &step) const -> std::string {
    std::ostringstream message;

    const auto minimum{std::get<0>(step.value)};
    const auto maximum{std::get<1>(step.value)};
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

  auto operator()(const AssertionTypeObjectBounded &step) const -> std::string {
    std::ostringstream message;

    const auto minimum{std::get<0>(step.value)};
    const auto maximum{std::get<1>(step.value)};
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

  auto operator()(const AssertionRegex &step) const -> std::string {
    assert(this->target.is_string());
    std::ostringstream message;
    message << "The string value " << escape_string(this->target.to_string())
            << " was expected to match the regular expression "
            << escape_string(step_value(step).second);
    return message.str();
  }

  auto operator()(const AssertionStringSizeLess &step) const -> std::string {
    if (this->keyword == "maxLength") {
      std::ostringstream message;
      const auto maximum{step_value(step) - 1};

      if (is_within_keyword(this->evaluate_path, "propertyNames")) {
        assert(this->instance_location.back().is_property());
        message << "The object property name "
                << escape_string(this->instance_location.back().to_property());
      } else {
        message << "The string value ";
        stringify(this->target, message);
      }

      message << " was expected to consist of at most " << maximum
              << (maximum == 1 ? " character" : " characters");

      if (this->valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it consisted of ";

      if (is_within_keyword(this->evaluate_path, "propertyNames")) {
        message << this->instance_location.back().to_property().size();
        message << (this->instance_location.back().to_property().size() == 1
                        ? " character"
                        : " characters");
      } else {
        message << this->target.size();
        message << (this->target.size() == 1 ? " character" : " characters");
      }

      return message.str();
    }

    return unknown();
  }

  auto operator()(const AssertionStringSizeGreater &step) const -> std::string {
    if (this->keyword == "minLength") {
      std::ostringstream message;
      const auto minimum{step_value(step) + 1};

      if (is_within_keyword(this->evaluate_path, "propertyNames")) {
        assert(this->instance_location.back().is_property());
        message << "The object property name "
                << escape_string(this->instance_location.back().to_property());
      } else {
        message << "The string value ";
        stringify(this->target, message);
      }

      message << " was expected to consist of at least " << minimum
              << (minimum == 1 ? " character" : " characters");

      if (this->valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it consisted of ";

      if (is_within_keyword(this->evaluate_path, "propertyNames")) {
        message << this->instance_location.back().to_property().size();
        message << (this->instance_location.back().to_property().size() == 1
                        ? " character"
                        : " characters");
      } else {
        message << this->target.size();
        message << (this->target.size() == 1 ? " character" : " characters");
      }

      return message.str();
    }

    return unknown();
  }

  auto operator()(const AssertionArraySizeLess &step) const -> std::string {
    if (this->keyword == "maxItems") {
      assert(this->target.is_array());
      std::ostringstream message;
      const auto maximum{step_value(step) - 1};
      message << "The array value was expected to contain at most " << maximum;
      assert(maximum > 0);
      if (maximum == 1) {
        message << " item";
      } else {
        message << " items";
      }

      if (this->valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it contained " << this->target.size();
      if (this->target.size() == 1) {
        message << " item";
      } else {
        message << " items";
      }

      return message.str();
    }

    return unknown();
  }

  auto operator()(const AssertionArraySizeGreater &step) const -> std::string {
    assert(this->target.is_array());
    std::ostringstream message;
    const auto minimum{step_value(step) + 1};
    message << "The array value was expected to contain at least " << minimum;
    assert(minimum > 0);
    if (minimum == 1) {
      message << " item";
    } else {
      message << " items";
    }

    if (this->valid) {
      message << " and";
    } else {
      message << " but";
    }

    message << " it contained " << this->target.size();
    if (this->target.size() == 1) {
      message << " item";
    } else {
      message << " items";
    }

    return message.str();
  }

  auto operator()(const AssertionObjectSizeLess &step) const -> std::string {
    if (this->keyword == "maxProperties") {
      assert(this->target.is_object());
      std::ostringstream message;
      const auto maximum{step_value(step) - 1};
      message << "The object value was expected to contain at most " << maximum;
      assert(maximum > 0);
      if (maximum == 1) {
        message << " property";
      } else {
        message << " properties";
      }

      if (this->valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it contained " << this->target.size();
      if (this->target.size() == 1) {
        message << " property: ";
        message << escape_string(this->target.as_object().cbegin()->first);
      } else {
        message << " properties: ";

        std::vector<std::string> properties;
        for (const auto &entry : this->target.as_object()) {
          properties.push_back(entry.first);
        }
        std::sort(properties.begin(), properties.end());

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

  auto operator()(const AssertionObjectSizeGreater &step) const -> std::string {
    if (this->keyword == "minProperties") {
      assert(this->target.is_object());
      std::ostringstream message;
      const auto minimum{step_value(step) + 1};
      message << "The object value was expected to contain at least "
              << minimum;
      assert(minimum > 0);
      if (minimum == 1) {
        message << " property";
      } else {
        message << " properties";
      }

      if (this->valid) {
        message << " and";
      } else {
        message << " but";
      }

      message << " it contained " << this->target.size();
      if (this->target.size() == 1) {
        message << " property: ";
        message << escape_string(this->target.as_object().cbegin()->first);
      } else {
        message << " properties: ";
        std::vector<std::string> properties;
        for (const auto &entry : this->target.as_object()) {
          properties.push_back(entry.first);
        }
        std::sort(properties.begin(), properties.end());

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

  auto operator()(const AssertionEqual &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to equal the " << to_string(value.type())
            << " constant ";
    stringify(value, message);
    return message.str();
  }

  auto operator()(const AssertionGreaterEqual &step) const {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to be greater than or equal to the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  auto operator()(const AssertionLessEqual &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to be less than or equal to the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  auto operator()(const AssertionGreater &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to be greater than the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    if (!this->valid && value == this->target) {
      message << ", but they were equal";
    }

    return message.str();
  }

  auto operator()(const AssertionLess &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to be less than the " << to_string(value.type())
            << " ";
    stringify(value, message);
    if (!this->valid && value == this->target) {
      message << ", but they were equal";
    }

    return message.str();
  }

  auto operator()(const AssertionUnique &) const -> std::string {
    assert(this->target.is_array());
    auto array{this->target.as_array()};
    std::ostringstream message;
    if (this->valid) {
      message << "The array value was expected to not contain duplicate items";
    } else {
      std::set<sourcemeta::jsontoolkit::JSON> duplicates;
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

  auto operator()(const AssertionDivisible &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to be divisible by the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  auto operator()(const AssertionEqualsAny &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    assert(!value.empty());

    if (value.size() == 1) {
      message << " was expected to equal the "
              << to_string(value.cbegin()->type()) << " constant ";
      stringify(*(value.cbegin()), message);
    } else {
      if (this->valid) {
        message << " was expected to equal one of the " << value.size()
                << " declared values";
      } else {
        message << " was expected to equal one of the following values: ";
        for (auto iterator = value.cbegin(); iterator != value.cend();
             ++iterator) {
          if (std::next(iterator) == value.cend()) {
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

  auto operator()(const AssertionStringType &step) const -> std::string {
    assert(this->target.is_string());
    std::ostringstream message;
    message << "The string value " << escape_string(this->target.to_string())
            << " was expected to represent a valid";
    switch (step_value(step)) {
      case ValueStringType::URI:
        message << " URI";
        break;
      default:
        return unknown();
    }

    return message.str();
  }

  auto operator()(const AssertionPropertyType &step) const -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    if (!this->valid && value == sourcemeta::jsontoolkit::JSON::Type::Real &&
        this->target.type() == sourcemeta::jsontoolkit::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!this->valid &&
               value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
               this->target.type() ==
                   sourcemeta::jsontoolkit::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(this->valid, this->target.type(), value, message);
    }

    return message.str();
  }

  auto operator()(const AssertionPropertyTypeEvaluate &step) const
      -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    if (!this->valid && value == sourcemeta::jsontoolkit::JSON::Type::Real &&
        this->target.type() == sourcemeta::jsontoolkit::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!this->valid &&
               value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
               this->target.type() ==
                   sourcemeta::jsontoolkit::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(this->valid, this->target.type(), value, message);
    }

    return message.str();
  }

  auto operator()(const AssertionPropertyTypeStrict &step) const
      -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    if (!this->valid && value == sourcemeta::jsontoolkit::JSON::Type::Real &&
        this->target.type() == sourcemeta::jsontoolkit::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!this->valid &&
               value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
               this->target.type() ==
                   sourcemeta::jsontoolkit::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(this->valid, this->target.type(), value, message);
    }

    return message.str();
  }

  auto operator()(const AssertionPropertyTypeStrictEvaluate &step) const
      -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    if (!this->valid && value == sourcemeta::jsontoolkit::JSON::Type::Real &&
        this->target.type() == sourcemeta::jsontoolkit::JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!this->valid &&
               value == sourcemeta::jsontoolkit::JSON::Type::Integer &&
               this->target.type() ==
                   sourcemeta::jsontoolkit::JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(this->valid, this->target.type(), value, message);
    }

    return message.str();
  }

  auto operator()(const AssertionPropertyTypeStrictAny &step) const
      -> std::string {
    std::ostringstream message;
    describe_types_check(this->valid, this->target.type(), step_value(step),
                         message);
    return message.str();
  }

  auto operator()(const AssertionPropertyTypeStrictAnyEvaluate &step) const
      -> std::string {
    std::ostringstream message;
    describe_types_check(this->valid, this->target.type(), step_value(step),
                         message);
    return message.str();
  }

  auto operator()(const AssertionArrayPrefix &step) const -> std::string {
    assert(this->keyword == "items" || this->keyword == "prefixItems");
    assert(!step.children.empty());
    assert(this->target.is_array());

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

  auto operator()(const AssertionArrayPrefixEvaluate &step) const
      -> std::string {
    assert(this->keyword == "items" || this->keyword == "prefixItems");
    assert(!step.children.empty());
    assert(this->target.is_array());

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

  auto operator()(const LoopPropertiesMatch &step) const -> std::string {
    assert(!step.children.empty());
    assert(this->target.is_object());
    std::ostringstream message;
    message << "The object value was expected to validate against the ";
    if (step.children.size() == 1) {
      message << "single defined property subschema";
    } else {
      message << step.children.size() << " defined properties subschemas";
    }

    return message.str();
  }

  auto operator()(const LogicalWhenDefines &step) const -> std::string {
    std::ostringstream message;
    message << "The object value defined the property \"" << step_value(step)
            << "\"";
    return message.str();
  }

  auto operator()(const LoopPropertiesRegex &step) const -> std::string {
    assert(this->target.is_object());
    std::ostringstream message;
    message << "The object properties that match the regular expression \""
            << step_value(step).second
            << "\" were expected to validate against the defined pattern "
               "property subschema";
    return message.str();
  }

  auto operator()(const LoopPropertiesStartsWith &step) const -> std::string {
    assert(this->target.is_object());
    std::ostringstream message;
    message << "The object properties that start with the string \""
            << step_value(step)
            << "\" were expected to validate against the defined pattern "
               "property subschema";
    return message.str();
  }

  auto operator()(const LogicalWhenType &step) const -> std::string {
    if (this->keyword == "items") {
      std::ostringstream message;
      describe_type_check(this->valid, this->target.type(), step_value(step),
                          message);
      return message.str();
    }

    if (this->keyword == "dependencies") {
      assert(this->target.is_object());
      assert(!step.children.empty());

      std::set<std::string> present;
      std::set<std::string> present_with_schemas;
      std::set<std::string> present_with_properties;
      std::set<std::string> all_dependencies;
      std::set<std::string> required_properties;

      for (const auto &child : step.children) {
        // Schema
        if (std::holds_alternative<LogicalWhenDefines>(child)) {
          const auto &substep{std::get<LogicalWhenDefines>(child)};
          const auto &property{step_value(substep)};
          all_dependencies.insert(property);
          if (!this->target.defines(property)) {
            continue;
          }

          present.insert(property);
          present_with_schemas.insert(property);

          // Properties
        } else {
          assert(std::holds_alternative<AssertionPropertyDependencies>(child));
          const auto &substep{std::get<AssertionPropertyDependencies>(child)};

          for (const auto &[property, dependencies] : substep.value) {
            all_dependencies.insert(property);
            if (this->target.defines(property)) {
              present.insert(property);
              present_with_properties.insert(property);
              for (const auto &dependency : dependencies) {
                if (this->valid || !this->target.defines(dependency)) {
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

    if (this->keyword == "dependentSchemas") {
      assert(this->target.is_object());
      assert(!step.children.empty());
      std::set<std::string> present;
      std::set<std::string> all_dependencies;
      for (const auto &child : step.children) {
        assert(std::holds_alternative<LogicalWhenDefines>(child));
        const auto &substep{std::get<LogicalWhenDefines>(child)};
        const auto &property{step_value(substep)};
        all_dependencies.insert(property);
        if (!this->target.defines(property)) {
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

  auto operator()(const AssertionPropertyDependencies &step) const
      -> std::string {
    assert(this->target.is_object());
    std::set<std::string> present;
    std::set<std::string> all_dependencies;
    std::set<std::string> required;

    for (const auto &[property, dependencies] : step.value) {
      all_dependencies.insert(property);
      if (this->target.defines(property)) {
        present.insert(property);
        for (const auto &dependency : dependencies) {
          if (this->valid || !this->target.defines(dependency)) {
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

  auto operator()(const LogicalWhenArraySizeGreater &step) const
      -> std::string {
    if (this->keyword == "additionalItems" || this->keyword == "items") {
      assert(this->target.is_array());
      std::ostringstream message;

      if (this->target.size() > step_value(step)) {
        const auto rest{this->target.size() - step_value(step)};
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

  // These steps are never described, at least not right now

  auto operator()(const ControlGroup &) const -> std::string {
    return unknown();
  }

  auto operator()(const ControlGroupWhenDefines &) const -> std::string {
    return unknown();
  }
};

} // namespace

namespace sourcemeta::blaze {

// TODO: What will unlock even better error messages is being able to
// get the subschema being evaluated along with the keyword
auto describe(const bool valid, const Template::value_type &step,
              const sourcemeta::jsontoolkit::WeakPointer &evaluate_path,
              const sourcemeta::jsontoolkit::WeakPointer &instance_location,
              const sourcemeta::jsontoolkit::JSON &instance,
              const sourcemeta::jsontoolkit::JSON &annotation) -> std::string {
  assert(evaluate_path.empty() || evaluate_path.back().is_property());
  return std::visit<std::string>(
      DescribeVisitor{
          valid, evaluate_path,
          evaluate_path.empty() ? "" : evaluate_path.back().to_property(),
          instance_location, get(instance, instance_location), annotation},
      step);
}

} // namespace sourcemeta::blaze
