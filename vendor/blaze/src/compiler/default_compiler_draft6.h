#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::ranges::all_of
#include <utility>   // std::to_underlying

#include "compile_helpers.h"
#include "upstream.h"

namespace sourcemeta::blaze {

auto compiler_draft6_validation_type(const KeywordContext &context,
                                     const Instructions &current)
    -> Instructions {
  // Every property name is a string, so inside `propertyNames` this keyword
  // is decidable at compile time. Note that the evaluator applies such a
  // subschema to a null placeholder and carries the name out of band, so a
  // type assertion emitted here would test that placeholder rather than the
  // name. The failure is emitted per name, as these instructions become the
  // children of the loop over the keys, so an object with no properties
  // still passes
  if (context.local.is_property_name) {
    const auto types{sourcemeta::core::schema_type_set(context.value())};
    if (types.test(std::to_underlying(sourcemeta::core::JSON::Type::String))) {
      return {};
    }

    // A set that names known types, none of them a string, can never match a
    // property name, so every name fails
    if (types.any()) {
      return {make(InstructionIndex::AssertionFail, context, ValueNone{})};
    }

    // No known type was named at all, which the meta-schema rejects
    throw_keyword_error(context, EXPECTED_TYPE_NAMES);
  }

  if (context.value().is_string()) {
    const auto &type{context.value().to_string()};
    if (type == "null") {
      if (type_covered_by_enum(context, "null")) {
        return {};
      }

      if (type_covered_by_const(context, "null")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Null)};
    }
    if (type == "boolean") {
      if (type_covered_by_enum(context, "boolean")) {
        return {};
      }

      if (type_covered_by_const(context, "boolean")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Boolean)};
    }
    if (type == "object") {
      const auto minimum{
          unsigned_integer_property(context.schema(), "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(context.schema(), "maxProperties")};

      const auto fused{fuse_type_size_bound(
          context, minimum, maximum, InstructionIndex::AssertionTypeObjectUpper,
          InstructionIndex::AssertionTypeObjectBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, "object")) {
        return {};
      }

      if (type_covered_by_const(context, "object")) {
        return {};
      }

      // A non-empty `required` rejects a non-object on its own, so the type
      // assertion is redundant. An empty `required` asserts nothing, so the
      // type check must stay
      if (context.mode() == Mode::FastValidation &&
          context.schema().defines("required") &&
          context.schema().at("required").is_array() &&
          !context.schema().at("required").empty()) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Object)};
    }
    if (type == "array") {
      const auto minimum{
          unsigned_integer_property(context.schema(), "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(context.schema(), "maxItems")};

      // A preceding array loop at this location already rejects a non-array,
      // so the type assertion is redundant. The array size bounds are only
      // redundant when there are none to enforce, as the loop may be
      // carrying bounds fused from a different schema. Only a loop that
      // truly rejects a non-array qualifies: every other item loop holds
      // vacuously on a non-array, as `items` asserts nothing on one
      if (context.mode() == Mode::FastValidation && minimum == 0 &&
          !maximum.has_value() && !current.empty() &&
          current.back().type ==
              InstructionIndex::LoopItemsIntegerBoundedSized &&
          current.back().relative_instance_location ==
              to_pointer(context.target.base_instance_location)) {
        return {};
      }

      const auto fused{fuse_type_size_bound(
          context, minimum, maximum, InstructionIndex::AssertionTypeArrayUpper,
          InstructionIndex::AssertionTypeArrayBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, "array")) {
        return {};
      }

      if (type_covered_by_const(context, "array")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::Array)};
    }
    if (type == "number") {
      if (type_covered_by_enum(context, "number")) {
        return {};
      }

      if (type_covered_by_const(context, "number")) {
        return {};
      }

      ValueTypes types{};
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Real));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Integer));
      types.set(std::to_underlying(sourcemeta::core::JSON::Type::Decimal));
      return {make(InstructionIndex::AssertionTypeStrictAny, context, types)};
    }
    if (type == "integer") {
      if (type_covered_by_enum(context, "integer")) {
        return {};
      }

      if (type_covered_by_const(context, "integer")) {
        return {};
      }

      return {make(InstructionIndex::AssertionType, context,
                   sourcemeta::core::JSON::Type::Integer)};
    }
    if (type == "string") {
      const auto minimum{
          unsigned_integer_property(context.schema(), "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(context.schema(), "maxLength")};

      const auto fused{fuse_type_size_bound(
          context, minimum, maximum, InstructionIndex::AssertionTypeStringUpper,
          InstructionIndex::AssertionTypeStringBounded)};
      if (fused.has_value()) {
        return fused.value();
      }

      if (type_covered_by_enum(context, "string")) {
        return {};
      }

      if (type_covered_by_const(context, "string")) {
        return {};
      }

      return {make(InstructionIndex::AssertionTypeStrict, context,
                   sourcemeta::core::JSON::Type::String)};
    }
    throw_keyword_error(context, EXPECTED_TYPE_NAMES);
  }
  if (context.value().is_array() && context.value().size() == 1 &&
      context.value().front().is_string()) {
    auto assertion{
        make_type_assertion(context, context.value().front().to_string())};
    if (assertion.has_value()) {
      return {std::move(assertion).value()};
    }

    throw_keyword_error(context, EXPECTED_TYPE_NAMES);
  }
  if (context.value().is_array()) {
    // The meta-schema asks for a non-empty array of unique type names, and a
    // union that does not satisfy that is not a constraint at all
    if (context.value().empty() || !context.value().unique()) {
      throw_keyword_error(context, EXPECTED_TYPE_NAMES);
    }

    ValueTypes types{};
    for (const auto &type : context.value().as_array()) {
      if (!type.is_string()) {
        throw_keyword_error(context, EXPECTED_TYPE_NAMES);
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
        throw_keyword_error(context, EXPECTED_TYPE_NAMES);
      }
    }

    assert(types.any());
    return {make(InstructionIndex::AssertionTypeAny, context, types)};
  }

  // Neither a type name nor a union of them, which the meta-schema rejects
  throw_keyword_error(context, EXPECTED_TYPE_NAMES);
}

auto compiler_draft6_validation_const(const KeywordContext &context,
                                      const Instructions &) -> Instructions {
  return {make(InstructionIndex::AssertionEqual, context,
               sourcemeta::core::JSON{context.value()})};
}

auto compiler_draft6_validation_exclusivemaximum(const KeywordContext &context,
                                                 const Instructions &)
    -> Instructions {
  if (!context.value().is_number()) {
    throw_keyword_error(context, EXPECTED_NUMBER);
  }

  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "integer" &&
      context.schema().at("type").to_string() != "number") {
    return {};
  }

  return {make(InstructionIndex::AssertionLess, context,
               sourcemeta::core::JSON{context.value()})};
}

auto compiler_draft6_validation_exclusiveminimum(const KeywordContext &context,
                                                 const Instructions &)
    -> Instructions {
  if (!context.value().is_number()) {
    throw_keyword_error(context, EXPECTED_NUMBER);
  }

  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "integer" &&
      context.schema().at("type").to_string() != "number") {
    return {};
  }

  return {make(InstructionIndex::AssertionGreater, context,
               sourcemeta::core::JSON{context.value()})};
}

auto compiler_draft6_applicator_contains(const KeywordContext &context,
                                         const Instructions &) -> Instructions {
  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "array") {
    return {};
  }

  Instructions children{compile(context.relative(),
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (children.empty()) {
    // We still need to check the instance is not empty
    return {make(InstructionIndex::AssertionArraySizeGreater, context,
                 ValueUnsignedInteger{0})};
  }

  return {make(InstructionIndex::LoopContains, context,
               ValueRange{1, std::nullopt, false}, std::move(children))};
}

auto compiler_draft6_validation_propertynames(const KeywordContext &context,
                                              const Instructions &)
    -> Instructions {
  if (context.schema().defines("type") &&
      context.schema().at("type").is_string() &&
      context.schema().at("type").to_string() != "object") {
    return {};
  }

  // TODO: How can we avoid this copy?
  auto nested_schema_context = context.local;
  nested_schema_context.is_property_name = true;
  const KeywordContext nested_context{.global = context.global,
                                      .local = nested_schema_context,
                                      .target = context.relative().target};
  Instructions children{compile(nested_context,
                                sourcemeta::core::EMPTY_WEAK_POINTER,
                                sourcemeta::core::EMPTY_WEAK_POINTER)};

  if (children.empty()) {
    return {};
  }

  return {make(InstructionIndex::LoopKeys, context, ValueNone{},
               std::move(children))};
}

} // namespace sourcemeta::blaze
#endif
