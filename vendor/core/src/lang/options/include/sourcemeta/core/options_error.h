#ifndef SOURCEMETA_CORE_OPTIONS_ERROR_H_
#define SOURCEMETA_CORE_OPTIONS_ERROR_H_

#ifndef SOURCEMETA_CORE_OPTIONS_EXPORT
#include <sourcemeta/core/options_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup options
/// This class represents a general options error
class SOURCEMETA_CORE_OPTIONS_EXPORT OptionsError : public Error {
public:
  /// Construct the error given a message.
  OptionsError(const StaticString message) noexcept : Error{message} {}
};

/// @ingroup options
/// This class represents a unknown option error
class SOURCEMETA_CORE_OPTIONS_EXPORT OptionsUnknownOptionError
    : public OptionsError {
public:
  /// Construct the error given the offending option.
  OptionsUnknownOptionError(const std::string_view option)
      : OptionsError{"Unknown option"}, option_{option} {}

  /// The offending option.
  [[nodiscard]] auto option() const noexcept -> std::string_view {
    return this->option_;
  }

private:
  std::string option_;
};

/// @ingroup options
/// This class represents a value being passed to a flag
class SOURCEMETA_CORE_OPTIONS_EXPORT OptionsUnexpectedValueFlagError
    : public OptionsError {
public:
  /// Construct the error given the offending option.
  OptionsUnexpectedValueFlagError(const std::string_view option)
      : OptionsError{"This flag cannot take a value"}, option_{option} {}

  /// The offending option.
  [[nodiscard]] auto option() const noexcept -> std::string_view {
    return this->option_;
  }

private:
  std::string option_;
};

/// @ingroup options
/// This class represents a missing value from an option
class SOURCEMETA_CORE_OPTIONS_EXPORT OptionsMissingOptionValueError
    : public OptionsError {
public:
  /// Construct the error given the offending option.
  OptionsMissingOptionValueError(const std::string_view option)
      : OptionsError{"This option must take a value"}, option_{option} {}

  /// The offending option.
  [[nodiscard]] auto option() const noexcept -> std::string_view {
    return this->option_;
  }

private:
  std::string option_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
