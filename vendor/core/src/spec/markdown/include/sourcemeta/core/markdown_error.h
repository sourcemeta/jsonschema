#ifndef SOURCEMETA_CORE_MARKDOWN_ERROR_H_
#define SOURCEMETA_CORE_MARKDOWN_ERROR_H_

#ifndef SOURCEMETA_CORE_MARKDOWN_EXPORT
#include <sourcemeta/core/markdown_export.h>
#endif

#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup markdown
/// An error that represents a Markdown conversion that would exceed one of the
/// bounds on the size of its output
class SOURCEMETA_CORE_MARKDOWN_EXPORT MarkdownError : public Error {
public:
  /// Construct an error with the given message
  MarkdownError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
