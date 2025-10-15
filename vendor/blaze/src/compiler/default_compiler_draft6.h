#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::all_of

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft6_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context,
                                     const Instructions &current)
    -> Instructions {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_null(); })) {
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
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_boolean(); })) {
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
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_object(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_object()) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("required")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Object)};
    } else if (type == "array") {
      if (context.mode == Mode::FastValidation && !current.empty() &&
          (current.back().type ==
               sourcemeta::blaze::InstructionIndex::
                   LoopItemsPropertiesExactlyTypeStrictHash ||
           current.back().type ==
               sourcemeta::blaze::InstructionIndex::
                   LoopItemsPropertiesExactlyTypeStrictHash3) &&
          current.back().relative_instance_location ==
              dynamic_context.base_instance_location) {
        return {};
      }

      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};

      if (context.mode == Mode::FastValidation) {
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
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_array(); })) {
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
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_number(); })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          schema_context.schema.at("const").is_number()) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context,
                   std::vector<sourcemeta::core::JSON::Type>{
                       sourcemeta::core::JSON::Type::Real,
                       sourcemeta::core::JSON::Type::Integer})};
    } else if (type == "integer") {
      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("enum") &&
          schema_context.schema.at("enum").is_array() &&
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) {
                        return value.is_integer() || value.is_integer_real();
                      })) {
        return {};
      }

      if (context.mode == Mode::FastValidation &&
          schema_context.schema.defines("const") &&
          (schema_context.schema.at("const").is_integer() ||
           schema_context.schema.at("const").is_integer_real())) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionType, context,
                   schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    } else if (type == "string") {
      if (dynamic_context.property_as_target) {
        return {};
      }

      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};

      if (context.mode == Mode::FastValidation) {
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
          std::all_of(schema_context.schema.at("enum").as_array().cbegin(),
                      schema_context.schema.at("enum").as_array().cend(),
                      [](const auto &value) { return value.is_string(); })) {
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
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context,
                   std::vector<sourcemeta::core::JSON::Type>{
                       sourcemeta::core::JSON::Type::Real,
                       sourcemeta::core::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionType, context,
                   schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    } else if (type == "string") {
      if (dynamic_context.property_as_target) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<sourcemeta::core::JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(sourcemeta::core::JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(sourcemeta::core::JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(sourcemeta::core::JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(sourcemeta::core::JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(sourcemeta::core::JSON::Type::Integer);
        types.push_back(sourcemeta::core::JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(sourcemeta::core::JSON::Type::Integer);
      } else if (type_string == "string") {
        if (dynamic_context.property_as_target) {
          continue;
        }

        types.push_back(sourcemeta::core::JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeAny, context,
                 schema_context, dynamic_context, std::move(types))};
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

  Instructions children{compile(
      context, schema_context, relative_dynamic_context(dynamic_context),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

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
  Instructions children{compile(
      context, nested_schema_context, property_relative_dynamic_context(),
      sourcemeta::core::empty_pointer, sourcemeta::core::empty_pointer)};

  if (children.empty()) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LoopKeys, context,
               schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

} // namespace internal
#endif
