#ifndef SOURCEMETA_CORE_JSON_ERROR_H_
#define SOURCEMETA_CORE_JSON_ERROR_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint>    // std::uint64_t
#include <filesystem> // std::filesystem::path
#include <utility>    // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup json
/// This class represents a parsing error
class SOURCEMETA_CORE_JSON_EXPORT JSONParseError : public Error {
public:
  /// Create a parsing error
  JSONParseError(const std::uint64_t line, const std::uint64_t column) noexcept
      : Error{"Failed to parse the JSON document"}, line_{line},
        column_{column} {}

  /// Create a parsing error with a custom message
  JSONParseError(const StaticString message, const std::uint64_t line,
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

/// @ingroup json
/// This class represents a parsing error occurring from parsing a file
class SOURCEMETA_CORE_JSON_EXPORT JSONFileParseError : public JSONParseError {
public:
  /// Create a file parsing error
  JSONFileParseError(const StaticString message, std::filesystem::path path,
                     const std::uint64_t line, const std::uint64_t column)
      : JSONParseError{message, line, column}, path_{std::move(path)} {}

  /// Create a file parsing error from a parse error
  JSONFileParseError(std::filesystem::path path, const JSONParseError &parent)
      : JSONParseError{parent.message(), parent.line(), parent.column()},
        path_{std::move(path)} {}

  /// Get the file path of the error
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

/// @ingroup json
/// This class represents an attempt to hold a number that JSON does not
/// define, as per ECMA-404
class SOURCEMETA_CORE_JSON_EXPORT JSONNonFiniteError : public Error {
public:
  JSONNonFiniteError() noexcept
      : Error{"JSON does not support Infinity or NaN"} {}
};

/// @ingroup json
/// This class represents a number that does not fit in the integer type the
/// caller asked for
class SOURCEMETA_CORE_JSON_EXPORT JSONIntegerRangeError : public Error {
public:
  /// Construct an error with the given message
  JSONIntegerRangeError(const StaticString message) noexcept : Error{message} {}
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
