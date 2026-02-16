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
  LinterInvalidNameError(const std::string_view name) : name_{name} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The schema rule name is invalid";
  }

  [[nodiscard]] auto name() const noexcept -> const std::string & {
    return this->name_;
  }

private:
  std::string name_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
