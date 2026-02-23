#ifndef SOURCEMETA_CORE_OPTIONS_ERROR_H_
#define SOURCEMETA_CORE_OPTIONS_ERROR_H_

#ifndef SOURCEMETA_CORE_OPTIONS_EXPORT
#include <sourcemeta/core/options_export.h>
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

/// @ingroup options
/// This class represents a general options error
class SOURCEMETA_CORE_OPTIONS_EXPORT OptionsError : public std::exception {
public:
  explicit OptionsError(const char *message) : message_{message} {}
  explicit OptionsError(std::string message) = delete;
  explicit OptionsError(std::string &&message) = delete;
  explicit OptionsError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

/// @ingroup options
/// This class represents a unknown option error
struct SOURCEMETA_CORE_OPTIONS_EXPORT OptionsUnknownOptionError
    : public OptionsError {
  explicit OptionsUnknownOptionError(const std::string_view option)
      : OptionsError{"Unknown option"}, option_{option} {}
  [[nodiscard]] auto option() const noexcept -> std::string_view {
    return this->option_;
  }

private:
  std::string option_;
};

/// @ingroup options
/// This class represents a value being passed to a flag
struct SOURCEMETA_CORE_OPTIONS_EXPORT OptionsUnexpectedValueFlagError
    : public OptionsError {
  explicit OptionsUnexpectedValueFlagError(const std::string_view option)
      : OptionsError{"This flag cannot take a value"}, option_{option} {}
  [[nodiscard]] auto option() const noexcept -> std::string_view {
    return this->option_;
  }

private:
  std::string option_;
};

/// @ingroup options
/// This class represents a missing value from an option
struct SOURCEMETA_CORE_OPTIONS_EXPORT OptionsMissingOptionValueError
    : public OptionsError {
  explicit OptionsMissingOptionValueError(const std::string_view option)
      : OptionsError{"This option must take a value"}, option_{option} {}
  [[nodiscard]] auto option() const noexcept -> std::string_view {
    return this->option_;
  }

private:
  std::string option_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
