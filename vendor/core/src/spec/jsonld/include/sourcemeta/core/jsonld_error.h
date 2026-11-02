#ifndef SOURCEMETA_CORE_JSONLD_ERROR_H_
#define SOURCEMETA_CORE_JSONLD_ERROR_H_

#ifndef SOURCEMETA_CORE_JSONLD_EXPORT
#include <sourcemeta/core/jsonld_export.h>
#endif

#include <sourcemeta/core/error.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <initializer_list> // std::initializer_list
#include <utility>          // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

/// @ingroup jsonld
/// An error that represents a JSON-LD processing failure. The message is one of
/// the error codes defined by the JSON-LD 1.1 API specification, and the
/// pointer locates the offending position in the input document
class SOURCEMETA_CORE_JSONLD_EXPORT JSONLDError : public Error {
public:
  /// Locate the error at an owned pointer.
  JSONLDError(const StaticString code, Pointer pointer)
      : Error{code}, pointer_{std::move(pointer)} {}

  /// Locate the error at a weak pointer, materialising an owned pointer.
  JSONLDError(const StaticString code, const WeakPointer &pointer)
      : Error{code}, pointer_{to_pointer(pointer)} {}

  /// Locate the error at a weak pointer extended with the given trailing
  /// property tokens.
  JSONLDError(const StaticString code, const WeakPointer &pointer,
              const std::initializer_list<JSON::StringView> children)
      : Error{code}, pointer_{to_pointer(pointer)} {
    for (const auto child : children) {
      this->pointer_.push_back(JSON::String{child});
    }
  }

  /// Get the JSON Pointer to the position in the input document that caused the
  /// error
  [[nodiscard]] auto pointer() const noexcept -> const Pointer & {
    return this->pointer_;
  }

private:
  Pointer pointer_;
};

/// @ingroup jsonld
/// An error that represents a constants fragment grammar violation. The
/// message states the violated fragment rule, the key names the offending
/// fragment entry, and the pointer locates the offending value inside the
/// fragment
class SOURCEMETA_CORE_JSONLD_EXPORT JSONLDFragmentError : public JSONLDError {
public:
  /// Locate the error at a fragment entry.
  JSONLDFragmentError(const StaticString code, Pointer pointer,
                      JSON::String key)
      : JSONLDError{code, std::move(pointer)}, key_{std::move(key)} {}

  /// Get the key of the fragment entry that caused the error, empty when the
  /// fragment itself is malformed
  [[nodiscard]] auto key() const noexcept -> const JSON::String & {
    return this->key_;
  }

private:
  JSON::String key_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
