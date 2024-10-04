#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT7_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_DRAFT7_H_

#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "compile_helpers.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

// TODO: Don't generate `if` if neither `then` nor `else` is defined
auto compiler_draft7_applicator_if(
    const SchemaCompilerContext &context,
    const SchemaCompilerSchemaContext &schema_context,
    const SchemaCompilerDynamicContext &dynamic_context)
    -> SchemaCompilerTemplate {
  // `if`
  SchemaCompilerTemplate children{compile(
      context, schema_context, dynamic_context, empty_pointer, empty_pointer)};

  // `then`
  std::size_t then_cursor{0};
  if (schema_context.schema.defines("then")) {
    then_cursor = children.size();
    const auto destination{
        to_uri(schema_context.relative_pointer.initial().concat({"then"}),
               schema_context.base)
            .recompose()};
    assert(context.frame.contains({ReferenceType::Static, destination}));
    for (auto &&step :
         compile(context, schema_context, relative_dynamic_context, {"then"},
                 empty_pointer, destination)) {
      children.push_back(std::move(step));
    }

    // In this case, `if` did nothing, so we can short-circuit
    if (then_cursor == 0) {
      return children;
    }
  }

  // `else`
  std::size_t else_cursor{0};
  if (schema_context.schema.defines("else")) {
    else_cursor = children.size();
    const auto destination{
        to_uri(schema_context.relative_pointer.initial().concat({"else"}),
               schema_context.base)
            .recompose()};
    assert(context.frame.contains({ReferenceType::Static, destination}));
    for (auto &&step :
         compile(context, schema_context, relative_dynamic_context, {"else"},
                 empty_pointer, destination)) {
      children.push_back(std::move(step));
    }
  }

  return {make<SchemaCompilerLogicalCondition>(
      false, context,
      {schema_context.relative_pointer.initial(), schema_context.schema,
       schema_context.vocabularies, schema_context.base, schema_context.labels,
       schema_context.references},
      relative_dynamic_context, {then_cursor, else_cursor},
      std::move(children))};
}

// We handle `then` as part of `if`
auto compiler_draft7_applicator_then(const SchemaCompilerContext &,
                                     const SchemaCompilerSchemaContext &,
                                     const SchemaCompilerDynamicContext &)
    -> SchemaCompilerTemplate {
  return {};
}

// We handle `else` as part of `if`
auto compiler_draft7_applicator_else(const SchemaCompilerContext &,
                                     const SchemaCompilerSchemaContext &,
                                     const SchemaCompilerDynamicContext &)
    -> SchemaCompilerTemplate {
  return {};
}

} // namespace internal
#endif
