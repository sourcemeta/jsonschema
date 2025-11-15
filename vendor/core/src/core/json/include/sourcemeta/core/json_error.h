#ifndef SOURCEMETA_CORE_JSON_ERROR_H_
#define SOURCEMETA_CORE_JSON_ERROR_H_

#ifndef SOURCEMETA_CORE_JSON_EXPORT
#include <sourcemeta/core/json_export.h>
#endif

#include <cstdint>    // std::uint64_t
#include <exception>  // std::exception
#include <filesystem> // std::filesystem::path
#include <string>     // std::string
#include <utility>    // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup json
/// This class represents a parsing error
class SOURCEMETA_CORE_JSON_EXPORT JSONParseError : public std::exception {
public:
  /// Create a parsing error
  JSONParseError(const std::uint64_t line, const std::uint64_t column)
      : line_{line}, column_{column} {}

  /// Create a parsing error with a custom error
  JSONParseError(const std::uint64_t line, const std::uint64_t column,
                 std::string message)
      : line_{line}, column_{column}, message_{std::move(message)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
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
  std::string message_{"Failed to parse the JSON document"};
};

/// @ingroup json
/// This class represents a parsing error occurring from parsing a file
class SOURCEMETA_CORE_JSON_EXPORT JSONFileParseError : public JSONParseError {
public:
  /// Create a file parsing error
  JSONFileParseError(std::filesystem::path path, const std::uint64_t line,
                     const std::uint64_t column, std::string message)
      : JSONParseError{line, column, std::move(message)},
        path_{std::move(path)} {}

  /// Create a file parsing error from a parse error
  JSONFileParseError(std::filesystem::path path, const JSONParseError &parent)
      : JSONParseError{parent.line(), parent.column(), parent.what()},
        path_{std::move(path)} {}

  /// Get the file path of the error
  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
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
