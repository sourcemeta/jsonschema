#ifndef SOURCEMETA_CORE_JSON_ERROR_H_
#define SOURCEMETA_CORE_JSON_ERROR_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

#include <cstdint>    // std::uint64_t
#include <exception>  // std::exception
#include <filesystem> // std::filesystem::path

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup json
/// This class represents a parsing error
class SOURCEMETA_CORE_JSON_EXPORT ParseError : public std::exception {
public:
  /// Create a parsing error
  ParseError(const std::uint64_t line, const std::uint64_t column)
      : line_{line}, column_{column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "Failed to parse the JSON document";
  }

  /// Get the line number of the error
  [[nodiscard]] auto line() const noexcept -> std::uint64_t { return line_; }

  // Get the column number of the error
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return column_;
  }

private:
  std::uint64_t line_;
  std::uint64_t column_;
};

/// @ingroup json
/// This class represents a parsing error occurring from parsing a file
class SOURCEMETA_CORE_JSON_EXPORT FileParseError : public ParseError {
public:
  /// Create a file parsing error
  FileParseError(const std::filesystem::path &path, const std::uint64_t line,
                 const std::uint64_t column)
      : ParseError{line, column}, path_{path} {}

  /// Create a file parsing error from a parse error
  FileParseError(const std::filesystem::path &path, const ParseError &parent)
      : ParseError{parent.line(), parent.column()}, path_{path} {}

  /// Get the fiel path of the error
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path {
    return path_;
  }

private:
  std::filesystem::path path_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
