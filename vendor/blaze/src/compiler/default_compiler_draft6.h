#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::ranges::all_of
#include <utility>   // std::to_underlying

#include "compile_helpers.h"

#include "schema_helpers.h"

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
    const auto types{
        parse_schema_type(schema_context.schema.at(dynamic_context.keyword))};
    if (types.test(std::to_underlying(sourcemeta::core::JSON::Type::String))) {
      return {};
    }

    // A set that names known types, none of them a string, can never match a
    // property name, so every name fails
    if (types.any()) {
      return {make(sourcemeta::blaze::InstructionIndex::AssertionFail, context,
                   schema_context, dynamic_context, ValueNone{})};
    }

    // No known type was named at all, which the meta-schema rejects
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        EXPECTED_TYPE_NAMES);
  }

  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      if (type_covered_by_enum(context, schema_context, "null")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "null")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Null)};
    }
    if (type == "boolean") {
      if (type_covered_by_enum(context, schema_context, "boolean")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "boolean")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Boolean)};
    }
    if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};

      const auto fused{fuse_type_size_bound(
          context, schema_context, dynamic_context, minimum, maximum,
          sourcemeta::blaze::InstructionIndex::AssertionTypeObjectUpper,
          sourcemeta::blaze::InstructionIndex::AssertionTypeObjectBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, schema_context, "object")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "object")) {
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
    }
    if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};

      // A preceding array loop at this location already rejects a non-array,
      // so the type assertion is redundant. The array size bounds are only
      // redundant when there are none to enforce, as the loop may be
      // carrying bounds fused from a different schema. Only a loop that
      // truly rejects a non-array qualifies: every other item loop holds
      // vacuously on a non-array, as `items` asserts nothing on one
      if (context.mode == Mode::FastValidation && minimum == 0 &&
          !maximum.has_value() && !current.empty() &&
          current.back().type == sourcemeta::blaze::InstructionIndex::
                                     LoopItemsIntegerBoundedSized &&
          current.back().relative_instance_location ==
              to_pointer(dynamic_context.base_instance_location)) {
        return {};
      }

      const auto fused{fuse_type_size_bound(
          context, schema_context, dynamic_context, minimum, maximum,
          sourcemeta::blaze::InstructionIndex::AssertionTypeArrayUpper,
          sourcemeta::blaze::InstructionIndex::AssertionTypeArrayBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, schema_context, "array")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "array")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Array)};
    }
    if (type == "number") {
      if (type_covered_by_enum(context, schema_context, "number")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "number")) {
        return {};
      }

      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrictAny,
                   context, schema_context, dynamic_context, types)};
    }
    if (type == "integer") {
      if (type_covered_by_enum(context, schema_context, "integer")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "integer")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionType, context,
                   schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::Integer)};
    }
    if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};

      const auto fused{fuse_type_size_bound(
          context, schema_context, dynamic_context, minimum, maximum,
          sourcemeta::blaze::InstructionIndex::AssertionTypeStringUpper,
          sourcemeta::blaze::InstructionIndex::AssertionTypeStringBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, schema_context, "string")) {
        return {};
      }

      if (type_covered_by_const(context, schema_context, "string")) {
        return {};
      }

      return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeStrict,
                   context, schema_context, dynamic_context,
                   sourcemeta::core::JSON::Type::String)};
    }
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        EXPECTED_TYPE_NAMES);
  }
  if (schema_context.schema.at(dynamic_context.keyword).is_array() &&
      schema_context.schema.at(dynamic_context.keyword).size() == 1 &&
      schema_context.schema.at(dynamic_context.keyword).front().is_string()) {
    auto assertion{make_type_assertion(
        context, schema_context, dynamic_context,
        schema_context.schema.at(dynamic_context.keyword).front().to_string())};
    if (assertion.has_value()) {
      return {std::move(assertion).value()};
    }

    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        EXPECTED_TYPE_NAMES);
  }
  if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    // The meta-schema asks for a non-empty array of unique type names, and a
    // union that does not satisfy that is not a constraint at all
    if (schema_context.schema.at(dynamic_context.keyword).empty() ||
        !schema_context.schema.at(dynamic_context.keyword).unique()) {
      throw sourcemeta::blaze::CompilerError(
          schema_context.base,
          absolute_schema_location(context, schema_context),
          EXPECTED_TYPE_NAMES);
    }

    ValueTypes types{};
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      if (!type.is_string()) {
        throw sourcemeta::blaze::CompilerError(
            schema_context.base,
            absolute_schema_location(context, schema_context),
            EXPECTED_TYPE_NAMES);
      }

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
      } else {
        throw sourcemeta::blaze::CompilerError(
            schema_context.base,
            absolute_schema_location(context, schema_context),
            EXPECTED_TYPE_NAMES);
      }
    }

    assert(types.any());
    return {make(sourcemeta::blaze::InstructionIndex::AssertionTypeAny, context,
                 schema_context, dynamic_context, types)};
  }

  // Neither a type name nor a union of them, which the meta-schema rejects
  throw sourcemeta::blaze::CompilerError(
      schema_context.base, absolute_schema_location(context, schema_context),
      EXPECTED_TYPE_NAMES);
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
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        EXPECTED_NUMBER);
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
    throw sourcemeta::blaze::CompilerError(
        schema_context.base, absolute_schema_location(context, schema_context),
        EXPECTED_NUMBER);
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
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

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
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (children.empty()) {
    return {};
  }

  return {make(sourcemeta::blaze::InstructionIndex::LoopKeys, context,
               schema_context, dynamic_context, ValueNone{},
               std::move(children))};
}

} // namespace internal
#endif
