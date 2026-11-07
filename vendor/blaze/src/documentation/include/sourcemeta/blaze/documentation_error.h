#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_ERROR_H_
#define SOURCEMETA_BLAZE_DOCUMENTATION_ERROR_H_

#ifndef SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT
#include <sourcemeta/blaze/documentation_export.h>
#endif

#include <sourcemeta/core/jsonpointer.h>

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup documentation
///
/// A schema that cannot be described in the table format exactly, with the
/// place in the schema that cannot be described.
class SOURCEMETA_BLAZE_DOCUMENTATION_EXPORT DocumentationError
    : public std::exception {
public:
  /// The message is a fixed sentence, so it cannot be built out of whatever
  /// the schema happened to hold. What the sentence is about, such as the
  /// keyword at fault, is the subject.
  DocumentationError(const char *message, sourcemeta::core::Pointer location,
                     std::string subject = "")
      : message_{message}, location_{std::move(location)},
        subject_{std::move(subject)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// What the message is about, such as the keyword that cannot be described.
  /// Empty when the message speaks for itself.
  [[nodiscard]] auto subject() const noexcept -> std::string_view {
    return this->subject_;
  }

  /// Where in the schema the trouble is
  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  const char *message_;
  sourcemeta::core::Pointer location_;
  std::string subject_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::blaze

#endif
