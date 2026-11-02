#ifndef SOURCEMETA_BLAZE_COMPILER_MESSAGE_H_
#define SOURCEMETA_BLAZE_COMPILER_MESSAGE_H_

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <algorithm>   // std::ranges::find
#include <cstddef>     // std::size_t
#include <format>      // std::format
#include <iterator>    // std::distance
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::to_underlying
#include <variant>     // std::get_if
#include <vector>      // std::vector

// The wording of an error message lives here rather than in the evaluator. The
// compiler resolves one template per instruction and the compiled template
// carries the ones a given schema actually uses, which is what lets other
// ports render the same sentences without duplicating them. Every argument
// index is written out, as the renderer rejects an implicit one
namespace sourcemeta::blaze {

inline auto message_string_type(const ValueStringType type)
    -> std::string_view {
  switch (type) {
    case ValueStringType::URI:
      return "The string value {0} was expected to represent a valid URI";
    case ValueStringType::URIReference:
      return "The string value {0} was expected to represent a valid URI "
             "reference";
    case ValueStringType::URITemplate:
      return "The string value {0} was expected to represent a valid URI "
             "template";
    case ValueStringType::IRI:
      return "The string value {0} was expected to represent a valid IRI";
    case ValueStringType::IRIReference:
      return "The string value {0} was expected to represent a valid IRI "
             "reference";
    case ValueStringType::Email:
      return "The string value {0} was expected to represent a valid email "
             "address";
    case ValueStringType::IDNEmail:
      return "The string value {0} was expected to represent a valid "
             "internationalized email address";
    case ValueStringType::IPv4:
      return "The string value {0} was expected to represent a valid IPv4 "
             "address";
    case ValueStringType::IPv6:
      return "The string value {0} was expected to represent a valid IPv6 "
             "address";
    case ValueStringType::Hostname:
      return "The string value {0} was expected to represent a valid hostname";
    case ValueStringType::IDNHostname:
      return "The string value {0} was expected to represent a valid "
             "internationalized hostname";
    case ValueStringType::DateTime:
      return "The string value {0} was expected to represent a valid RFC 3339 "
             "date-time";
    case ValueStringType::Date:
      return "The string value {0} was expected to represent a valid RFC 3339 "
             "full-date";
    case ValueStringType::Time:
      return "The string value {0} was expected to represent a valid RFC 3339 "
             "full-time";
    case ValueStringType::PartialTime:
      return "The string value {0} was expected to represent a valid RFC 3339 "
             "partial-time without fractional seconds";
    case ValueStringType::Duration:
      return "The string value {0} was expected to represent a valid RFC 3339 "
             "duration";
    case ValueStringType::JSONPointer:
      return "The string value {0} was expected to represent a valid JSON "
             "Pointer";
    case ValueStringType::RelativeJSONPointer:
      return "The string value {0} was expected to represent a valid relative "
             "JSON Pointer";
    case ValueStringType::UUID:
      return "The string value {0} was expected to represent a valid UUID";
    case ValueStringType::Regex:
      return "The string value {0} was expected to represent a valid ECMA-262 "
             "regular expression";
    case ValueStringType::Color:
      return "The string value {0} was expected to represent a valid CSS 2 "
             "color";
    case ValueStringType::Base64:
      return "The string value {0} was expected to represent a valid RFC 4648 "
             "Base64 string";
    default:
      return "";
  }
}

// A count paired with its noun, in agreement, as the evaluator spells it out
// for the messages that count at render time
inline auto message_quantity(const std::size_t count,
                             const std::string_view singular,
                             const std::string_view plural) -> std::string {
  return std::format("{} {}", count, count == 1 ? singular : plural);
}

// A single subschema is named without a count or a selection, as "at least
// one of the 1 given subschemas" does not read
inline auto message_subschemas(const std::size_t count,
                               const std::string_view selection)
    -> std::string {
  if (count == 1) {
    return "the given subschema";
  }

  return std::format("{}the {} given subschemas", selection, count);
}

// The numeric relations differ only in the phrase they name. The instance
// value goes in the subject, so none of them reports an actual of its own
inline auto message_comparison(const std::string_view relation) -> std::string {
  return std::string{"The {0} value {1} was expected to be "}
      .append(relation)
      .append(" the {2} {3}");
}

// The three size families differ only in the noun they count, the way the
// bound runs, and the measure they report. The bound is what the schema
// states, so it is spelled out here in agreement, while the size the instance
// turned out to have arrives as a bare number nothing has to agree with. The
// connective is what picks between the two slots
inline auto message_size(const Instruction &instruction, const bool inclusive,
                         const std::string_view singular,
                         const std::string_view plural,
                         const std::string_view opening,
                         const std::string_view connective,
                         const std::string_view measure) -> std::string {
  const auto *limit{std::get_if<ValueUnsignedInteger>(&instruction.value)};
  if (limit == nullptr) {
    return "";
  }

  return std::format(
      "{} {} {} {}", opening,
      message_quantity(inclusive ? *limit + 1 : *limit - 1, singular, plural),
      connective, measure);
}

// The opening and the measure of each size family, which its two slots share
constexpr std::string_view STRING_SIZE_MEASURE{"its length was {1}"};
constexpr std::string_view ARRAY_SIZE_OPENING{
    "The array value was expected to contain"};
constexpr std::string_view ARRAY_SIZE_MEASURE{"its size was {0}"};
constexpr std::string_view OBJECT_SIZE_OPENING{
    "The object value was expected to contain"};
constexpr std::string_view OBJECT_SIZE_MEASURE{"it defines {0} of them{1}"};

// The fused type and size checks name the whole range they accept, which the
// schema states and so needs no argument of its own
inline auto message_bounded(const Instruction &instruction,
                            const std::string_view subject,
                            const std::string_view singular,
                            const std::string_view plural) -> std::string {
  const auto *range{std::get_if<ValueRange>(&instruction.value)};
  if (range == nullptr) {
    return "";
  }

  const auto &[minimum, maximum, exhaustive]{*range};
  if (minimum == 0 && maximum.has_value()) {
    return std::format("The value was expected to consist of {} of at most {}",
                       subject,
                       message_quantity(maximum.value(), singular, plural));
  }

  if (maximum.has_value()) {
    return std::format("The value was expected to consist of {} of {} to {}",
                       subject, minimum,
                       message_quantity(maximum.value(), singular, plural));
  }

  return std::format("The value was expected to consist of {} of at least {}",
                     subject, message_quantity(minimum, singular, plural));
}

// As above, for the variants that only ever carry the upper bound
inline auto message_bounded_upper(const Instruction &instruction,
                                  const std::string_view subject,
                                  const std::string_view singular,
                                  const std::string_view plural)
    -> std::string {
  const auto *bound{std::get_if<ValueUnsignedInteger>(&instruction.value)};
  if (bound == nullptr) {
    return "";
  }

  return std::format("The value was expected to consist of {} of at most {}",
                     subject, message_quantity(*bound, singular, plural));
}

// How many items a containment check accepts, and whether the noun that
// follows agrees with one of them or with more
inline auto message_contains(const Instruction &instruction) -> std::string {
  const auto *range{std::get_if<ValueRange>(&instruction.value)};
  if (range == nullptr) {
    return "";
  }

  const auto &[minimum, maximum, exhaustive]{*range};
  std::string bound;
  auto count{minimum};

  if (!maximum.has_value()) {
    bound = std::format("at least {}", minimum);
  } else if (minimum == maximum.value() && minimum == 0) {
    bound = "any number of";
    count = 0;
  } else if (minimum == maximum.value()) {
    bound = std::format("exactly {}", minimum);
  } else if (minimum == 0) {
    bound = std::format("up to {}", maximum.value());
    count = maximum.value();
  } else {
    bound = std::format("{} to {}", minimum, maximum.value());
    count = maximum.value();
  }

  return std::format(
      "The array value was expected to contain {} {} against the given "
      "subschema",
      bound, count == 1 ? "item that validates" : "items that validate");
}

// How many items of the array the loop skips before it starts, which reads as
// an exception rather than as a count when it skips just the one
inline auto message_items_from(const Instruction &instruction) -> std::string {
  const auto *skip{std::get_if<ValueUnsignedInteger>(&instruction.value)};
  if (skip == nullptr) {
    return "";
  }

  std::string exception;
  if (*skip == 1) {
    exception = " except for the first one";
  } else if (*skip > 0) {
    exception = std::format(" except for the first {}", *skip);
  }

  return std::format("Every item in the array value{} was expected to validate "
                     "against the given subschema",
                     exception);
}

// The keywords that reach the properties no other keyword covers either forbid
// them outright or hold them to a subschema of their own
inline auto message_rest_properties(const std::string_view keyword,
                                    const bool closed) -> std::string {
  if (keyword == "unevaluatedProperties") {
    return closed ? "The object value was not expected to define unevaluated "
                    "properties"
                  : "The object properties not covered by other object "
                    "keywords were expected to validate against this "
                    "subschema";
  }

  if (keyword != "additionalProperties") {
    return "";
  }

  return closed ? "The object value was not expected to define additional "
                  "properties"
                : "The object properties not covered by other adjacent object "
                  "keywords were expected to validate against this subschema";
}

// Whether the first thing an applicator does is refuse, which is how a `false`
// subschema compiles
inline auto message_refuses(const Instruction &instruction) -> bool {
  return !instruction.children.empty() &&
         instruction.children.front().type == InstructionIndex::AssertionFail;
}

// How many types a set names once the numeric ones that read alike are folded
// together. The evaluator folds the same way when it renders the list, as
// `number` already covers an integer and a real alike
inline auto message_type_arity(const Instruction &instruction) -> std::size_t {
  const auto *expected{std::get_if<ValueTypes>(&instruction.value)};
  if (expected == nullptr) {
    return 0;
  }

  ValueTypes types{*expected};
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

  return types.count();
}

// Every annotation message draws on the same fixed arguments, as the keyword
// and the value that pick the sentence are both known here while the phrases
// that fill it are not:
//
//   {0}  the scope the annotation applies to
//   {1}  the value as a quoted string
//   {2}  the value as the evaluator spells it
//   {3}  the values of an array, as a list
//   {4}  the keyword, quoted
//   {5}  the value as JSON
//
// NOLINTNEXTLINE(google-readability-function-size,hicpp-function-size,readability-function-size)
inline auto message_annotation(const std::string_view keyword,
                               const sourcemeta::core::JSON &value)
    -> std::string {
  if (keyword == "properties" && value.is_string()) {
    return "The object property {1} successfully validated against its "
           "property subschema";
  }

  if ((keyword == "items" || keyword == "additionalItems") &&
      value.is_boolean() && value.to_boolean()) {
    return "Every item in the array value was successfully validated";
  }

  if ((keyword == "prefixItems" || keyword == "items") && value.is_integer() &&
      value.is_positive()) {
    if (value.to_integer() == 0) {
      return "The first item of the array value successfully validated against "
             "the first positional subschema";
    }

    return std::format("The first {} items of the array value successfully "
                       "validated against the given positional subschemas",
                       value.to_integer() + 1);
  }

  if (keyword == "prefixItems" && value.is_boolean() && value.to_boolean()) {
    return "Every item of the array value validated against the given "
           "positional subschemas";
  }

  // Note that these annotation values come from the schema as they are
  // written, and nothing forces them to have the type their keyword expects
  if ((keyword == "title" || keyword == "description") && value.is_string()) {
    return std::format("The {} of the {{0}} was {{1}}", keyword);
  }

  if (keyword == "default") {
    return "The default value of the {0} was {5}";
  }

  if ((keyword == "deprecated" || keyword == "readOnly" ||
       keyword == "writeOnly") &&
      value.is_boolean()) {
    const std::string_view quality{keyword == "deprecated" ? "deprecated"
                                   : keyword == "readOnly" ? "read-only"
                                                           : "write-only"};
    return std::format("The {{0}} was{} considered {}",
                       value.to_boolean() ? "" : " not", quality);
  }

  if (keyword == "examples" && value.is_array()) {
    return "Examples of the {0} were {3}";
  }

  if (keyword == "contentEncoding" && value.is_string()) {
    return "The content encoding of the {0} was {1}";
  }

  if (keyword == "contentMediaType" && value.is_string()) {
    return "The content media type of the {0} was {1}";
  }

  if (keyword == "contentSchema") {
    return "When decoded, the {0} was expected to validate against the schema "
           "{2}";
  }

  if (keyword == "format") {
    return "The logical type of the {0} was expected to be {2}";
  }

  if (keyword == "x-format-assertion" && value.is_boolean()) {
    return value.to_boolean()
               ? "A sibling `format` keyword was expected to be enforced as "
                 "an assertion"
               : "A sibling `format` keyword was expected to be collected as "
                 "an annotation";
  }

  if (keyword == "x-jsonld-id" && value.is_string()) {
    return "The JSON-LD predicate was {1}";
  }

  if (keyword == "x-jsonld-id" && value.is_null()) {
    return "The nested JSON-LD predicates were expected to be removed";
  }

  if (keyword == "x-jsonld-reverse" && value.is_string()) {
    return "The reverse JSON-LD predicate was {1}";
  }

  if (keyword == "x-jsonld-reverse" && value.is_null()) {
    return "The nested reverse JSON-LD predicates were expected to be removed";
  }

  if (keyword == "x-jsonld-type" && value.is_string()) {
    return "The JSON-LD type was {1}";
  }

  if (keyword == "x-jsonld-type" && value.is_null()) {
    return "The nested JSON-LD types were expected to be removed";
  }

  if (keyword == "x-jsonld-type" && value.is_array()) {
    return value.empty() ? "No JSON-LD types were assigned"
                         : "The JSON-LD types were {3}";
  }

  if (keyword == "x-jsonld-datatype" && value.is_string()) {
    return "The JSON-LD datatype was {1}";
  }

  if (keyword == "x-jsonld-datatype" && value.is_null()) {
    return "The JSON-LD datatype was expected to be removed";
  }

  if (keyword == "x-jsonld-language" && value.is_string()) {
    return "The natural language was {1}";
  }

  if (keyword == "x-jsonld-language" && value.is_null()) {
    return "The natural language was expected to be removed";
  }

  if (keyword == "x-jsonld-direction" && value.is_string()) {
    return "The base direction was {1}";
  }

  if (keyword == "x-jsonld-direction" && value.is_null()) {
    return "The base direction was expected to be removed";
  }

  if (keyword == "x-jsonld-json" && (value.is_boolean() || value.is_null())) {
    return value.is_boolean() && value.to_boolean()
               ? "The value was expected to be treated as an opaque JSON "
                 "literal"
               : "The value was not expected to be treated as an opaque JSON "
                 "literal";
  }

  if (keyword == "x-jsonld-graph" && (value.is_boolean() || value.is_null())) {
    return value.is_boolean() && value.to_boolean()
               ? "The value was expected to be wrapped in a JSON-LD named "
                 "graph"
               : "The value was not expected to be wrapped in a JSON-LD named "
                 "graph";
  }

  if (keyword == "x-jsonld-container" && value.is_string()) {
    return "The JSON-LD container was {1}";
  }

  if (keyword == "x-jsonld-container" && value.is_null()) {
    return "The JSON-LD container was expected to be removed";
  }

  if (keyword == "x-jsonld-self" && value.is_string()) {
    return "The JSON-LD identifier template was {1}";
  }

  if (keyword == "x-jsonld-self" && value.is_null()) {
    return "The JSON-LD identifier was expected to be removed";
  }

  if (keyword == "x-jsonld-override" && value.is_boolean()) {
    return value.to_boolean()
               ? "The sibling JSON-LD annotations were expected to override "
                 "the nested annotations they shadow"
               : "The sibling JSON-LD annotations were not expected to "
                 "override any other annotations";
  }

  return "The unrecognized keyword {4} was collected as the annotation {2}";
}

// The template a given instruction renders
// NOLINTNEXTLINE(google-readability-function-size,hicpp-function-size,readability-function-size)
inline auto message_template(const Instruction &instruction,
                             const std::string_view keyword) -> std::string {
  switch (instruction.type) {
    case InstructionIndex::AssertionFail:
      if (keyword == "enum") {
        return "The {0} value was not expected to validate against the empty "
               "enumeration";
      }

      if (keyword == "contains") {
        return "The constraints declared for this keyword were not "
               "satisfiable";
      }

      return "No instance is expected to succeed against the false schema";
    case InstructionIndex::AssertionDefines:
      return "The object value was expected to define the property {0}";
    case InstructionIndex::AssertionDefinesStrict:
      return "The value was expected to be an object that defines the "
             "property {0}";
    case InstructionIndex::AssertionDefinesAll:
      return "The object value was expected to define properties {0}";
    case InstructionIndex::AssertionDefinesAllStrict:
      return "The value was expected to be an object that defines properties "
             "{0}";
    case InstructionIndex::AssertionDefinesExactly:
      return "The object value was expected to only define properties {0}";
    case InstructionIndex::AssertionDefinesExactlyStrict:
      return "The value was expected to be an object that only defines "
             "properties {0}";
    case InstructionIndex::AssertionDefinesExactlyStrictHash3: {
      const auto *hashes{std::get_if<ValueStringHashes>(&instruction.value)};
      if (hashes == nullptr) {
        return "";
      }

      return std::format("The value was expected to be an object that only "
                         "defines the {} given properties",
                         hashes->first.size());
    }
    case InstructionIndex::AssertionPropertyDependencies:
      return "The object value did not define {0}";
    case InstructionIndex::AssertionType:
    case InstructionIndex::AssertionTypeStrict:
    case InstructionIndex::AssertionPropertyType:
    case InstructionIndex::AssertionPropertyTypeEvaluate:
    case InstructionIndex::AssertionPropertyTypeStrict:
    case InstructionIndex::AssertionPropertyTypeStrictEvaluate:
      return "{0} was expected to be of type {1}";
    case InstructionIndex::AssertionTypeAny:
    case InstructionIndex::AssertionTypeStrictAny:
    case InstructionIndex::AssertionPropertyTypeStrictAny:
    case InstructionIndex::AssertionPropertyTypeStrictAnyEvaluate:
      return message_type_arity(instruction) == 1
                 ? "{0} was expected to be of type {1}"
                 : "{0} was expected to be of type {1} and it was of type {2}";
    case InstructionIndex::AssertionNotTypeStrictAny:
      // Draft 3 permits a negative `maxLength`, which we compile into a check
      // that the value is not a string, as no string is that short. Saying so
      // in terms of types would not explain the keyword the reader wrote
      if (keyword == "maxLength") {
        return "The value was expected to be of a type other than string, as "
               "no string is short enough to satisfy a negative maximum "
               "length";
      }

      // The lower bounds below compile the same way when the bound is too
      // large to hold, as nothing can be big enough to satisfy one
      if (keyword == "minLength") {
        return "The value was expected to be of a type other than string, as "
               "no string is long enough to satisfy a minimum length that "
               "large";
      }

      if (keyword == "minItems") {
        return "The value was expected to be of a type other than array, as "
               "no array has enough items to satisfy a minimum size that "
               "large";
      }

      if (keyword == "minProperties") {
        return "The value was expected to be of a type other than object, as "
               "no object has enough properties to satisfy a minimum size "
               "that large";
      }

      // `contains` compiles the same way when its `minContains` is too large
      // to hold, as no array has that many items for the subschema to match
      if (keyword == "contains") {
        return "The value was expected to be of a type other than array, as "
               "no array has enough matching items to satisfy a minimum count "
               "that large";
      }

      return message_type_arity(instruction) == 1
                 ? "{0} was expected to NOT be of type {1}"
                 : "{0} was expected to NOT be of type {1} and it was of type "
                   "{2}";
    case InstructionIndex::AssertionTypeStringBounded:
      return message_bounded(instruction, "a string", "character",
                             "characters");
    case InstructionIndex::AssertionTypeStringUpper:
      return message_bounded_upper(instruction, "a string", "character",
                                   "characters");
    case InstructionIndex::AssertionTypeArrayBounded:
      return message_bounded(instruction, "an array", "item", "items");
    case InstructionIndex::AssertionTypeArrayUpper:
      return message_bounded_upper(instruction, "an array", "item", "items");
    case InstructionIndex::AssertionTypeObjectBounded:
      return message_bounded(instruction, "an object", "property",
                             "properties");
    case InstructionIndex::AssertionTypeObjectUpper:
      return message_bounded_upper(instruction, "an object", "property",
                                   "properties");
    case InstructionIndex::AssertionRegex:
      return "{0} was expected to match the regular expression {1}";
    case InstructionIndex::AssertionStringSizeLess:
      return keyword == "maxLength"
                 ? message_size(instruction, false, "character", "characters",
                                "{0} was expected to consist of at most", "and",
                                STRING_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionStringSizeGreater:
      return keyword == "minLength"
                 ? message_size(instruction, true, "character", "characters",
                                "{0} was expected to consist of at least",
                                "and", STRING_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionArraySizeLess:
      return keyword == "maxItems"
                 ? message_size(
                       instruction, false, "item", "items",
                       std::string{ARRAY_SIZE_OPENING}.append(" at most"),
                       "and", ARRAY_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionArraySizeGreater:
      return message_size(instruction, true, "item", "items",
                          std::string{ARRAY_SIZE_OPENING}.append(" at least"),
                          "and", ARRAY_SIZE_MEASURE);
    case InstructionIndex::AssertionObjectSizeLess:
      if (keyword == "additionalProperties") {
        return "The object value was not expected to define additional "
               "properties";
      }

      return keyword == "maxProperties"
                 ? message_size(
                       instruction, false, "property", "properties",
                       std::string{OBJECT_SIZE_OPENING}.append(" at most"),
                       "and", OBJECT_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionObjectSizeGreater:
      return keyword == "minProperties"
                 ? message_size(
                       instruction, true, "property", "properties",
                       std::string{OBJECT_SIZE_OPENING}.append(" at least"),
                       "and", OBJECT_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionEqual:
      return "{0} was expected to equal the {1} constant {2}";
    case InstructionIndex::AssertionEqualsAny: {
      const auto *values{std::get_if<ValueSet>(&instruction.value)};
      if (values == nullptr || values->empty()) {
        return "";
      }

      return values->size() == 1
                 ? "{0} was expected to equal the {1} constant {2}"
                 : std::format("{{0}} was expected to equal one of the {} "
                               "declared values",
                               values->size());
    }
    case InstructionIndex::AssertionEqualsAnyStringHash: {
      const auto *hashes{std::get_if<ValueStringHashes>(&instruction.value)};
      if (hashes == nullptr) {
        return "";
      }

      return hashes->first.size() == 1
                 ? "{0} was expected to equal the given constant"
                 : "{0} was expected to equal one of the given declared "
                   "values";
    }
    case InstructionIndex::AssertionGreaterEqual:
      return message_comparison("greater than or equal to");
    case InstructionIndex::AssertionLessEqual:
      return message_comparison("less than or equal to");
    case InstructionIndex::AssertionGreater:
      return message_comparison("greater than");
    case InstructionIndex::AssertionLess:
      return message_comparison("less than");
    case InstructionIndex::AssertionUnique:
      return "The array value was expected to not contain duplicate items";
    case InstructionIndex::AssertionDivisible:
      return message_comparison("divisible by");
    case InstructionIndex::AssertionTypeIntegerBounded:
    case InstructionIndex::AssertionTypeIntegerBoundedStrict:
      return "The value was expected to be an integer within the given range";
    case InstructionIndex::AssertionTypeIntegerLowerBound:
    case InstructionIndex::AssertionTypeIntegerLowerBoundStrict:
      return "The value was expected to be an integer above the given minimum";
    case InstructionIndex::AssertionNumberIntegerBounded: {
      const auto *bounds{std::get_if<ValueIntegerBounds>(&instruction.value)};
      if (bounds == nullptr) {
        return "";
      }

      return std::format("The {{0}} value {{1}} was expected to be an integer "
                         "between {} and {}",
                         bounds->first, bounds->second);
    }
    case InstructionIndex::AssertionNumberType: {
      const auto *type{std::get_if<ValueNumberType>(&instruction.value)};
      if (type == nullptr) {
        return "";
      }

      return std::format(
          "The {{0}} value {{1}} was expected to be exactly representable as "
          "an IEEE 754 {} precision floating point number",
          *type == ValueNumberType::Float ? "single" : "double");
    }
    case InstructionIndex::AssertionStringType: {
      const auto *type{std::get_if<ValueStringType>(&instruction.value)};
      return type == nullptr ? "" : std::string{message_string_type(*type)};
    }
    case InstructionIndex::AssertionPropertyEqual:
      return "The {0} value {1} was expected to equal the {2} constant {3}";
    case InstructionIndex::AssertionArrayPrefix:
    case InstructionIndex::AssertionArrayPrefixEvaluate:
      return instruction.children.size() <= 2
                 ? "The first item of the array value was expected to "
                   "validate against the corresponding subschemas"
                 : "The first {0} items of the array value were expected to "
                   "validate against the corresponding subschemas";
    case InstructionIndex::AssertionObjectPropertiesSimple:
      return "The object value was expected to validate against the defined "
             "property subschemas";
    case InstructionIndex::AnnotationEmit:
    case InstructionIndex::AnnotationEmitWrapped: {
      const auto *value{std::get_if<ValueJSON>(&instruction.value)};
      return value == nullptr ? "" : message_annotation(keyword, *value);
    }
    case InstructionIndex::AnnotationToParent: {
      const auto *value{std::get_if<ValueJSON>(&instruction.value)};
      if (keyword != "unevaluatedItems" || value == nullptr ||
          !value->is_boolean() || !value->to_boolean()) {
        return "";
      }

      return "At least one item of the array value successfully validated "
             "against the subschema for unevaluated items";
    }
    case InstructionIndex::AnnotationBasenameToParent:
      if (keyword == "patternProperties") {
        return "The object property {0} successfully validated against its "
               "pattern property subschema";
      }

      if (keyword == "additionalProperties") {
        return "The object property {0} successfully validated against the "
               "additional properties subschema";
      }

      if (keyword == "unevaluatedProperties") {
        return "The object property {0} successfully validated against the "
               "subschema for unevaluated properties";
      }

      return keyword == "contains"
                 ? "The item at index {0} of the array value successfully "
                   "validated against the containment check subschema"
                 : "";
    case InstructionIndex::Evaluate:
      return "The instance location was marked as evaluated";
    case InstructionIndex::LogicalNot:
    case InstructionIndex::LogicalNotEvaluate:
      return "The {0} value was expected to not validate against the given "
             "subschema";
    case InstructionIndex::LogicalOr:
      return std::string{"The {0} value was expected to validate against "}
          .append(message_subschemas(instruction.children.size(),
                                     "at least one of "));
    case InstructionIndex::LogicalAnd:
      if (keyword == "allOf" || keyword == "extends") {
        return std::string{"The {0} value was expected to validate against "}
            .append(message_subschemas(instruction.children.size(), ""));
      }

      if (keyword == "$ref") {
        return "The {0} value was expected to validate against the referenced "
               "schema";
      }

      // For the wrapper instruction we emit when hoisting `then` subschemas
      // whose `if` condition compiles to nothing
      return keyword == "then" ? "The {0} value was expected to validate "
                                 "against the given conditional"
                               : "";
    case InstructionIndex::LogicalXor:
      return std::string{"{0} was expected to validate against "}.append(
          message_subschemas(instruction.children.size(),
                             "one and only one of "));
    case InstructionIndex::LogicalCondition:
      return "The {0} value was expected to validate against the given "
             "conditional";
    case InstructionIndex::LogicalWhenType:
      if (keyword == "dependentSchemas") {
        return "The object value did not define {0}";
      }

      // The evaluator only reaches this instruction when the instance is of
      // the type it names, so the wrapper for `properties` always has the
      // subschemas it guards to report on
      if (keyword == "properties") {
        return instruction.children.size() == 1
                   ? "The object value was expected to validate against the "
                     "single defined property subschema"
                   // We cannot provide the specific number of properties, as
                   // the number of children might be flatten out for
                   // performance reasons
                   : "The object value was expected to validate against the "
                     "defined properties subschemas";
      }

      return keyword == "items" ? "{0} was expected to be of type {1}" : "";
    case InstructionIndex::LogicalWhenDefines:
      return "The object value defined the property \"{0}\"";
    case InstructionIndex::LogicalWhenArraySizeGreater:
      return keyword == "additionalItems" || keyword == "items"
                 ? "The array value does not contain additional items not "
                   "described by related keywords"
                 : "";
    case InstructionIndex::LoopPropertiesUnevaluated:
    case InstructionIndex::LoopPropertiesUnevaluatedExcept:
    case InstructionIndex::LoopProperties:
    case InstructionIndex::LoopPropertiesExcept:
      return message_rest_properties(keyword, message_refuses(instruction));
    case InstructionIndex::LoopPropertiesEvaluate:
      return message_rest_properties(keyword,
                                     instruction.children.size() == 1 &&
                                         message_refuses(instruction));
    case InstructionIndex::LoopPropertiesMatch:
      return instruction.children.size() == 1
                 ? "The object value was expected to validate against the "
                   "single defined property subschema"
                 : "The object value was expected to validate against the {0} "
                   "defined properties subschemas";
    case InstructionIndex::LoopPropertiesMatchClosed:
      return instruction.children.size() == 1
                 ? "The object value was expected to validate against the "
                   "single defined property subschema"
                 : "Every object value was expected to validate against one "
                   "of the {0} defined properties subschemas";
    case InstructionIndex::LoopPropertiesRegex:
      return "The object properties that match the regular expression \"{0}\" "
             "were expected to validate against the defined pattern property "
             "subschema";
    case InstructionIndex::LoopPropertiesRegexClosed:
      return "The object properties were expected to match the regular "
             "expression \"{0}\" and validate against the defined pattern "
             "property subschema";
    case InstructionIndex::LoopPropertiesStartsWith:
      return "The object properties that start with the string \"{0}\" were "
             "expected to validate against the defined pattern property "
             "subschema";
    case InstructionIndex::LoopPropertiesType:
    case InstructionIndex::LoopPropertiesTypeEvaluate:
    case InstructionIndex::LoopPropertiesTypeStrict:
    case InstructionIndex::LoopPropertiesTypeStrictEvaluate:
    case InstructionIndex::LoopPropertiesTypeStrictAny:
    case InstructionIndex::LoopPropertiesTypeStrictAnyEvaluate:
      return "The object properties were expected to be of type {0}";
    case InstructionIndex::LoopPropertiesExactlyTypeStrict:
    case InstructionIndex::LoopPropertiesExactlyTypeStrictHash:
      return "The required object properties were expected to be of type {0}";
    case InstructionIndex::LoopKeys:
      return keyword == "propertyNames"
                 ? "The object is empty and no properties were expected to "
                   "validate against the given subschema"
                 : "";
    case InstructionIndex::LoopItems:
      return "Every item in the array value was expected to validate against "
             "the given subschema";
    case InstructionIndex::LoopItemsFrom:
      return message_items_from(instruction);
    case InstructionIndex::LoopItemsUnevaluated:
      return keyword == "unevaluatedItems"
                 ? "The array items not covered by other array keywords, if "
                   "any, were expected to validate against this subschema"
                 : "";
    case InstructionIndex::LoopItemsType:
    case InstructionIndex::LoopItemsTypeStrict:
    case InstructionIndex::LoopItemsTypeStrictAny:
      return "The array items were expected to be of type {0}";
    case InstructionIndex::LoopItemsPropertiesExactlyTypeStrictHash:
    case InstructionIndex::LoopItemsPropertiesExactlyTypeStrictHash3:
      return "Every item in the array was expected to be an object whose "
             "required properties were of type {0}";
    case InstructionIndex::LoopItemsIntegerBounded:
    case InstructionIndex::LoopItemsIntegerBoundedSized:
      return "Every item in the array was expected to be a number within the "
             "given range";
    case InstructionIndex::LoopItemsArrayBoundedTypeStrictAny:
      return "Every item in the array was expected to be an array of the "
             "given size whose items were of the given types";
    case InstructionIndex::LoopContains:
      return message_contains(instruction);
    case InstructionIndex::ControlDynamicAnchorJump:
      if (keyword == "$dynamicRef") {
        return "The {0} value was expected to validate against the first "
               "subschema in scope that declared the dynamic anchor {1}";
      }

      return keyword == "$recursiveRef"
                 ? "The {0} value was expected to validate against the first "
                   "subschema in scope that declared a recursive anchor"
                 : "";
    case InstructionIndex::ControlJump:
      return "The {0} value was expected to validate against the referenced "
             "schema";
    default:
      return "";
  }
}

// The template an instruction renders when it has something of its own to
// report, as against the form it renders when there is nothing to add. Empty
// when the instruction says the same thing either way
// NOLINTNEXTLINE(google-readability-function-size,hicpp-function-size,readability-function-size)
inline auto message_detail_template(const Instruction &instruction,
                                    const std::string_view keyword)
    -> std::string {
  switch (instruction.type) {
    case InstructionIndex::AssertionFail:
      if (keyword == "additionalProperties" ||
          keyword == "unevaluatedProperties") {
        return "The object value was not expected to define the property {0}";
      }

      return keyword == "unevaluatedItems"
                 ? "The array value was not expected to define the item at "
                   "index {0}"
                 : "";
    case InstructionIndex::AssertionDefinesAll:
      return "The object value was expected to define properties {0} but did "
             "not define {1}";
    case InstructionIndex::AssertionDefinesExactly:
      return "The object value was expected to only define properties {0}, "
             "but it also defines {1}";
    case InstructionIndex::AssertionDefinesExactlyStrict:
      return "The value was expected to be an object that only defines "
             "properties {0}, but it also defines {1}";
    case InstructionIndex::AssertionStringSizeLess:
      return keyword == "maxLength"
                 ? message_size(instruction, false, "character", "characters",
                                "{0} was expected to consist of at most", "but",
                                STRING_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionStringSizeGreater:
      return keyword == "minLength"
                 ? message_size(instruction, true, "character", "characters",
                                "{0} was expected to consist of at least",
                                "but", STRING_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionArraySizeLess:
      return keyword == "maxItems"
                 ? message_size(
                       instruction, false, "item", "items",
                       std::string{ARRAY_SIZE_OPENING}.append(" at most"),
                       "but", ARRAY_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionArraySizeGreater:
      return message_size(instruction, true, "item", "items",
                          std::string{ARRAY_SIZE_OPENING}.append(" at least"),
                          "but", ARRAY_SIZE_MEASURE);
    case InstructionIndex::AssertionObjectSizeLess:
      return keyword == "maxProperties"
                 ? message_size(
                       instruction, false, "property", "properties",
                       std::string{OBJECT_SIZE_OPENING}.append(" at most"),
                       "but", OBJECT_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionObjectSizeGreater:
      return keyword == "minProperties"
                 ? message_size(
                       instruction, true, "property", "properties",
                       std::string{OBJECT_SIZE_OPENING}.append(" at least"),
                       "but", OBJECT_SIZE_MEASURE)
                 : "";
    case InstructionIndex::AssertionDefinesExactlyStrictHash3: {
      const auto *hashes{std::get_if<ValueStringHashes>(&instruction.value)};
      if (hashes == nullptr) {
        return "";
      }

      return std::format("The value was expected to be an object that only "
                         "defines the {} given properties, but it also "
                         "defines {{0}}",
                         hashes->first.size());
    }
    case InstructionIndex::AssertionUnique:
      return "The array value contained the following duplicates: {0}";
    case InstructionIndex::AssertionType:
    case InstructionIndex::AssertionTypeStrict:
    case InstructionIndex::AssertionPropertyType:
    case InstructionIndex::AssertionPropertyTypeEvaluate:
    case InstructionIndex::AssertionPropertyTypeStrict:
    case InstructionIndex::AssertionPropertyTypeStrictEvaluate:
    case InstructionIndex::AssertionTypeAny:
    case InstructionIndex::AssertionTypeStrictAny:
    case InstructionIndex::AssertionPropertyTypeStrictAny:
    case InstructionIndex::AssertionPropertyTypeStrictAnyEvaluate:
      return "{0} was expected to be of type {1} but it was of type {2}";
    case InstructionIndex::AssertionNotTypeStrictAny:
      if (keyword == "maxLength" || keyword == "minLength" ||
          keyword == "minItems" || keyword == "minProperties" ||
          keyword == "contains") {
        return "";
      }

      return "{0} was expected to NOT be of type {1} but it was of type "
             "{2}";
    case InstructionIndex::AssertionEqualsAny: {
      const auto *values{std::get_if<ValueSet>(&instruction.value)};
      if (values == nullptr || values->size() <= 1) {
        return "";
      }

      return "{0} was expected to equal one of the following values: {1}";
    }
    case InstructionIndex::AssertionPropertyDependencies:
      return "Because the object value defined {0}, it was also expected to "
             "define {1}";
    case InstructionIndex::LogicalWhenType:
      if (keyword == "dependentSchemas") {
        return "Because the object value defined {0}, it was also expected to "
               "validate against the corresponding subschemas";
      }

      return keyword == "items"
                 ? "{0} was expected to be of type {1} but it was of type {2}"
                 : "";
    case InstructionIndex::LogicalWhenArraySizeGreater:
      return keyword == "additionalItems" || keyword == "items"
                 ? "The array value contains additional items not described "
                   "by related keywords"
                 : "";
    case InstructionIndex::LoopKeys:
      return keyword == "propertyNames"
                 ? "The object value was expected to validate the property "
                   "names {0} against the given subschema"
                 : "";
    default:
      return "";
  }
}

// The keyword an instruction was compiled from, which its schema location
// ends with
inline auto message_keyword(const std::string &keyword_location)
    -> std::string_view {
  const auto separator{keyword_location.rfind('/')};
  if (separator == std::string::npos) {
    return {};
  }

  return std::string_view{keyword_location}.substr(separator + 1);
}

// As with Template::vocabularies, where zero means the instruction refers to
// none of them
inline auto message_intern(std::vector<std::string> &messages,
                           const std::string &message) -> std::size_t {
  if (message.empty()) {
    return 0;
  }

  const auto iterator{std::ranges::find(messages, message)};
  if (iterator == messages.end()) {
    messages.emplace_back(message);
    return messages.size();
  }

  return 1 +
         static_cast<std::size_t>(std::distance(messages.begin(), iterator));
}

inline auto assign_messages(const Instructions &instructions,
                            InstructionExtras &extra,
                            std::vector<std::string> &messages) -> void {
  for (const auto &instruction : instructions) {
    auto &metadata{extra[instruction.extra_index]};
    const auto keyword{message_keyword(metadata.keyword_location)};
    metadata.message =
        message_intern(messages, message_template(instruction, keyword));
    metadata.message_detail =
        message_intern(messages, message_detail_template(instruction, keyword));
    assign_messages(instruction.children, extra, messages);
  }
}

// Walk every instruction, resolve its template, and record where the pool
// holds it
inline auto assign_messages(const std::vector<Instructions> &targets,
                            InstructionExtras &extra,
                            std::vector<std::string> &messages) -> void {
  for (const auto &target : targets) {
    assign_messages(target, extra, messages);
  }
}

} // namespace sourcemeta::blaze

#endif
