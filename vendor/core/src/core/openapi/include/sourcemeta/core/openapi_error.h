#ifndef SOURCEMETA_CORE_OPENAPI_ERROR_H_
#define SOURCEMETA_CORE_OPENAPI_ERROR_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/json.h>
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
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup openapi
/// An error that represents an OpenAPI Description that does not conform to
/// the specification. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const sourcemeta::core::OpenAPIError error{
///     sourcemeta::core::Pointer{"info"}, "The Info Object is required"};
/// assert(error.location() == sourcemeta::core::Pointer{"info"});
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIError : public std::exception {
public:
  /// Construct an error from where the problem is and a message
  OpenAPIError(Pointer location, const char *message)
      : location_{std::move(location)}, message_{message} {}
  OpenAPIError(Pointer location, std::string message) = delete;
  OpenAPIError(Pointer location, std::string &&message) = delete;
  OpenAPIError(Pointer location, std::string_view message) = delete;

  /// Construct an error that names the document the problem is in, which is
  /// what a caller that framed more than one of them tells them apart by
  OpenAPIError(JSON::String base, Pointer location, const char *message)
      : base_{std::move(base)}, location_{std::move(location)},
        message_{message} {}
  OpenAPIError(JSON::String base, Pointer location,
               std::string message) = delete;
  OpenAPIError(JSON::String base, Pointer location,
               std::string &&message) = delete;
  OpenAPIError(JSON::String base, Pointer location,
               std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// Get where the problem is, as a pointer from the root of the document
  /// that holds it
  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->location_;
  }

  /// Get the base URI of the document that holds the problem, or the empty
  /// URI reference when the caller established none
  [[nodiscard]] auto base() const noexcept -> JSON::StringView {
    return this->base_;
  }

private:
  JSON::String base_;
  Pointer location_;
  const char *message_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
