#ifndef SOURCEMETA_CORE_SCHEMACONFIG_ERROR_H_
#define SOURCEMETA_CORE_SCHEMACONFIG_ERROR_H_

#ifndef SOURCEMETA_CORE_SCHEMACONFIG_EXPORT
#include <sourcemeta/core/schemaconfig_export.h>
#endif

#include <sourcemeta/core/jsonpointer.h>

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

/// @ingroup schemaconfig
class SOURCEMETA_CORE_SCHEMACONFIG_EXPORT SchemaConfigParseError
    : public std::exception {
public:
  SchemaConfigParseError(const char *message, Pointer location)
      : message_{message}, location_{std::move(location)} {}
  SchemaConfigParseError(std::string message, Pointer location) = delete;
  SchemaConfigParseError(std::string &&message, Pointer location) = delete;
  SchemaConfigParseError(std::string_view message, Pointer location) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  [[nodiscard]] auto location() const noexcept -> const auto & {
    return this->location_;
  }

private:
  const char *message_;
  Pointer location_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
