#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <algorithm> // std::any_of
#include <cassert>   // assert
#include <sstream>   // std::ostringstream
#include <variant>   // std::visit

namespace {
using namespace sourcemeta::jsontoolkit;

template <typename T>
auto step_value(const SchemaCompilerStepValue<T> &value) -> const T & {
  assert(std::holds_alternative<T>(value));
  return std::get<T>(value);
}

template <typename T> auto step_value(const T &step) -> decltype(auto) {
  return step_value(step.value);
}

auto to_string(const JSON::Type type) -> std::string {
  // Otherwise the type "real" might not make a lot
  // of sense to JSON Schema users
  if (type == JSON::Type::Real) {
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

auto describe_type_check(const bool valid, const JSON::Type current,
                         const JSON::Type expected,
                         std::ostringstream &message) -> void {
  message << "The value was expected to be of type ";
  message << to_string(expected);
  if (!valid) {
    message << " but it was of type ";
    message << to_string(current);
  }
}

auto describe_types_check(const bool valid, const JSON::Type current,
                          const std::set<JSON::Type> &expected,
                          std::ostringstream &message) -> void {
  assert(expected.size() > 1);
  auto copy = expected;
  if (copy.contains(JSON::Type::Real) && copy.contains(JSON::Type::Integer)) {
    copy.erase(JSON::Type::Integer);
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

  if (valid && current == JSON::Type::Integer &&
      copy.contains(JSON::Type::Real)) {
    message << "number";
  } else {
    message << to_string(current);
  }
}

auto describe_reference(const JSON &target) -> std::string {
  std::ostringstream message;
  message << "The " << to_string(target.type())
          << " value was expected to validate against the statically "
             "referenced schema";
  return message.str();
}

auto is_within_keyword(const Pointer &evaluate_path,
                       const std::string &keyword) -> bool {
  return std::any_of(evaluate_path.cbegin(), evaluate_path.cend(),
                     [&keyword](const auto &token) {
                       return token.is_property() &&
                              token.to_property() == keyword;
                     });
}

struct DescribeVisitor {
  const bool valid;
  const Pointer &evaluate_path;
  const std::string &keyword;
  const Pointer &instance_location;
  const JSON &target;
  const JSON &annotation;

  auto operator()(const SchemaCompilerLogicalOr &) const -> std::string {
    return "The target is expected to match at least one of the given "
           "assertions";
  }

  auto operator()(const SchemaCompilerLogicalAnd &step) const -> std::string {
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

    if (this->keyword == "then" || this->keyword == "else") {
      assert(!step.children.empty());
      std::ostringstream message;
      message << "Because of the conditional outcome, the "
              << to_string(this->target.type())
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

    return "The target is expected to match all of the given assertions";
  }

  auto operator()(const SchemaCompilerLogicalXor &) const -> std::string {
    return "The target is expected to match one and only one of the given "
           "assertions";
  }
  auto operator()(const SchemaCompilerLogicalTry &) const -> std::string {
    if (this->keyword == "if") {
      std::ostringstream message;
      message << "The " << to_string(this->target.type())
              << " value was tested against the conditional subschema";
      return message.str();
    }

    return "The target might match all of the given assertions";
  }
  auto operator()(const SchemaCompilerLogicalNot &) const -> std::string {
    return "The given schema is expected to not validate successfully";
  }
  auto
  operator()(const SchemaCompilerInternalContainer &) const -> std::string {
    return "Internal";
  }
  auto
  operator()(const SchemaCompilerInternalAnnotation &) const -> std::string {
    return "The target was annotated with the given value";
  }
  auto operator()(const SchemaCompilerInternalNoAdjacentAnnotation &) const
      -> std::string {
    return "The target was not annotated with the given value at the same "
           "schema location";
  }
  auto
  operator()(const SchemaCompilerInternalNoAnnotation &) const -> std::string {
    return "The target was not annotated with the given value";
  }
  auto
  operator()(const SchemaCompilerInternalDefinesAll &) const -> std::string {
    return "The target object is expected to define all of the given "
           "properties";
  }

  auto operator()(const SchemaCompilerControlLabel &) const -> std::string {
    return describe_reference(this->target);
  }

  auto operator()(const SchemaCompilerControlMark &) const -> std::string {
    return describe_reference(this->target);
  }

  auto operator()(const SchemaCompilerControlJump &) const -> std::string {
    return describe_reference(this->target);
  }

  auto operator()(const SchemaCompilerControlDynamicAnchorJump &) const
      -> std::string {
    return "Jump to a dynamic anchor";
  }

  auto operator()(const SchemaCompilerAnnotationPublic &) const -> std::string {
    if (this->keyword == "if") {
      assert(this->annotation == JSON{true});
      std::ostringstream message;
      message
          << "The " << to_string(this->target.type())
          << " value successfully validated against the conditional subschema";
      return message.str();
    }

    return "Emit an annotation";
  }

  auto operator()(const SchemaCompilerLoopProperties &) const -> std::string {
    return "Loop over the properties of the target object";
  }

  auto operator()(const SchemaCompilerLoopKeys &) const -> std::string {
    if (this->keyword == "propertyNames") {
      assert(this->target.is_object());
      std::ostringstream message;

      if (this->target.size() == 0) {
        assert(this->valid);
        message << "The object is empty and no properties are expected to "
                   "validate against the given subschema";
      } else if (this->target.size() == 1) {
        message << "The object property ";
        message << escape_string(this->target.as_object().cbegin()->first);
        message << " is expected to validate against the given subschema";
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

        message << " are expected to validate against the given subschema";
      }

      return message.str();
    }

    return "Loop over the property keys of the target object";
  }

  auto operator()(const SchemaCompilerLoopItems &) const -> std::string {
    return "Loop over the items of the target array";
  }
  auto operator()(const SchemaCompilerLoopItemsFromAnnotationIndex &) const
      -> std::string {
    return "Loop over the items of the target array potentially bound by an "
           "annotation result";
  }

  auto operator()(const SchemaCompilerLoopContains &step) const -> std::string {
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

  auto operator()(const SchemaCompilerAssertionFail &) const -> std::string {
    return "Abort evaluation on failure";
  }

  auto
  operator()(const SchemaCompilerAssertionDefines &step) const -> std::string {
    std::ostringstream message;
    message << "The object value was expected to define the property "
            << escape_string(step_value(step));
    return message.str();
  }

  auto operator()(const SchemaCompilerAssertionDefinesAll &step) const
      -> std::string {
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
        if (std::next(iterator) == value.cend()) {
          message << "and " << escape_string(*iterator);
        } else {
          message << escape_string(*iterator) << ", ";
        }
      }
    }

    return message.str();
  }

  auto
  operator()(const SchemaCompilerAssertionType &step) const -> std::string {
    std::ostringstream message;
    describe_type_check(this->valid, this->target.type(), step_value(step),
                        message);
    return message.str();
  }

  auto operator()(const SchemaCompilerAssertionTypeStrict &step) const
      -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    if (!this->valid && value == JSON::Type::Real &&
        this->target.type() == JSON::Type::Integer) {
      message
          << "The value was expected to be a real number but it was an integer";
    } else if (!this->valid && value == JSON::Type::Integer &&
               this->target.type() == JSON::Type::Real) {
      message
          << "The value was expected to be an integer but it was a real number";
    } else {
      describe_type_check(this->valid, this->target.type(), value, message);
    }

    return message.str();
  }

  auto
  operator()(const SchemaCompilerAssertionTypeAny &step) const -> std::string {
    std::ostringstream message;
    describe_types_check(this->valid, this->target.type(), step_value(step),
                         message);
    return message.str();
  }

  auto operator()(const SchemaCompilerAssertionTypeStrictAny &step) const
      -> std::string {
    std::ostringstream message;
    describe_types_check(this->valid, this->target.type(), step_value(step),
                         message);
    return message.str();
  }

  auto
  operator()(const SchemaCompilerAssertionRegex &step) const -> std::string {
    assert(this->target.is_string());
    std::ostringstream message;
    message << "The string value " << escape_string(this->target.to_string())
            << " was expected to match the regular expression "
            << escape_string(step_value(step).second);
    return message.str();
  }

  auto operator()(const SchemaCompilerAssertionSizeGreater &step) const
      -> std::string {
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

    return "The target size is expected to be greater than the given number";
  }

  auto
  operator()(const SchemaCompilerAssertionSizeLess &step) const -> std::string {
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

    return "The target size is expected to be less than the given number";
  }

  auto
  operator()(const SchemaCompilerAssertionSizeEqual &) const -> std::string {
    return "The target size is expected to be equal to the given number";
  }

  auto
  operator()(const SchemaCompilerAssertionEqual &step) const -> std::string {
    if (this->keyword == "const") {
      std::ostringstream message;
      const auto &value{step_value(step)};
      message << "The " << to_string(this->target.type()) << " value ";
      stringify(this->target, message);
      message << " was expected to equal the " << to_string(value.type())
              << " constant ";
      stringify(value, message);
      return message.str();
    }

    return "The target is expected to be equal to the given value";
  }

  auto operator()(const SchemaCompilerAssertionGreaterEqual &) const {
    return "The target number is expected to be greater than or equal to the "
           "given number";
  }
  auto
  operator()(const SchemaCompilerAssertionLessEqual &) const -> std::string {
    return "The target number is expected to be less than or equal to the "
           "given number";
  }

  auto
  operator()(const SchemaCompilerAssertionGreater &step) const -> std::string {
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

  auto
  operator()(const SchemaCompilerAssertionLess &step) const -> std::string {
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

  auto operator()(const SchemaCompilerAssertionUnique &) const -> std::string {
    return "The target array is expected to not contain duplicates";
  }

  auto operator()(const SchemaCompilerAssertionDivisible &step) const
      -> std::string {
    std::ostringstream message;
    const auto &value{step_value(step)};
    message << "The " << to_string(this->target.type()) << " value ";
    stringify(this->target, message);
    message << " was expected to be divisible by the "
            << to_string(value.type()) << " ";
    stringify(value, message);
    return message.str();
  }

  auto
  operator()(const SchemaCompilerAssertionStringType &) const -> std::string {
    return "The target string is expected to match the given logical type";
  }
  auto
  operator()(const SchemaCompilerAssertionEqualsAny &) const -> std::string {
    return "The target document is expected to be one of the given values";
  }
};

} // namespace

namespace sourcemeta::jsontoolkit {

auto describe(const bool valid, const SchemaCompilerTemplate::value_type &step,
              const Pointer &evaluate_path, const Pointer &instance_location,
              const JSON &instance, const JSON &annotation) -> std::string {
  assert(evaluate_path.back().is_property());
  return std::visit<std::string>(
      DescribeVisitor{valid, evaluate_path, evaluate_path.back().to_property(),
                      instance_location, get(instance, instance_location),
                      annotation},
      step);
}

} // namespace sourcemeta::jsontoolkit
