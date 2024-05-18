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

} // namespace internal
#endif
