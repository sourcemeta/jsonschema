#ifndef SOURCEMETA_CORE_YAML_ERROR_H_
#define SOURCEMETA_CORE_YAML_ERROR_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <exception> // std::exception
#include <string>    // std::string
#include <utility>   // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup yaml
/// An error that represents a general YAML error event
class SOURCEMETA_CORE_YAML_EXPORT YAMLError : public std::exception {
public:
  YAMLError(std::string message) : message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  std::string message_;
};

/// @ingroup yaml
/// An error that represents YAML parse error event
class SOURCEMETA_CORE_YAML_EXPORT YAMLParseError : public std::exception {
public:
  YAMLParseError(std::string message) : message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  std::string message_;
};

/// @ingroup yaml
/// An error that represents an unknown anchor reference in YAML
class SOURCEMETA_CORE_YAML_EXPORT YAMLUnknownAnchorError
    : public YAMLParseError {
public:
  YAMLUnknownAnchorError(std::string anchor_name)
      : YAMLParseError{"YAML alias references undefined anchor"},
        anchor_name_{std::move(anchor_name)} {}

  [[nodiscard]] auto anchor() const noexcept -> const std::string & {
    return this->anchor_name_;
  }

private:
  std::string anchor_name_;
};

} // namespace sourcemeta::core

#endif
