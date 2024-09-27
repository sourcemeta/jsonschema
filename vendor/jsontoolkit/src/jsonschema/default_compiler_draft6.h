#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft6_validation_type(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.at(dynamic_context.keyword).is_string()) {
    const auto &type{
        schema_context.schema.at(dynamic_context.keyword).to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Boolean)};
    } else if (type == "object") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minProperties", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxProperties")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<SchemaCompilerAssertionTypeObjectBounded>(
            true, context, schema_context, dynamic_context,
            {minimum, maximum, false})};
      }

      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Object)};
    } else if (type == "array") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minItems", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxItems")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<SchemaCompilerAssertionTypeArrayBounded>(
            true, context, schema_context, dynamic_context,
            {minimum, maximum, false})};
      }

      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Array)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<JSON::Type>{JSON::Type::Real, JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionType>(
          true, context, schema_context, dynamic_context, JSON::Type::Integer)};
    } else if (type == "string") {
      const auto minimum{
          unsigned_integer_property(schema_context.schema, "minLength", 0)};
      const auto maximum{
          unsigned_integer_property(schema_context.schema, "maxLength")};
      if (minimum > 0 || maximum.has_value()) {
        return {make<SchemaCompilerAssertionTypeStringBounded>(
            true, context, schema_context, dynamic_context,
            {minimum, maximum, false})};
      }

      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::String)};
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
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Null)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Boolean)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Object)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::Array)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          true, context, schema_context, dynamic_context,
          std::vector<JSON::Type>{JSON::Type::Real, JSON::Type::Integer})};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionType>(
          true, context, schema_context, dynamic_context, JSON::Type::Integer)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          true, context, schema_context, dynamic_context, JSON::Type::String)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::vector<JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.push_back(JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.push_back(JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.push_back(JSON::Type::Object);
      } else if (type_string == "array") {
        types.push_back(JSON::Type::Array);
      } else if (type_string == "number") {
        types.push_back(JSON::Type::Integer);
        types.push_back(JSON::Type::Real);
      } else if (type_string == "integer") {
        types.push_back(JSON::Type::Integer);
      } else if (type_string == "string") {
        types.push_back(JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<SchemaCompilerAssertionTypeAny>(
        true, context, schema_context, dynamic_context, std::move(types))};
  }

  return {};
}

auto compiler_draft6_validation_const(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerAssertionEqual>(
      true, context, schema_context, dynamic_context,
      JSON{schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusivemaximum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<SchemaCompilerAssertionLess>(
      true, context, schema_context, dynamic_context,
      JSON{schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_validation_exclusiveminimum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "integer" &&
      schema_context.schema.at("type").to_string() != "number") {
    return {};
  }

  return {make<SchemaCompilerAssertionGreater>(
      true, context, schema_context, dynamic_context,
      JSON{schema_context.schema.at(dynamic_context.keyword)})};
}

auto compiler_draft6_applicator_contains(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "array") {
    return {};
  }

  return {make<SchemaCompilerLoopContains>(
      true, context, schema_context, dynamic_context,
      SchemaCompilerValueRange{1, std::nullopt, false},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};
}

auto compiler_draft6_validation_propertynames(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  if (schema_context.schema.defines("type") &&
      schema_context.schema.at("type").is_string() &&
      schema_context.schema.at("type").to_string() != "string") {
    return {};
  }

  return {make<SchemaCompilerLoopKeys>(
      true, context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};
}

} // namespace internal
#endif
