#ifndef SOURCEMETA_CORE_MARKDOWN_ERROR_H_
#define SOURCEMETA_CORE_MARKDOWN_ERROR_H_

#ifndef SOURCEMETA_CORE_MARKDOWN_EXPORT
#include <sourcemeta/core/markdown_export.h>
#endif

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup markdown
/// An error that represents a Markdown conversion that would exceed one of the
/// bounds on the size of its output
class SOURCEMETA_CORE_MARKDOWN_EXPORT MarkdownError : public std::exception {
public:
  /// Construct an error with the given message
  MarkdownError(const char *message) : message_{message} {}
  MarkdownError(std::string message) = delete;
  MarkdownError(std::string &&message) = delete;
  MarkdownError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
