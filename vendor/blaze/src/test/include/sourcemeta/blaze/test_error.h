#ifndef SOURCEMETA_BLAZE_TEST_ERROR_H_
#define SOURCEMETA_BLAZE_TEST_ERROR_H_

#ifndef SOURCEMETA_BLAZE_TEST_EXPORT
#include <sourcemeta/blaze/test_export.h>
#endif

#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>     // std::uint64_t
#include <exception>   // std::exception
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::blaze {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup test
/// An error that occurs when parsing a test file
class SOURCEMETA_BLAZE_TEST_EXPORT TestParseError : public std::exception {
public:
  TestParseError(const char *message, sourcemeta::core::Pointer location,
                 std::uint64_t line, std::uint64_t column)
      : message_{message}, location_{std::move(location)}, line_{line},
        column_{column} {}
  TestParseError(std::string message, sourcemeta::core::Pointer location,
                 std::uint64_t line, std::uint64_t column) = delete;
  TestParseError(std::string &&message, sourcemeta::core::Pointer location,
                 std::uint64_t line, std::uint64_t column) = delete;
  TestParseError(std::string_view message, sourcemeta::core::Pointer location,
                 std::uint64_t line, std::uint64_t column) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    return this->line_;
  }

  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  const char *message_;
  sourcemeta::core::Pointer location_;
  std::uint64_t line_;
  std::uint64_t column_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
