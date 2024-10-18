#ifndef SOURCEMETA_BLAZE_COMPILER_ERROR_H
#define SOURCEMETA_BLAZE_COMPILER_ERROR_H

#ifndef SOURCEMETA_BLAZE_COMPILER_EXPORT
#include <sourcemeta/blaze/compiler_export.h>
#endif

#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <exception> // std::exception
#include <string>    // std::string
#include <utility>   // std::move

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
  CompilerError(const sourcemeta::jsontoolkit::URI &base,
                const sourcemeta::jsontoolkit::Pointer &schema_location,
                std::string message)
      : base_{base}, schema_location_{schema_location},
        message_{std::move(message)} {}
  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_.c_str();
  }

  [[nodiscard]] auto base() const noexcept
      -> const sourcemeta::jsontoolkit::URI & {
    return this->base_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::jsontoolkit::Pointer & {
    return this->schema_location_;
  }

private:
  sourcemeta::jsontoolkit::URI base_;
  sourcemeta::jsontoolkit::Pointer schema_location_;
  std::string message_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::blaze

#endif
