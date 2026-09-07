#include <sourcemeta/core/allocator.h>

namespace sourcemeta::core {

auto allocator() -> Allocator {
#ifdef SOURCEMETA_CORE_ALLOCATOR_MIMALLOC
  return Allocator::Mimalloc;
#else
  return Allocator::System;
#endif
}

} // namespace sourcemeta::core
