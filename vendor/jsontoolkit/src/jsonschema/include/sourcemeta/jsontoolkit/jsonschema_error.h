#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_ERROR_H
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_ERROR_H

#if defined(__EMSCRIPTEN__) || defined(__Unikraft__)
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#else
#include "jsonschema_export.h"
#endif

#include <exception> // std::exception
#include <string>    // std::string
#include <utility>   // std::move

namespace sourcemeta::jsontoolkit {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup jsonschema
/// An error that represents a general schema error event
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaError
    : public std::exception {
public:
  SchemaError(std::string message) : message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

private:
  std::string message_;
};

/// @ingroup jsonschema
/// An error that represents a schema resolution failure event
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaResolutionError
    : public std::exception {
public:
  SchemaResolutionError(std::string identifier, std::string message)
      : identifier_{std::move(identifier)}, message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto id() const noexcept -> std::string_view {
    return this->identifier_;
  }

private:
  std::string identifier_;
  std::string message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::jsontoolkit

#endif
