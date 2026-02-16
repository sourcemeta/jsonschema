#ifndef SOURCEMETA_BLAZE_LINTER_ERROR_H_
#define SOURCEMETA_BLAZE_LINTER_ERROR_H_

#ifndef SOURCEMETA_BLAZE_LINTER_EXPORT
#include <sourcemeta/blaze/linter_export.h>
#endif

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup linter
/// An error that represents an invalid schema rule name. The name must
/// consist only of lowercase ASCII letters, digits, underscores, or slashes.
class SOURCEMETA_BLAZE_LINTER_EXPORT LinterInvalidNameError
    : public std::exception {
public:
  LinterInvalidNameError(const std::string_view identifier,
                         const std::string_view message)
      : identifier_{identifier}, message_{message} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto identifier() const noexcept -> const std::string & {
    return this->identifier_;
  }

private:
  std::string identifier_;
  std::string message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
