#ifndef SOURCEMETA_BLAZE_CONFIGURATION_ERROR_H_
#define SOURCEMETA_BLAZE_CONFIGURATION_ERROR_H_

#ifndef SOURCEMETA_BLAZE_CONFIGURATION_EXPORT
#include <sourcemeta/blaze/configuration_export.h>
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
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup configuration
class SOURCEMETA_BLAZE_CONFIGURATION_EXPORT ConfigurationParseError
    : public std::exception {
public:
  ConfigurationParseError(const char *message,
                          sourcemeta::core::Pointer location)
      : message_{message}, location_{std::move(location)} {}
  ConfigurationParseError(std::string message,
                          sourcemeta::core::Pointer location) = delete;
  ConfigurationParseError(std::string &&message,
                          sourcemeta::core::Pointer location) = delete;
  ConfigurationParseError(std::string_view message,
                          sourcemeta::core::Pointer location) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto location() const noexcept -> const auto & {
    return this->location_;
  }

private:
  const char *message_;
  sourcemeta::core::Pointer location_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
