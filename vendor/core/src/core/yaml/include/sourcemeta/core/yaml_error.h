#ifndef SOURCEMETA_CORE_YAML_ERROR_H_
#define SOURCEMETA_CORE_YAML_ERROR_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <cstdint>     // std::uint64_t
#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe
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
/// An error that represents a YAML parse error event
class SOURCEMETA_CORE_YAML_EXPORT YAMLParseError : public std::exception {
public:
  YAMLParseError(const std::uint64_t line, const std::uint64_t column)
      : line_{line}, column_{column},
        message_{"Failed to parse the YAML document"} {}

  YAMLParseError(const std::uint64_t line, const std::uint64_t column,
                 const char *message)
      : line_{line}, column_{column}, message_{message} {}
  YAMLParseError(const std::uint64_t line, const std::uint64_t column,
                 std::string message) = delete;
  YAMLParseError(const std::uint64_t line, const std::uint64_t column,
                 std::string &&message) = delete;
  YAMLParseError(const std::uint64_t line, const std::uint64_t column,
                 std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    return this->line_;
  }

  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t line_;
  std::uint64_t column_;
  const char *message_;
};

/// @ingroup yaml
/// An error that represents an unknown anchor reference in YAML
class SOURCEMETA_CORE_YAML_EXPORT YAMLUnknownAnchorError
    : public YAMLParseError {
public:
  YAMLUnknownAnchorError(const std::string_view anchor_name,
                         const std::uint64_t line, const std::uint64_t column)
      : YAMLParseError{line, column, "YAML alias references undefined anchor"},
        anchor_name_{anchor_name} {}

  [[nodiscard]] auto anchor() const noexcept -> std::string_view {
    return this->anchor_name_;
  }

private:
  std::string anchor_name_;
};

/// @ingroup yaml
/// An error that represents a duplicate key in a YAML mapping
/// YAML 1.2.2 requires unique keys in mappings, unlike JSON where duplicate
/// keys are undefined behavior. See https://yaml.org/spec/1.2.2/#mapping
class SOURCEMETA_CORE_YAML_EXPORT YAMLDuplicateKeyError
    : public YAMLParseError {
public:
  YAMLDuplicateKeyError(const std::string_view key_name,
                        const std::uint64_t line, const std::uint64_t column)
      : YAMLParseError{line, column, "Duplicate key in YAML mapping"},
        key_name_{key_name} {}

  [[nodiscard]] auto key() const noexcept -> std::string_view {
    return this->key_name_;
  }

private:
  std::string key_name_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
