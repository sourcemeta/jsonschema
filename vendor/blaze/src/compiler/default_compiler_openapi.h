#ifndef SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_OPENAPI_H_
#define SOURCEMETA_BLAZE_COMPILER_DEFAULT_COMPILER_OPENAPI_H_

#include <sourcemeta/blaze/compiler.h>

namespace internal {
using namespace sourcemeta::blaze;

auto compiler_openapi_noop(const Context &, const SchemaContext &,
                           const DynamicContext &, const Instructions &)
    -> Instructions {
  return {};
}

} // namespace internal

#endif
