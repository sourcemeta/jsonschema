#ifndef SOURCEMETA_CORE_SEMVER_ERROR_H_
#define SOURCEMETA_CORE_SEMVER_ERROR_H_

#ifndef SOURCEMETA_CORE_SEMVER_EXPORT
#include <sourcemeta/core/semver_export.h>
#endif

#include <cstdint>   // std::uint64_t
#include <exception> // std::exception

namespace sourcemeta::core {

#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// An error raised when a string cannot be parsed as a Semantic Version.
class SOURCEMETA_CORE_SEMVER_EXPORT SemVerParseError : public std::exception {
public:
  /// Construct an error located at the given column of the input.
  SemVerParseError(const std::uint64_t column) : column_{column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The input is not a valid Semantic Version";
  }

  /// The column at which the parsing failure occurred.
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

/// An error raised when a numeric component of a Semantic Version overflows.
class SOURCEMETA_CORE_SEMVER_EXPORT SemVerOverflowError
    : public std::exception {
public:
  /// Construct an error located at the given column of the input.
  SemVerOverflowError(const std::uint64_t column) : column_{column} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The numeric component of the Semantic Version overflows";
  }

  /// The column at which the overflow occurred.
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
