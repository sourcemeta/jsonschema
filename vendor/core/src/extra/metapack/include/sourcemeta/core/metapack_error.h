#ifndef SOURCEMETA_CORE_METAPACK_ERROR_H_
#define SOURCEMETA_CORE_METAPACK_ERROR_H_

#ifndef SOURCEMETA_CORE_METAPACK_EXPORT
#include <sourcemeta/core/metapack_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup metapack
/// An error that represents contents that cannot be written as a file of this
/// format
class SOURCEMETA_CORE_METAPACK_EXPORT MetapackError : public Error {
public:
  /// Construct an error with the given message
  MetapackError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
