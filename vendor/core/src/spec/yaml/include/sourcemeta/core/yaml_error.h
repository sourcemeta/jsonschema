#ifndef SOURCEMETA_CORE_YAML_ERROR_H_
#define SOURCEMETA_CORE_YAML_ERROR_H_

#ifndef SOURCEMETA_CORE_YAML_EXPORT
#include <sourcemeta/core/yaml_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint>     // std::uint64_t
#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup yaml
/// An error that represents a general YAML error event
class SOURCEMETA_CORE_YAML_EXPORT YAMLError : public Error {
public:
  /// Create a general error
  YAMLError(const StaticString message) noexcept : Error{message} {}
};

/// @ingroup yaml
/// An error that represents a YAML parse error event
class SOURCEMETA_CORE_YAML_EXPORT YAMLParseError : public Error {
public:
  /// Create a parsing error
  YAMLParseError(const std::uint64_t line, const std::uint64_t column) noexcept
      : Error{"Failed to parse the YAML document"}, line_{line},
        column_{column} {}

  /// Create a parsing error with a custom message
  YAMLParseError(const StaticString message, const std::uint64_t line,
                 const std::uint64_t column) noexcept
      : Error{message}, line_{line}, column_{column} {}

  /// Get the line number of the error
  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    return this->line_;
  }

  /// Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t line_;
  std::uint64_t column_;
};

/// @ingroup yaml
/// An error that represents a YAML parse error occurring from parsing a file
class SOURCEMETA_CORE_YAML_EXPORT YAMLFileParseError : public YAMLParseError {
public:
  /// Create a file parsing error
  YAMLFileParseError(const StaticString message, std::filesystem::path path,
                     const std::uint64_t line, const std::uint64_t column)
      : YAMLParseError{message, line, column}, path_{std::move(path)} {}

  /// Create a file parsing error from a parse error
  YAMLFileParseError(std::filesystem::path path, const YAMLParseError &parent)
      : YAMLParseError{parent.message(), parent.line(), parent.column()},
        path_{std::move(path)} {}

  /// Get the file path of the error
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup yaml
/// An error that represents an unknown anchor reference in YAML
class SOURCEMETA_CORE_YAML_EXPORT YAMLUnknownAnchorError
    : public YAMLParseError {
public:
  /// Create an unknown anchor error
  YAMLUnknownAnchorError(const std::string_view anchor_name,
                         const std::uint64_t line, const std::uint64_t column)
      : YAMLParseError{"YAML alias references undefined anchor", line, column},
        anchor_name_{anchor_name} {}

  /// Get the anchor name of the error
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
  /// Create a duplicate key error
  YAMLDuplicateKeyError(const std::string_view key_name,
                        const std::uint64_t line, const std::uint64_t column)
      : YAMLParseError{"Duplicate key in YAML mapping", line, column},
        key_name_{key_name} {}

  /// Get the key name of the error
  [[nodiscard]] auto key() const noexcept -> std::string_view {
    return this->key_name_;
  }

private:
  std::string key_name_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
