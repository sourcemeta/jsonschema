#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_ERROR_H
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_ERROR_H

#include "jsonschema_export.h"

#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/uri.h>

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

/// @ingroup jsonschema
/// An error that represents a schema vocabulary error
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaVocabularyError
    : public std::exception {
public:
  SchemaVocabularyError(std::string uri, std::string message)
      : uri_{std::move(uri)}, message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto uri() const noexcept -> std::string_view {
    return this->uri_;
  }

private:
  std::string uri_;
  std::string message_;
};

/// @ingroup jsonschema
/// An error that represents a schema resolution failure event
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaReferenceError
    : public std::exception {
public:
  SchemaReferenceError(std::string identifier, const Pointer &schema_location,
                       std::string message)
      : identifier_{std::move(identifier)}, schema_location_{schema_location},
        message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto id() const noexcept -> std::string_view {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->schema_location_;
  }

private:
  std::string identifier_;
  Pointer schema_location_;
  std::string message_;
};

/// @ingroup jsonschema
/// An error that represents a schema compilation failure event
class SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT SchemaCompilationError
    : public std::exception {
public:
  SchemaCompilationError(const URI &base, const Pointer &schema_location,
                         std::string message)
      : base_{base}, schema_location_{schema_location},
        message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto base() const noexcept -> const URI & {
    return this->base_;
  }

  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->schema_location_;
  }

private:
  URI base_;
  Pointer schema_location_;
  std::string message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::jsontoolkit

#endif
