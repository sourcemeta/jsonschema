#ifndef SOURCEMETA_BLAZE_COMPILER_ERROR_H
#define SOURCEMETA_BLAZE_COMPILER_ERROR_H

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/uri.h>

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

/// @ingroup jsonschema
/// An error that represents a schema compilation failure event
class SOURCEMETA_BLAZE_COMPILER_EXPORT CompilerError : public std::exception {
public:
  CompilerError(sourcemeta::core::URI base,
                sourcemeta::core::Pointer schema_location, std::string message)
      : base_{std::move(base)}, schema_location_{std::move(schema_location)},
        message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto base() const noexcept -> const sourcemeta::core::URI & {
    return this->base_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->schema_location_;
  }

private:
  sourcemeta::core::URI base_;
  sourcemeta::core::Pointer schema_location_;
  std::string message_;
};

/// @ingroup jsonschema
/// An error that represents a reference target that is not a valid schema
class SOURCEMETA_BLAZE_COMPILER_EXPORT CompilerReferenceTargetNotSchemaError
    : public std::exception {
public:
  CompilerReferenceTargetNotSchemaError(
      const std::string_view identifier,
      sourcemeta::core::Pointer schema_location)
      : identifier_{identifier}, schema_location_{std::move(schema_location)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The referenced schema is not considered to be a valid subschema "
           "given the dialect and vocabularies in use";
  }

  [[nodiscard]] auto identifier() const noexcept -> const std::string & {
    return this->identifier_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->schema_location_;
  }

private:
  std::string identifier_;
  sourcemeta::core::Pointer schema_location_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
