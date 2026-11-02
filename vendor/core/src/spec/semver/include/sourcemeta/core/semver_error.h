#ifndef SOURCEMETA_CORE_SEMVER_ERROR_H_
#define SOURCEMETA_CORE_SEMVER_ERROR_H_

#ifndef SOURCEMETA_CORE_SEMVER_EXPORT
#include <sourcemeta/core/semver_export.h>
#endif

#include <sourcemeta/core/error.h>

#include <cstdint> // std::uint64_t

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// An error raised when a string cannot be parsed as a Semantic Version.
class SOURCEMETA_CORE_SEMVER_EXPORT SemVerParseError : public Error {
public:
  /// Construct an error located at the given column of the input.
  SemVerParseError(const std::uint64_t column) noexcept
      : Error{"The input is not a valid Semantic Version"}, column_{column} {}

  /// The column at which the parsing failure occurred.
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

/// An error raised when a numeric component of a Semantic Version overflows.
class SOURCEMETA_CORE_SEMVER_EXPORT SemVerOverflowError : public Error {
public:
  /// Construct an error located at the given column of the input.
  SemVerOverflowError(const std::uint64_t column) noexcept
      : Error{"The numeric component of the Semantic Version overflows"},
        column_{column} {}

  /// The column at which the overflow occurred.
  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t column_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
