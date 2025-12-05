#ifndef SOURCEMETA_CORE_PUNYCODE_ERROR_H_
#define SOURCEMETA_CORE_PUNYCODE_ERROR_H_

#ifndef SOURCEMETA_CORE_PUNYCODE_EXPORT
#include <sourcemeta/core/punycode_error.h>
#endif

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup punycode
class SOURCEMETA_CORE_PUNYCODE_EXPORT PunycodeError : public std::exception {
public:
  PunycodeError(const char *message) : message_{message} {}
  PunycodeError(std::string message) = delete;
  PunycodeError(std::string &&message) = delete;
  PunycodeError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
