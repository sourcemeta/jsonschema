#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2020_12_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_DEFAULT_COMPILER_2020_12_H_

#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include "default_compiler_draft4.h"

namespace internal {
using namespace sourcemeta::jsontoolkit;

auto compiler_2020_12_validation_type(const SchemaCompilerContext &context)
    -> SchemaCompilerTemplate {
  return compiler_draft4_validation_type(context);
}

} // namespace internal

#endif
