#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT6_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT6_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

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
