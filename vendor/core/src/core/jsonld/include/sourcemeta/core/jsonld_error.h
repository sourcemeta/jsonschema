#ifndef SOURCEMETA_CORE_JSONLD_ERROR_H_
#define SOURCEMETA_CORE_JSONLD_ERROR_H_

#ifndef SOURCEMETA_CORE_JSONLD_EXPORT
#include <sourcemeta/core/jsonld_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <exception>        // std::exception
#include <initializer_list> // std::initializer_list
#include <utility>          // std::move

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup jsonld
/// An error that represents a JSON-LD processing failure. The message is one of
/// the error codes defined by the JSON-LD 1.1 API specification, and the
/// pointer locates the offending position in the input document
class SOURCEMETA_CORE_JSONLD_EXPORT JSONLDError : public std::exception {
public:
  /// Locate the error at an owned pointer.
  JSONLDError(const char *code, Pointer pointer)
      : code_{code}, pointer_{std::move(pointer)} {}

  /// Locate the error at a weak pointer, materialising an owned pointer.
  JSONLDError(const char *code, const WeakPointer &pointer)
      : code_{code}, pointer_{to_pointer(pointer)} {}

  /// Locate the error at a weak pointer extended with the given trailing
  /// property tokens.
  JSONLDError(const char *code, const WeakPointer &pointer,
              const std::initializer_list<JSON::StringView> children)
      : code_{code}, pointer_{to_pointer(pointer)} {
    for (const auto child : children) {
      this->pointer_.push_back(JSON::String{child});
    }
  }

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->code_.c_str();
  }

  /// Get the JSON Pointer to the position in the input document that caused the
  /// error
  [[nodiscard]] auto pointer() const noexcept -> const Pointer & {
    return this->pointer_;
  }

private:
  JSON::String code_;
  Pointer pointer_;
};

#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif

} // namespace sourcemeta::core

#endif
