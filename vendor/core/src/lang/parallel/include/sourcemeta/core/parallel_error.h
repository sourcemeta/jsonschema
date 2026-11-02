#ifndef SOURCEMETA_CORE_PARALLEL_ERROR_H_
#define SOURCEMETA_CORE_PARALLEL_ERROR_H_

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

/// @ingroup parallel
/// An error that represents a worker thread the platform refused to create
class ParallelThreadError : public Error {
public:
  /// Construct an error with the given message
  ParallelThreadError(const StaticString message) noexcept : Error{message} {}
};

} // namespace sourcemeta::core

#endif
