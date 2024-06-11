#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT7_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT7_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_draft7_applicator_if(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children{
      compile(subcontext, empty_pointer, empty_pointer)};
  children.push_back(make<SchemaCompilerAnnotationPrivate>(
      subcontext, JSON{true}, {}, SchemaCompilerTargetType::Instance));
  return {make<SchemaCompilerLogicalTry>(context, SchemaCompilerValueNone{},
                                         std::move(children),
                                         SchemaCompilerTemplate{})};
}

auto compiler_draft7_applicator_then(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.schema.is_object());

  // Nothing to do here
  if (!context.schema.defines("if")) {
    return {};
  }

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children{
      compile(subcontext, empty_pointer, empty_pointer)};
  SchemaCompilerTemplate condition{make<SchemaCompilerInternalAnnotation>(
      subcontext, JSON{true}, {}, SchemaCompilerTargetType::AdjacentAnnotations,
      Pointer{"if"})};
  return {make<SchemaCompilerLogicalAnd>(context, SchemaCompilerValueNone{},
                                         std::move(children),
                                         std::move(condition))};
}

auto compiler_draft7_applicator_else(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  assert(context.schema.is_object());

  // Nothing to do here
  if (!context.schema.defines("if")) {
    return {};
  }

  const auto subcontext{applicate(context)};
  SchemaCompilerTemplate children{
      compile(subcontext, empty_pointer, empty_pointer)};
  SchemaCompilerTemplate condition{make<SchemaCompilerInternalNoAnnotation>(
      subcontext, JSON{true}, {}, SchemaCompilerTargetType::AdjacentAnnotations,
      Pointer{"if"})};
  return {make<SchemaCompilerLogicalAnd>(context, SchemaCompilerValueNone{},
                                         std::move(children),
                                         std::move(condition))};
}

} // namespace internal
#endif
