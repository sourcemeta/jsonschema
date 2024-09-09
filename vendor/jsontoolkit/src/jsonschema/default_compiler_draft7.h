#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT7_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT7_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft7_applicator_if(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  return {make<SchemaCompilerLogicalTryMark>(
      true, context, schema_context, dynamic_context, SchemaCompilerValueNone{},
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};
}

auto compiler_draft7_applicator_then(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.is_object());

  // Nothing to do here
  if (!schema_context.schema.defines("if")) {
    return {};
  }

  return {make<SchemaCompilerLogicalWhenAdjacentMarked>(
      true, context, schema_context, dynamic_context, "if",
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};
}

auto compiler_draft7_applicator_else(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  assert(schema_context.schema.is_object());

  // Nothing to do here
  if (!schema_context.schema.defines("if")) {
    return {};
  }

  return {make<SchemaCompilerLogicalWhenAdjacentUnmarked>(
      true, context, schema_context, dynamic_context, "if",
      compile(context, schema_context, relative_dynamic_context, empty_pointer,
              empty_pointer))};
}

} // namespace internal
#endif
