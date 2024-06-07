#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft6_validation_type(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  if (context.value.is_string()) {
    const auto &type{context.value.to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Null, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Object, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Array, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer},
          {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionType>(
          context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::String, {}, SchemaCompilerTargetType::Instance)};
    } else {
      return {};
    }
  } else if (context.value.is_array() && context.value.size() == 1 &&
             context.value.front().is_string()) {
    const auto &type{context.value.front().to_string()};
    if (type == "null") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Null, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "boolean") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Boolean, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "object") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Object, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "array") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Array, {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "number") {
      return {make<SchemaCompilerAssertionTypeStrictAny>(
          context, std::set<JSON::Type>{JSON::Type::Real, JSON::Type::Integer},
          {}, SchemaCompilerTargetType::Instance)};
    } else if (type == "integer") {
      return {make<SchemaCompilerAssertionType>(
          context, JSON::Type::Integer, {},
          SchemaCompilerTargetType::Instance)};
    } else if (type == "string") {
      return {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::String, {}, SchemaCompilerTargetType::Instance)};
    } else {
      return {};
    }
  } else if (context.value.is_array()) {
    std::set<JSON::Type> types;
    for (const auto &type : context.value.as_array()) {
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

    assert(types.size() >= context.value.size());
    return {make<SchemaCompilerAssertionTypeAny>(
        context, std::move(types), {}, SchemaCompilerTargetType::Instance)};
  }

  return {};
}

auto compiler_draft6_validation_const(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerAssertionEqual>(
      context, context.value, {}, SchemaCompilerTargetType::Instance)};
}

auto compiler_draft6_validation_exclusivemaximum(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  const auto subcontext{applicate(context)};

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{
      make<SchemaCompilerLogicalOr>(subcontext, SchemaCompilerValueNone{},
                                    {make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
                                     make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
                                    SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionLess>(
      context, context.value, std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft6_validation_exclusiveminimum(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  assert(context.value.is_number());
  const auto subcontext{applicate(context)};

  // TODO: As an optimization, avoid this condition if the subschema
  // declares `type` to `number` or `integer` already
  SchemaCompilerTemplate condition{
      make<SchemaCompilerLogicalOr>(subcontext, SchemaCompilerValueNone{},
                                    {make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Real, {},
                                         SchemaCompilerTargetType::Instance),
                                     make<SchemaCompilerAssertionTypeStrict>(
                                         subcontext, JSON::Type::Integer, {},
                                         SchemaCompilerTargetType::Instance)},
                                    SchemaCompilerTemplate{})};

  return {make<SchemaCompilerAssertionGreater>(
      context, context.value, std::move(condition),
      SchemaCompilerTargetType::Instance)};
}

auto compiler_draft6_applicator_contains(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};
  return {make<SchemaCompilerLoopContains>(
      context, SchemaCompilerValueNone{},
      compile(subcontext, empty_pointer, empty_pointer),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `array` already
      {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Array, {},
          SchemaCompilerTargetType::Instance)})};
}

auto compiler_draft6_validation_propertynames(
    const SchemaCompilerContext &context) -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};
  return {make<SchemaCompilerLoopKeys>(
      context, SchemaCompilerValueNone{},
      compile(subcontext, empty_pointer, empty_pointer),

      // TODO: As an optimization, avoid this condition if the subschema
      // declares `type` to `object` already
      {make<SchemaCompilerAssertionTypeStrict>(
          context, JSON::Type::Object, {},
          SchemaCompilerTargetType::Instance)})};
}

} // namespace internal
#endif
