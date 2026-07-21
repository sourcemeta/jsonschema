#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::ranges::all_of
#include <utility>   // std::to_underlying

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft6_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context,
                                     const Instructions &current)
    -> Instructions {
  // Every property name is a string, so inside `propertyNames` this keyword
  // is decidable at compile time. Note that the evaluator applies such a
  // subschema to a null placeholder and carries the name out of band, so a
  // type assertion emitted here would test that placeholder rather than the
  // name. The failure is emitted per name, as these instructions become the
  // children of the loop over the keys, so an object with no properties
  // still passes
  if (schema_context.is_property_name) {
    const auto types{sourcemeta::blaze::parse_schema_type(
        schema_context.schema.at(dynamic_context.keyword))};
    if (types.test(std::to_underlying(sourcemeta::core::JSON::Type::String))) {
      return {};
    }

    // A set that names known types, none of them a string, can never match a
    // property name, so every name fails
    if (types.any()) {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                   schema_context, dynamic_context, ValueNone{})};
    }

    // No known type was named. An empty array names nothing at all, so no name
    // can match it, whereas an unrecognised name is an invalid but legitimate
    // use that constrains nothing and is ignored, matching how the forms below
    // treat both outside `propertyNames`
    if (schema_context.schema.at(dynamic_context.keyword).is_array() &&
        schema_context.schema.at(dynamic_context.keyword).empty()) {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                   schema_context, dynamic_context, ValueNone{})};
    }

    return {};
  }

  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_null(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_null()) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Null)};
    } else if (type == "boolean") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_boolean(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_boolean()) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};

      if (context.mode == Mode::FastValidation) {
        if (maximum.has_value() && minimum > maximum.value()) {
          return {make(sourcemeta::blaze::InstructionIndex::AssertionFail,
                       context, schema_context, dynamic_context, ValueNone{})};
        }

        if (maximum.has_value() && minimum == 0) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeObjectUpper,
              context, schema_context, dynamic_context,
              ValueUnsignedInteger{maximum.value()})};
        } else if (minimum > 0 || maximum.has_value()) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeObjectBounded,
              context, schema_context, dynamic_context,
              ValueRange{minimum, maximum, false})};
        }
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_object(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_object()) {
        return {};
      }

      // A non-empty `required` rejects a non-object on its own, so the type
      // assertion is redundant. An empty `required` asserts nothing, so the
      // type check must stay
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("required") &&
          schema_context.schema.at("required").is_array() &&
          !schema_context.schema.at("required").empty()) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};

      // A preceding array loop at this location already rejects a non-array,
      // so the type assertion is redundant. The array size bounds are only
      // redundant when there are none to enforce, as the loops other than the
      // sized one do not check the item count. Only a loop that truly rejects
      // a non-array qualifies: some item loops instead pass a non-array
      // untouched, so they are deliberately excluded below
      if (context.mode == Mode::FastValidation && minimum == 0 &&
          !maximum.has_value() && !current.empty() &&
          (current.back().type ==
               sourcemeta::blaze::InstructionIndex::
                   LoopItemsPropertiesExactlyTypeStrictHash ||
           current.back().type ==
               sourcemeta::blaze::InstructionIndex::
                   LoopItemsPropertiesExactlyTypeStrictHash3 ||
           current.back().type == sourcemeta::blaze::InstructionIndex::
                                      LoopItemsIntegerBoundedSized) &&
          current.back().relative_instance_location ==
              to_pointer(dynamic_context.base_instance_location)) {
        return {};
      }

      if (context.mode == Mode::FastValidation) {
        if (maximum.has_value() && minimum > maximum.value()) {
          return {make(sourcemeta::blaze::InstructionIndex::AssertionFail,
                       context, schema_context, dynamic_context, ValueNone{})};
        }

        if (maximum.has_value() && minimum == 0) {
          return {
              make(sourcemeta::blaze::InstructionIndex::AssertionTypeArrayUpper,
                   context, schema_context, dynamic_context,
                   ValueUnsignedInteger{maximum.value()})};
        } else if (minimum > 0 || maximum.has_value()) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeArrayBounded,
              context, schema_context, dynamic_context,
              ValueRange{minimum, maximum, false})};
        }
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_array(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_array()) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Array)};
    } else if (type == "number") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_number(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_number()) {
        return {};
      }

      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, types)};
    } else if (type == "integer") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_integral(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          (schema_context.schema.at("const").is_integral())) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionType, context,
                   schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};

      if (context.mode == Mode::FastValidation) {
        if (maximum.has_value() && minimum > maximum.value()) {
          return {make(sourcemeta::blaze::InstructionIndex::AssertionFail,
                       context, schema_context, dynamic_context, ValueNone{})};
        }

        if (maximum.has_value() && minimum == 0) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeStringUpper,
              context, schema_context, dynamic_context,
              ValueUnsignedInteger{maximum.value()})};
        } else if (minimum > 0 || maximum.has_value()) {
          return {make(
              sourcemeta::blaze::InstructionIndex::AssertionTypeStringBounded,
              context, schema_context, dynamic_context,
              ValueRange{minimum, maximum, false})};
        }
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::ranges::all_of(
              schema_context.schema.at("enum").as_array(),
              [](const auto &value) -> auto { return value.is_string(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_string()) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array() &&
             schema_context.schema.at(dynamic_context.keyword).size() == 1 &&
             schema_context.schema.at(dynamic_context.keyword)
                 .front()
                 .is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).front().to_string()};
    if (type == "null") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Object)};
    } else if (type == "array") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Array)};
    } else if (type == "number") {
      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, types)};
    } else if (type == "integer") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionType, context,
                   schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    } else if (type == "string") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    ValueTypes types{};
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Null));
      } else if (type_string == "boolean") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Boolean));
      } else if (type_string == "object") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Object));
      } else if (type_string == "array") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Array));
      } else if (type_string == "number") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      } else if (type_string == "integer") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      } else if (type_string == "string") {
        types.set(std::to_underlying(sourcemeta::core::JSON::Type::String));
      }
    }

    if (types.none()) {
      // An empty array names no type at all, so no value can match it. A
      // non-empty one that named only unrecognised types is an invalid but
      // legitimate use of the keyword that we ignore, constraining nothing,
      // exactly as the scalar form does for a single unrecognised name
      if (schema_context.schema.at(dynamic_context.keyword).empty()) {
        return {make(sourcemeta::blaze::InstructionIndex::AssertionFail,
                     context, schema_context, dynamic_context, ValueNone{})};
      }

      return {};
    }

    return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeAny, context,
                 schema_context, dynamic_context, types)};
  }

  return {};
}

auto compiler_draft6_validation_const(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context,
                                      const Instructions &) -> Instructions {
  return {make(sourcemeta::blaze::InstructionIndex::AssertionEqual, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON{
                   schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusivemaximum(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_number()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionLess, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON{
                   schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusiveminimum(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (!schema_context.schema.at(dynamic_context.keyword).is_number()) {
    return {};
  }

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::AssertionGreater, context,
               schema_context, dynamic_context,
               sourcemeta::core::JSON{
                   schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_applicator_contains(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context,
                                         const Instructions &) -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  Instructions children{compile(context, schema_context,
                                relative_dynamic_context(),
                                sourcemeta::core::empty_weak_pointer,
                                sourcemeta::core::empty_weak_pointer)};

  if (children.empty()) {
    // We still need to check the instance is not empty
    return {make(sourcemeta::blaze::InstructionIndex::AssertionArraySizeGreater,
                 context, schema_context, dynamic_context,
                 ValueUnsignedInteger{0})};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LoopContains, context,
               schema_context, dynamic_context,
               ValueRange{1, std::nullopt, false}, std::move(children))};
}

auto compiler_draft6_validation_propertynames(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context, const Instructions &)
    -> Instructions {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "object") {
    return {};
  }

  // TODO: How can we avoid this copy?
  auto nested_schema_context = schema_context;
  nested_schema_context.is_property_name = true;
  Instructions children{compile(context, nested_schema_context,
                                relative_dynamic_context(),
                                sourcemeta::core::empty_weak_pointer,
                                sourcemeta::core::empty_weak_pointer)};

  if (children.empty()) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LoopKeys, context,
               schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

} // namespace internal
#endif
