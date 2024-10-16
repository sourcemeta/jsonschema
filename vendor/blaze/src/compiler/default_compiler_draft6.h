#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/blaze/compiler.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_draft6_validation_type(const Context &context,
                                     const SchemaContext &schema_context,
                                     const DynamicContext &dynamic_context)
    -> Template {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<AssertionTypeObjectBounded>(true, context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<AssertionTypeArrayBounded>(true, context, schema_context,
                                                dynamic_context,
                                                {minimum, maximum, false})};
      }

      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      return {make<AssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {
          make<AssertionType>(true, context, schema_context, dynamic_context,
                              sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<AssertionTypeStringBounded>(true, context, schema_context,
                                                 dynamic_context,
                                                 {minimum, maximum, false})};
      }

      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::String)};
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
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Object)};
    } else if (type == "array") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::Array)};
    } else if (type == "number") {
      return {make<AssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<sourcemeta::jsontoolkit::JSON::Type>{
              sourcemeta::jsontoolkit::JSON::Type::Real,
              sourcemeta::jsontoolkit::JSON::Type::Integer})};
    } else if (type == "integer") {
      return {
          make<AssertionType>(true, context, schema_context, dynamic_context,
                              sourcemeta::jsontoolkit::JSON::Type::Integer)};
    } else if (type == "string") {
      return {make<AssertionTypeStrict>(
          true, context, schema_context, dynamic_context,
          sourcemeta::jsontoolkit::JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<sourcemeta::jsontoolkit::JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Integer);
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::Integer);
      } else if (type_string == "string") {
        types.push_back(sourcemeta::jsontoolkit::JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<AssertionTypeAny>(true, context, schema_context,
                                   dynamic_context, std::move(types))};
  }

  return {};
}

auto compiler_draft6_validation_const(const Context &context,
                                      const SchemaContext &schema_context,
                                      const DynamicContext &dynamic_context)
    -> Template {
  return {make<AssertionEqual>(
      true, context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusivemaximum(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<AssertionLess>(
      true, context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusiveminimum(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<AssertionGreater>(
      true, context, schema_context, dynamic_context,
      sourcemeta::jsontoolkit::JSON{
          schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_applicator_contains(const Context &context,
                                         const SchemaContext &schema_context,
                                         const DynamicContext &dynamic_context)
    -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  return {make<LoopContains>(true, context, schema_context, dynamic_context,
                             ValueRange{1, std::nullopt, false},
                             compile(context, schema_context,
                                     relative_dynamic_context,
                                     sourcemeta::jsontoolkit::empty_pointer,
                                     sourcemeta::jsontoolkit::empty_pointer))};
}

auto compiler_draft6_validation_propertynames(
    const Context &context, const SchemaContext &schema_context,
    const DynamicContext &dynamic_context) -> Template {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  return {make<LoopKeys>(
      true, context, schema_context, dynamic_context, ValueNone{},
      compile(context, schema_context, relative_dynamic_context,
              sourcemeta::jsontoolkit::empty_pointer,
              sourcemeta::jsontoolkit::empty_pointer))};
}

} // namespace internal
#endif
