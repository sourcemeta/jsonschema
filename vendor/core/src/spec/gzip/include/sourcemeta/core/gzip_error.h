#ifndef SOURCEMETA_CORE_GZIP_ERROR_H_
#define SOURCEMETA_CORE_GZIP_ERROR_H_

#ifndef SOURCEMETA_CORE_GZIP_EXPORT
#include <sourcemeta/core/gzip_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup gzip
/// An error that represents a GZIP compression or decompression failure
class SOURCEMETA_CORE_GZIP_EXPORT GZIPError : public Error {
public:
  /// Construct an error with the given message
  GZIPError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
