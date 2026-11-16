#include <sourcemeta/blaze/evaluator.h>

#include <algorithm> // std::ranges::any_of, std::ranges::copy, std::ranges::sort
#include <array>     // std::array
#include <cassert>   // assert
#include <cstddef>   // std::size_t
#include <format>    // std::format, std::make_format_args, std::vformat
#include <initializer_list> // std::initializer_list
#include <set>              // std::set
#include <sstream>          // std::ostringstream
#include <string>           // std::string, std::to_string
#include <string_view>      // std::string_view
#include <utility>          // std::to_underlying, std::unreachable
#include <variant>          // std::get
#include <vector>           // std::vector

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

// How many arguments a message template can draw on. Every template the
// compiler emits names its arguments by index, as the renderer below has to
// hand the same number of them to `std::vformat` whatever the sentence is
constexpr std::size_t MESSAGE_ARGUMENTS{6};

// Substitute the phrases a message draws on into the template the compiler
// resolved for it. A caller names only the ones its sentence reaches for, and
// the rest stay empty
auto render_message(const std::string_view format,
                    const std::initializer_list<std::string_view> arguments)
    -> std::string {
  static_assert(MESSAGE_ARGUMENTS == 6,
                "Every argument must be named for std::make_format_args");
  assert(arguments.size() <= MESSAGE_ARGUMENTS);
  std::array<std::string_view, MESSAGE_ARGUMENTS> slots{};
  std::ranges::copy(arguments, slots.begin());
  return std::vformat(format,
                      std::make_format_args(slots[0], slots[1], slots[2],
                                            slots[3], slots[4], slots[5]));
}

// Whether a template draws on any argument at all, which is how the evaluator
// tells the forms that state something of the instance from the ones that
// state nothing but the schema
auto takes_arguments(const std::string_view format) -> bool {
  for (std::size_t cursor = 0; cursor + 1 < format.size(); cursor++) {
    if (format[cursor] == '{' && format[cursor + 1] >= '0' &&
        format[cursor + 1] <= '9') {
      return true;
    }
  }

  return false;
}

// The message template the compiler resolved for this instruction, or empty
// when no sentence applies to it
auto message_format(const Template &schema_template, const Instruction &step)
    -> std::string_view {
  const auto &extra{schema_template.extra.at(step.extra_index)};
  if (extra.message == 0) {
    return {};
  }

  return schema_template.messages.at(extra.message - 1);
}

// As above, for the form the instruction renders when it has something of its
// own to report
auto message_detail_format(const Template &schema_template,
                           const Instruction &step) -> std::string_view {
  const auto &extra{schema_template.extra.at(step.extra_index)};
  if (extra.message_detail == 0) {
    return {};
  }

  return schema_template.messages.at(extra.message_detail - 1);
}

// The sentence an instruction renders when the compiler resolved every word
// of it
auto describe_literal(const Template &schema_template, const Instruction &step)
    -> std::string {
  const auto format{message_format(schema_template, step)};
  assert(!format.empty());
  return std::string{format};
}

template <typename T>
auto describe_stringify(const sourcemeta::core::JSON &value, T &stream)
    -> void {
  if (value.is_decimal()) {
    stream << value.to_decimal().to_string();
  } else {
    sourcemeta::core::stringify(value, stream);
  }
}

auto type_name(const sourcemeta::core::JSON::Type type) -> std::string_view {
  using Type = sourcemeta::core::JSON::Type;
  switch (type) {
    case Type::Null:
      return "null";
    case Type::Boolean:
      return "boolean";
    case Type::Integer:
      return "integer";
    case Type::Real:
    case Type::Decimal:
      return "number";
    case Type::String:
      return "string";
    case Type::Array:
      return "array";
    case Type::Object:
      return "object";
    default:
      std::unreachable();
  }
}

auto value_type_name(const sourcemeta::core::JSON &value) -> std::string_view {
  if (value.type() == sourcemeta::core::JSON::Type::Decimal) {
    return value.to_decimal().is_integer() ? "integer" : "number";
  }
  return type_name(value.type());
}

auto escape_string(const std::string_view input) -> std::string {
  std::size_t size{2};
  for (const auto character : input) {
    size += (character == '"') ? 2 : 1;
  }

  std::string result;
  result.resize_and_overwrite(
      size, [&](char *buffer, std::size_t) -> std::size_t {
        auto *cursor{buffer};
        *cursor++ = '"';
        for (const auto character : input) {
          if (character == '"') {
            *cursor++ = '\\';
          }
          *cursor++ = character;
        }
        *cursor++ = '"';
        return static_cast<std::size_t>(cursor - buffer);
      });

  return result;
}

// Render a list of names as "a, b, and c", which is how every message that
// names more than one of them ends. A conjunction of "or" spells the same
// shape for a list of alternatives
template <typename Names>
auto describe_names(const Names &names, const std::string_view conjunction,
                    std::ostringstream &message) -> void {
  if (names.size() == 1) {
    message << escape_string(*(names.cbegin()));
    return;
  }

  for (auto iterator = names.cbegin(); iterator != names.cend(); ++iterator) {
    if (std::next(iterator) == names.cend()) {
      message << conjunction << " " << escape_string(*iterator);
    } else {
      message << escape_string(*iterator) << ", ";
    }
  }
}

// As above, for a list of JSON values rather than property names
template <typename Values>
auto describe_values(const Values &values, const std::string_view conjunction,
                     std::ostringstream &message) -> void {
  if (values.size() == 1) {
    describe_stringify(*(values.cbegin()), message);
    return;
  }

  for (auto iterator = values.cbegin(); iterator != values.cend(); ++iterator) {
    if (std::next(iterator) == values.cend()) {
      message << conjunction << " ";
      describe_stringify(*iterator, message);
    } else {
      describe_stringify(*iterator, message);
      message << ", ";
    }
  }
}

auto describe_type_check(const std::string_view format,
                         const std::string_view detail,
                         const std::string_view subject, const bool valid,
                         const sourcemeta::core::JSON::Type current,
                         const sourcemeta::core::JSON::Type expected)
    -> std::string {
  if (valid) {
    return render_message(format, {subject, type_name(expected)});
  }

  return render_message(detail,
                        {subject, type_name(expected),
                         current == sourcemeta::core::JSON::Type::Decimal
                             ? std::string_view{"number"}
                             : type_name(current)});
}

auto describe_not_type_check(const std::string_view format,
                             const std::string_view detail,
                             const std::string_view subject, const bool valid,
                             const sourcemeta::core::JSON::Type current,
                             const sourcemeta::core::JSON::Type expected)
    -> std::string {
  if (valid) {
    return render_message(format, {subject, type_name(expected)});
  }

  std::string_view actual{type_name(current)};
  if (current == sourcemeta::core::JSON::Type::Decimal &&
      expected == sourcemeta::core::JSON::Type::Integer) {
    actual = "integer";
  } else if ((current == sourcemeta::core::JSON::Type::Integer &&
              expected == sourcemeta::core::JSON::Type::Real) ||
             current == sourcemeta::core::JSON::Type::Decimal) {
    actual = "number";
  }

  return render_message(detail, {subject, type_name(expected), actual});
}

// A set of types with the numeric ones that read alike folded together, next
// to which of them the set named before the fold. The compiler folds the same
// way to decide whether a sentence names one type or several, and the two have
// to agree
struct CollapsedTypes {
  ValueTypes types;
  bool has_real;
  bool has_integer;
};

auto collapse_types(const ValueTypes expected) -> CollapsedTypes {
  ValueTypes types{expected};
  const auto has_real{
      types.test(std::to_underlying(sourcemeta::core::JSON::Type::Real))};
  const auto has_integer{
      types.test(std::to_underlying(sourcemeta::core::JSON::Type::Integer))};
  const auto has_decimal{
      types.test(std::to_underlying(sourcemeta::core::JSON::Type::Decimal))};

  if (has_real && has_integer) {
    types.reset(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
  }
  if (has_real && has_decimal) {
    types.reset(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
  }
  if (has_integer && has_decimal) {
    types.reset(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
  }

  return {.types = types, .has_real = has_real, .has_integer = has_integer};
}

auto first_type(const ValueTypes types) -> sourcemeta::core::JSON::Type {
  for (std::uint8_t bit{0}; bit < 8; bit++) {
    if (types.test(bit)) {
      return static_cast<sourcemeta::core::JSON::Type>(bit);
    }
  }

  std::unreachable();
}

// The type names a set holds, as a list of alternatives
auto describe_type_list(const ValueTypes types, std::ostringstream &message)
    -> void {
  std::uint8_t last_bit{255};
  for (std::uint8_t bit{0}; bit < 8; bit++) {
    if (types.test(bit)) {
      last_bit = bit;
    }
  }

  bool first{true};
  for (std::uint8_t bit{0}; bit < 8; bit++) {
    if (!types.test(bit)) {
      continue;
    }

    if (!first) {
      message << ", ";
    }
    if (bit == last_bit) {
      message << "or ";
    }

    message << type_name(static_cast<sourcemeta::core::JSON::Type>(bit));
    first = false;
  }
}

// The types an instruction names, as a single type or as a list of
// alternatives
auto describe_types(const ValueTypes types) -> std::string {
  const auto expectation{collapse_types(types)};
  if (expectation.types.count() == 1) {
    return std::string{type_name(first_type(expectation.types))};
  }

  std::ostringstream named;
  describe_type_list(expectation.types, named);
  return named.str();
}

// Which numeric type to report for the instance, given which numeric types the
// message named as alternatives. A set that narrowed to one of them reports
// that one, as the broader name would read as a contradiction
auto describe_actual_type(const bool narrowed,
                          const sourcemeta::core::JSON::Type current,
                          const bool has_real, const bool has_integer)
    -> std::string_view {
  if (narrowed && current == sourcemeta::core::JSON::Type::Decimal &&
      has_integer && !has_real) {
    return "integer";
  }

  if ((narrowed && current == sourcemeta::core::JSON::Type::Integer &&
       has_real) ||
      current == sourcemeta::core::JSON::Type::Decimal) {
    return "number";
  }

  return type_name(current);
}

auto describe_types_check(const Template &schema_template,
                          const Instruction &step,
                          const std::string_view subject, const bool valid,
                          const sourcemeta::core::JSON::Type current,
                          const ValueTypes expected) -> std::string {
  const auto expectation{collapse_types(expected)};
  if (expectation.types.count() == 1) {
    return describe_type_check(message_format(schema_template, step),
                               message_detail_format(schema_template, step),
                               subject, valid, current,
                               first_type(expectation.types));
  }

  std::ostringstream named;
  describe_type_list(expectation.types, named);
  return render_message(
      valid ? message_format(schema_template, step)
            : message_detail_format(schema_template, step),
      {subject, named.str(),
       describe_actual_type(valid, current, expectation.has_real,
                            expectation.has_integer)});
}

auto describe_not_types_check(const Template &schema_template,
                              const Instruction &step,
                              const std::string_view subject, const bool valid,
                              const sourcemeta::core::JSON::Type current,
                              const ValueTypes expected) -> std::string {
  const auto expectation{collapse_types(expected)};
  if (expectation.types.count() == 1) {
    return describe_not_type_check(message_format(schema_template, step),
                                   message_detail_format(schema_template, step),
                                   subject, valid, current,
                                   first_type(expectation.types));
  }

  std::ostringstream named;
  describe_type_list(expectation.types, named);
  return render_message(
      valid ? message_format(schema_template, step)
            : message_detail_format(schema_template, step),
      {subject, named.str(),
       describe_actual_type(!valid, current, expectation.has_real,
                            expectation.has_integer)});
}

// The property names a set carries, in the order it holds them
auto property_names(const ValueStringSet &value) -> std::vector<ValueString> {
  std::vector<ValueString> result;
  result.reserve(value.size());
  for (const auto &entry : value) {
    result.push_back(entry.first);
  }

  return result;
}

// The properties the object defines beyond the ones the set names
auto unexpected_properties(const sourcemeta::core::JSON &target,
                           const ValueStringSet &value)
    -> std::set<std::string_view> {
  std::set<std::string_view> result;
  for (const auto &entry : target.as_object()) {
    if (!value.contains(entry.first, entry.hash)) {
      result.insert(entry.first);
    }
  }

  return result;
}

auto describe_comparison(const std::string_view format,
                         const sourcemeta::core::JSON &target,
                         const sourcemeta::core::JSON &value) -> std::string {
  assert(!format.empty());
  std::ostringstream target_value;
  describe_stringify(target, target_value);
  std::ostringstream bound;
  describe_stringify(value, bound);
  return render_message(format, {value_type_name(target), target_value.str(),
                                 value_type_name(value), bound.str()});
}

auto is_within_keyword(const sourcemeta::core::WeakPointer &evaluate_path,
                       const std::string &keyword) -> bool {
  return std::ranges::any_of(
      evaluate_path, [&keyword](const auto &token) -> bool {
        return token.is_property() && token.to_property() == keyword;
      });
}

auto unknown() -> std::string {
  assert(false);
  return "<unknown>";
}

// Inside `propertyNames` the subject of a message is the property name that
// the evaluator carries out of band rather than the instance the location
// points at
auto names_a_property(const sourcemeta::core::WeakPointer &evaluate_path,
                      const sourcemeta::core::WeakPointer &instance_location)
    -> bool {
  return is_within_keyword(evaluate_path, "propertyNames") &&
         !instance_location.empty() && instance_location.back().is_property();
}

// Whether the subject names the instance value itself or only its type
enum class SubjectDetail : std::uint8_t { Value, Type };

// The scope an annotation applies to, as the instance itself or the location
// within it
auto describe_scope(const sourcemeta::core::WeakPointer &instance_location)
    -> std::string {
  if (instance_location.empty()) {
    return "instance";
  }

  std::ostringstream scope;
  scope << "instance location \"";
  stringify(instance_location, scope);
  scope << "\"";
  return scope.str();
}

// The noun phrase a message opens with
auto describe_subject(const sourcemeta::core::WeakPointer &evaluate_path,
                      const sourcemeta::core::WeakPointer &instance_location,
                      const sourcemeta::core::JSON &target,
                      const SubjectDetail detail) -> std::string {
  if (names_a_property(evaluate_path, instance_location)) {
    return std::format("The property name {}",
                       escape_string(instance_location.back().to_property()));
  }

  std::ostringstream subject;
  subject << "The " << type_name(target.type()) << " value";
  if (detail == SubjectDetail::Value) {
    subject << " ";
    describe_stringify(target, subject);
  }

  return subject.str();
}

// The subject of a message about a string, which inside `propertyNames` is the
// property name rather than the instance
auto describe_string_subject(
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &target) -> std::string {
  if (names_a_property(evaluate_path, instance_location)) {
    return std::format("The property name {}",
                       escape_string(instance_location.back().to_property()));
  }

  assert(target.is_string());
  return std::format("The string value {}", escape_string(target.to_string()));
}

auto describe_string_size(
    const Template &schema_template, const Instruction &step, const bool valid,
    const sourcemeta::core::WeakPointer &evaluate_path,
    const sourcemeta::core::WeakPointer &instance_location,
    const sourcemeta::core::JSON &target) -> std::string {
  const auto size{names_a_property(evaluate_path, instance_location)
                      ? instance_location.back().to_property().size()
                      : target.size()};
  return render_message(valid ? message_format(schema_template, step)
                              : message_detail_format(schema_template, step),
                        {describe_subject(evaluate_path, instance_location,
                                          target, SubjectDetail::Value),
                         std::to_string(size)});
}

auto describe_array_size(const Template &schema_template,
                         const Instruction &step, const bool valid,
                         const sourcemeta::core::JSON &target) -> std::string {
  assert(target.is_array());
  return render_message(valid ? message_format(schema_template, step)
                              : message_detail_format(schema_template, step),
                        {std::to_string(target.size())});
}

// As above, except that the message also names the properties the instance
// turned out to carry, which an empty object has none of
auto describe_object_size(const Template &schema_template,
                          const Instruction &step, const bool valid,
                          const sourcemeta::core::JSON &target) -> std::string {
  assert(target.is_object());
  std::ostringstream carried;

  if (!target.empty()) {
    std::vector<std::string> properties;
    properties.reserve(target.size());
    for (const auto &entry : target.as_object()) {
      properties.push_back(entry.first);
    }

    std::ranges::sort(properties);
    carried << ": ";
    describe_names(properties, "and", carried);
  }

  return render_message(valid ? message_format(schema_template, step)
                              : message_detail_format(schema_template, step),
                        {std::to_string(target.size()), carried.str()});
}

} // namespace

namespace sourcemeta::blaze {

// TODO: What will unlock even better error messages is being able to
// get the subschema being evaluated along with the keyword
// NOLINTNEXTLINE(google-readability-function-size,hicpp-function-size,readability-function-size)
auto describe(const Template &schema_template, const bool valid,
              const Instruction &step,
              const sourcemeta::core::WeakPointer &evaluate_path,
              const sourcemeta::core::WeakPointer &instance_location,
              const sourcemeta::core::JSON &instance,
              const sourcemeta::core::JSON &annotation) -> std::string {
  const std::string keyword{evaluate_path.empty() ||
                                    // The last token can be an index for
                                    // boolean schemas inside array applicators
                                    !evaluate_path.back().is_property()
                                ? ""
                                : evaluate_path.back().to_property()};
  const sourcemeta::core::JSON &target{get(instance, instance_location)};

  // The compiler resolved every word of these, as what they say does not
  // depend on how the instance turned out
  if (step.type == sourcemeta::blaze::InstructionIndex::Evaluate ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopProperties ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesEvaluate ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesUnevaluated ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesUnevaluatedExcept ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesExcept ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionObjectPropertiesSimple ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopItems ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopItemsFrom ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopItemsUnevaluated ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopItemsIntegerBounded ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopItemsIntegerBoundedSized ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopItemsArrayBoundedTypeStrictAny ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopContains ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeIntegerBounded ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionTypeIntegerBoundedStrict ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeIntegerLowerBound ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionTypeIntegerLowerBoundStrict ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStringBounded ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStringUpper ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeArrayBounded ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeArrayUpper ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeObjectBounded ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeObjectUpper) {
    return describe_literal(schema_template, step);
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionFail) {
    const auto detail{message_detail_format(schema_template, step)};
    // A `false` subschema declares no keyword of its own, so its evaluation
    // path ends at whatever declared it, which under `properties` and its
    // relatives is a name the author chose. A name that happens to read like
    // one of the keywords that name what the instance carried must not be
    // taken for the keyword itself, so the instance location has to agree
    if (!detail.empty() && !instance_location.empty()) {
      const auto &token{instance_location.back()};
      if (keyword == "unevaluatedItems") {
        if (token.is_index()) {
          return render_message(detail, {std::to_string(token.to_index())});
        }
      } else if (token.is_property()) {
        return render_message(detail, {escape_string(token.to_property())});
      }
    }

    return render_message(message_format(schema_template, step),
                          {type_name(target.type())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalOr) {
    assert(!step.children.empty());
    return render_message(message_format(schema_template, step),
                          {type_name(target.type())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalAnd) {
    const auto format{message_format(schema_template, step)};
    if (format.empty()) {
      return unknown();
    }

    return render_message(format, {type_name(target.type())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalXor) {
    assert(!step.children.empty());
    return render_message(message_format(schema_template, step),
                          {describe_subject(evaluate_path, instance_location,
                                            target, SubjectDetail::Type)});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalCondition ||
      step.type == sourcemeta::blaze::InstructionIndex::LogicalNot ||
      step.type == sourcemeta::blaze::InstructionIndex::LogicalNotEvaluate) {
    return render_message(message_format(schema_template, step),
                          {type_name(target.type())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::ControlJump) {
    // Inside `propertyNames` the subject is the property name, which is a
    // string rather than the instance the location points at
    return render_message(message_format(schema_template, step),
                          {names_a_property(evaluate_path, instance_location)
                               ? std::string_view{"string"}
                               : type_name(target.type())});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::ControlDynamicAnchorJump) {
    if (keyword == "$dynamicRef") {
      return render_message(
          message_format(schema_template, step),
          {type_name(target.type()),
           escape_string(instruction_value<ValueString>(step))});
    }

    assert(keyword == "$recursiveRef");
    return render_message(message_format(schema_template, step),
                          {type_name(target.type())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AnnotationEmit ||
      step.type == sourcemeta::blaze::InstructionIndex::AnnotationEmitWrapped ||
      step.type == sourcemeta::blaze::InstructionIndex::AnnotationToParent) {
    const auto format{message_format(schema_template, step)};
    if (format.empty()) {
      return unknown();
    }

    // Every annotation message draws on the same arguments, as the compiler
    // picked the sentence from the keyword and the value but cannot phrase
    // what fills it
    std::ostringstream spelled;
    describe_stringify(annotation, spelled);
    std::ostringstream serialized;
    stringify(annotation, serialized);
    std::ostringstream listed;
    if (annotation.is_array()) {
      describe_values(annotation.as_array(), "and", listed);
    }

    return render_message(format, {describe_scope(instance_location),
                                   annotation.is_string()
                                       ? escape_string(annotation.to_string())
                                       : std::string{},
                                   spelled.str(), listed.str(),
                                   escape_string(keyword), serialized.str()});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AnnotationBasenameToParent) {
    const auto format{message_format(schema_template, step)};
    if (format.empty()) {
      return unknown();
    }

    std::ostringstream named;
    if (annotation.is_string()) {
      named << escape_string(annotation.to_string());
    } else {
      assert(annotation.is_integer());
      assert(annotation.is_positive());
      describe_stringify(annotation, named);
    }

    return render_message(format, {named.str()});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesExactlyTypeStrict) {
    return render_message(
        message_format(schema_template, step),
        {type_name(instruction_value<ValueTypedProperties>(step).first)});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesExactlyTypeStrictHash ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopItemsPropertiesExactlyTypeStrictHash ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopItemsPropertiesExactlyTypeStrictHash3) {
    return render_message(
        message_format(schema_template, step),
        {type_name(instruction_value<ValueTypedHashes>(step).first)});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionNumberIntegerBounded) {
    std::ostringstream actual;
    describe_stringify(target, actual);
    return render_message(message_format(schema_template, step),
                          {value_type_name(target), actual.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesType ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeEvaluate ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrict ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesTypeStrictEvaluate ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopItemsType ||
      step.type == sourcemeta::blaze::InstructionIndex::LoopItemsTypeStrict) {
    return render_message(message_format(schema_template, step),
                          {type_name(instruction_value<ValueType>(step))});
  }

  if (step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesTypeStrictAny ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       LoopPropertiesTypeStrictAnyEvaluate ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopItemsTypeStrictAny) {
    return render_message(
        message_format(schema_template, step),
        {describe_types(instruction_value<ValueTypes>(step))});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopKeys) {
    assert(keyword == "propertyNames");
    assert(target.is_object());
    if (target.empty()) {
      assert(valid);
      return describe_literal(schema_template, step);
    }

    std::vector<std::string> names;
    names.reserve(target.size());
    for (const auto &entry : target.as_object()) {
      names.push_back(entry.first);
    }

    std::ostringstream named;
    describe_names(names, "and", named);
    return render_message(message_detail_format(schema_template, step),
                          {named.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionDefines) {
    return render_message(
        message_format(schema_template, step),
        {escape_string(instruction_value<ValueProperty>(step).first)});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesStrict) {
    return render_message(
        message_format(schema_template, step),
        {escape_string(instruction_value<ValueProperty>(step).first)});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionDefinesAll) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);
    std::ostringstream listed;
    describe_names(property_names(value), "and", listed);

    if (valid) {
      return render_message(message_format(schema_template, step),
                            {listed.str()});
    }

    assert(target.is_object());
    std::set<std::string> missing;
    for (const auto &property : value) {
      if (!target.defines(property.first, property.second)) {
        missing.insert(property.first);
      }
    }

    assert(!missing.empty());
    std::ostringstream named;
    describe_names(missing, "and", named);
    return render_message(message_detail_format(schema_template, step),
                          {listed.str(), named.str()});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesAllStrict) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);
    std::ostringstream properties;
    describe_names(property_names(value), "and", properties);
    return render_message(message_format(schema_template, step),
                          {properties.str()});
  }

  if (step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionDefinesExactly ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionDefinesExactlyStrict) {
    const auto &value{instruction_value<ValueStringSet>(step)};
    assert(value.size() > 1);
    auto value_vector{property_names(value)};

    std::ranges::sort(value_vector);
    std::ostringstream listed;
    describe_names(value_vector, "and", listed);

    if (valid || !target.is_object()) {
      return render_message(message_format(schema_template, step),
                            {listed.str()});
    }

    const auto extras{unexpected_properties(target, value)};
    if (extras.empty()) {
      return render_message(message_format(schema_template, step),
                            {listed.str()});
    }

    std::ostringstream named;
    describe_names(extras, "and", named);
    return render_message(message_detail_format(schema_template, step),
                          {listed.str(), named.str()});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionDefinesExactlyStrictHash3) {
    const auto &value{instruction_value<ValueStringHashes>(step).first};
    if (valid || !target.is_object()) {
      return describe_literal(schema_template, step);
    }

    std::set<std::string_view> extras;
    for (const auto &entry : target.as_object()) {
      bool allowed{false};
      for (const auto &candidate : value) {
        if (candidate.first == entry.hash && candidate.second == entry.first) {
          allowed = true;
          break;
        }
      }

      if (!allowed) {
        extras.insert(entry.first);
      }
    }

    if (extras.empty()) {
      return describe_literal(schema_template, step);
    }

    std::ostringstream named;
    describe_names(extras, "and", named);
    return render_message(message_detail_format(schema_template, step),
                          {named.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionType ||
      step.type == sourcemeta::blaze::InstructionIndex::AssertionTypeStrict) {
    // Inside `propertyNames` the subject is the property name, which is a
    // string however the assertion turns out, so there is nothing to report
    // beyond the type the keyword asked for
    const auto property{names_a_property(evaluate_path, instance_location)};
    return describe_type_check(
        message_format(schema_template, step),
        message_detail_format(schema_template, step),
        property
            ? std::format("The property name {}",
                          escape_string(instance_location.back().to_property()))
            : std::string{"The value"},
        valid || property, target.type(), instruction_value<ValueType>(step));
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionTypeAny ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeStrictAny ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionPropertyTypeStrictAnyEvaluate) {
    return describe_types_check(schema_template, step, "The value", valid,
                                target.type(),
                                instruction_value<ValueTypes>(step));
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionNotTypeStrictAny) {
    const auto format{message_format(schema_template, step)};
    // The keywords whose bound no instance can satisfy compile into a type
    // check that explains the keyword instead, in one sentence of its own
    if (!takes_arguments(format)) {
      return std::string{format};
    }

    return describe_not_types_check(schema_template, step, "The value", valid,
                                    target.type(),
                                    instruction_value<ValueTypes>(step));
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionRegex) {
    return render_message(
        message_format(schema_template, step),
        {describe_string_subject(evaluate_path, instance_location, target),
         escape_string(instruction_value<ValueRegex>(step).second)});
  }

  if (step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionStringSizeLess ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionStringSizeGreater) {
    return describe_string_size(schema_template, step, valid, evaluate_path,
                                instance_location, target);
  }

  if (step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionArraySizeLess ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionArraySizeGreater) {
    return describe_array_size(schema_template, step, valid, target);
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeLess) {
    const auto format{message_format(schema_template, step)};
    // A closed object says so without counting, as the bound it compiled from
    // is the number of properties the schema named
    if (!takes_arguments(format)) {
      return std::string{format};
    }

    return describe_object_size(schema_template, step, valid, target);
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionObjectSizeGreater) {
    return describe_object_size(schema_template, step, valid, target);
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::AssertionPropertyEqual) {
    const auto &value{instruction_value<ValueJSON>(step)};
    std::ostringstream actual;
    describe_stringify(target, actual);
    std::ostringstream constant;
    describe_stringify(value, constant);
    return render_message(message_format(schema_template, step),
                          {type_name(target.type()), actual.str(),
                           type_name(value.type()), constant.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionEqual) {
    const auto &value{instruction_value<ValueJSON>(step)};
    std::ostringstream constant;
    describe_stringify(value, constant);
    return render_message(message_format(schema_template, step),
                          {describe_subject(evaluate_path, instance_location,
                                            target, SubjectDetail::Value),
                           type_name(value.type()), constant.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionGreaterEqual ||
      step.type == sourcemeta::blaze::InstructionIndex::AssertionLessEqual ||
      step.type == sourcemeta::blaze::InstructionIndex::AssertionGreater ||
      step.type == sourcemeta::blaze::InstructionIndex::AssertionLess ||
      step.type == sourcemeta::blaze::InstructionIndex::AssertionDivisible) {
    return describe_comparison(message_format(schema_template, step), target,
                               instruction_value<ValueJSON>(step));
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionUnique) {
    assert(target.is_array());
    if (valid) {
      return describe_literal(schema_template, step);
    }

    const auto &array{target.as_array()};
    std::set<sourcemeta::core::JSON> duplicates;
    for (auto iterator = array.cbegin(); iterator != array.cend(); ++iterator) {
      for (auto subiterator = std::next(iterator); subiterator != array.cend();
           ++subiterator) {
        if (*iterator == *subiterator) {
          duplicates.insert(*iterator);
        }
      }
    }

    assert(!duplicates.empty());
    std::ostringstream listed;
    describe_values(duplicates, "and", listed);
    return render_message(message_detail_format(schema_template, step),
                          {listed.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionEqualsAny) {
    const auto &value{instruction_value<ValueSet>(step)};
    assert(!value.empty());
    const auto subject{describe_subject(evaluate_path, instance_location,
                                        target, SubjectDetail::Value)};

    if (value.size() == 1) {
      std::ostringstream constant;
      describe_stringify(*(value.cbegin()), constant);
      return render_message(
          message_format(schema_template, step),
          {subject, type_name(value.cbegin()->type()), constant.str()});
    }

    if (valid) {
      return render_message(message_format(schema_template, step), {subject});
    }

    std::vector<sourcemeta::core::JSON> copy{value.cbegin(), value.cend()};
    std::ranges::sort(copy);
    std::ostringstream listed;
    describe_values(copy, "and", listed);
    return render_message(message_detail_format(schema_template, step),
                          {subject, listed.str()});
  }

  if (step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionEqualsAnyStringHash ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionPropertyEqualsAnyStringHash) {
    assert(!instruction_value<ValueStringHashes>(step).first.empty());
    return render_message(message_format(schema_template, step),
                          {describe_subject(evaluate_path, instance_location,
                                            target, SubjectDetail::Value)});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionNumberType) {
    std::ostringstream actual;
    describe_stringify(target, actual);
    return render_message(message_format(schema_template, step),
                          {value_type_name(target), actual.str()});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionStringType) {
    assert(target.is_string());
    const auto format{message_format(schema_template, step)};
    if (format.empty()) {
      return unknown();
    }

    return render_message(format, {escape_string(target.to_string())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionPropertyType ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeEvaluate ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionPropertyTypeStrict ||
      step.type == sourcemeta::blaze::InstructionIndex::
                       AssertionPropertyTypeStrictEvaluate) {
    return describe_type_check(message_format(schema_template, step),
                               message_detail_format(schema_template, step),
                               "The value", valid, target.type(),
                               instruction_value<ValueType>(step));
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::AssertionArrayPrefix ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::AssertionArrayPrefixEvaluate) {
    assert(keyword == "items" || keyword == "prefixItems");
    assert(!step.children.empty());
    assert(target.is_array());
    return render_message(message_format(schema_template, step),
                          {std::to_string(step.children.size() - 1)});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesMatch ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesMatchClosed) {
    assert(!step.children.empty());
    assert(target.is_object());
    return render_message(message_format(schema_template, step),
                          {std::to_string(step.children.size())});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalWhenDefines) {
    return render_message(message_format(schema_template, step),
                          {instruction_value<ValueProperty>(step).first});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LoopPropertiesRegex ||
      step.type ==
          sourcemeta::blaze::InstructionIndex::LoopPropertiesRegexClosed) {
    assert(target.is_object());
    return render_message(message_format(schema_template, step),
                          {instruction_value<ValueRegex>(step).second});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LoopPropertiesStartsWith) {
    assert(target.is_object());
    return render_message(message_format(schema_template, step),
                          {instruction_value<ValueString>(step)});
  }

  if (step.type == sourcemeta::blaze::InstructionIndex::LogicalWhenType) {
    if (keyword == "items") {
      return describe_type_check(message_format(schema_template, step),
                                 message_detail_format(schema_template, step),
                                 "The value", valid, target.type(),
                                 instruction_value<ValueType>(step));
    }

    // The evaluator only reaches this instruction when the instance is of the
    // type it names, so there is never a mismatch of its own to report
    if (keyword == "properties") {
      assert(target.is_object());
      assert(!step.children.empty());
      return describe_literal(schema_template, step);
    }

    if (keyword == "dependentSchemas") {
      assert(target.is_object());
      assert(!step.children.empty());
      std::set<std::string> present;
      std::set<std::string> all_dependencies;
      for (const auto &child : step.children) {
        assert(child.type == InstructionIndex::LogicalWhenDefines);
        const auto &property{instruction_value<ValueProperty>(child)};
        all_dependencies.insert(property.first);
        if (target.defines(property.first, property.second)) {
          present.insert(property.first);
        }
      }

      std::ostringstream named;
      if (present.empty()) {
        assert(!all_dependencies.empty());
        describe_names(all_dependencies, "or", named);
        return render_message(message_format(schema_template, step),
                              {named.str()});
      }

      describe_names(present, "and", named);
      return render_message(message_detail_format(schema_template, step),
                            {named.str()});
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
      if (target.defines(entry.first, entry.hash)) {
        present.insert(entry.first);
        for (const auto &dependency : entry.second) {
          if (valid || !target.defines(dependency.first, dependency.second)) {
            required.insert(dependency.first);
          }
        }
      }
    }

    std::ostringstream defined;
    if (present.empty()) {
      assert(!all_dependencies.empty());
      describe_names(all_dependencies, "or", defined);
      return render_message(message_format(schema_template, step),
                            {defined.str()});
    }

    describe_names(present, "and", defined);
    assert(!required.empty());
    std::ostringstream expected;
    describe_names(required, "and", expected);

    return render_message(message_detail_format(schema_template, step),
                          {defined.str(), expected.str()});
  }

  if (step.type ==
      sourcemeta::blaze::InstructionIndex::LogicalWhenArraySizeGreater) {
    assert(keyword == "additionalItems" || keyword == "items");
    assert(target.is_array());
    if (target.size() <= instruction_value<ValueUnsignedInteger>(step)) {
      return describe_literal(schema_template, step);
    }

    return std::string{message_detail_format(schema_template, step)};
  }

  return unknown();
}

} // namespace sourcemeta::blaze
