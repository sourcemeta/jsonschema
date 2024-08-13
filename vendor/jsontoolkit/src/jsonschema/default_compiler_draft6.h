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
          context, schema_context, dynamic_context, JSON::Type::Null, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer}, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionType>(
          context, schema_context, dynamic_context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::String, {},
          SchemaCompilerTargetType::Instance)};
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
          context, schema_context, dynamic_context, JSON::Type::Null, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, schema_context, dynamic_context,
          std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer}, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionType>(
          context, schema_context, dynamic_context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, dynamic_context, JSON::Type::String, {},
          SchemaCompilerTargetType::Instance)};
    } else {
      return {};
    }
  } else if (schema_context.schema.at(dynamic_context.keyword).is_array()) {
    std::set<JSON::Type> types;
    for (const auto &type :
         schema_context.schema.at(dynamic_context.keyword).as_array()) {
      assert(type.is_string());
      const auto &type_string{type.to_string()};
      if (type_string == "null") {
        types.emplace(JSON::Type::Null);
      } else if (type_string == "boolean") {
        types.emplace(JSON::Type::Boolean);
      } else if (type_string == "object") {
        types.emplace(JSON::Type::Object);
      } else if (type_string == "array") {
        types.emplace(JSON::Type::Array);
      } else if (type_string == "number") {
        types.emplace(JSON::Type::Integer);
        types.emplace(JSON::Type::Real);
      } else if (type_string == "integer") {
        types.emplace(JSON::Type::Integer);
      } else if (type_string == "string") {
        types.emplace(JSON::Type::String);
      }
    }

    assert(types.size() >=
           schema_context.schema.at(dynamic_context.keyword).size());
    return {make<SchemaCompilerAssertionTypeAny>(
        context, schema_context, dynamic_context, std::move(types), {},
        SchemaCompilerTargetType::Instance)};
  }

  return {};
}

auto compiler_draft6_validation_const(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerAssertionEqual>(
      context, schema_context, dynamic_context,
      schema_context.schema.at(dynamic_context.keyword), {},
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft6_validation_exclusivemaximum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      context, schema_context, relative_dynamic_context, false,
      {make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context, JSON::Type::Real,
           {}, SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context,
           JSON::Type::Integer, {}, SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionLess>(
      context, schema_context, dynamic_context,
      schema_context.schema.at(dynamic_context.keyword), std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft6_validation_exclusiveminimum(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.at(dynamic_context.keyword).is_number());

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{make<SchemaCompilerLogicalOr>(
      context, schema_context, relative_dynamic_context, false,
      {make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context, JSON::Type::Real,
           {}, SchemaCompilerTargetType::Instance),
       make<SchemaCompilerAssertionTypeStrict>(
           context, schema_context, relative_dynamic_context,
           JSON::Type::Integer, {}, SchemaCompilerTargetType::Instance)},
      SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionGreater>(
      context, schema_context, dynamic_context,
      schema_context.schema.at(dynamic_context.keyword), std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft6_applicator_contains(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerLoopContains>(
      context, schema_context, dynamic_context,
      SchemaCompilerValueRange{1, std::nullopt, false},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `array` already
      {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, relative_dynamic_context, JSON::Type::Array,
          {}, SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft6_validation_propertynames(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerLoopKeys>(
      context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionTypeStrict>(
          context, schema_context, relative_dynamic_context, JSON::Type::Object,
          {}, SchemaCompilerTargetType::Instance)})};
}

} // namespace internal
#endif
