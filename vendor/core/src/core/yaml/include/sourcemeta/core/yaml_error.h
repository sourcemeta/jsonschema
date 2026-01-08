#ifndef SOURCEMETA_CORE_YAML_ERROR_H_
#define SOURCEMETA_CORE_YAML_ERROR_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

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
  YAMLError(const char *message) : message_{message} {}
  YAMLError(std::string message) = delete;
  YAMLError(std::string &&message) = delete;
  YAMLError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

/// @ingroup yaml
/// An error that represents YAML parse error event
class SOURCEMETA_CORE_YAML_EXPORT YAMLParseError : public std::exception {
public:
  YAMLParseError(const char *message) : message_{message} {}
  YAMLParseError(std::string message) = delete;
  YAMLParseError(std::string &&message) = delete;
  YAMLParseError(std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

private:
  const char *message_;
};

/// @ingroup yaml
/// An error that represents an unknown anchor reference in YAML
class SOURCEMETA_CORE_YAML_EXPORT YAMLUnknownAnchorError
    : public YAMLParseError {
public:
  YAMLUnknownAnchorError(const std::string_view anchor_name)
      : YAMLParseError{"YAML alias references undefined anchor"},
        anchor_name_{anchor_name} {}

  [[nodiscard]] auto anchor() const noexcept -> std::string_view {
    return this->anchor_name_;
  }

private:
  std::string anchor_name_;
};

} // namespace sourcemeta::core

#endif
