#ifndef SOURCEMETA_CORE_OPENAPI_ERROR_H_
#define SOURCEMETA_CORE_OPENAPI_ERROR_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>     // std::uint64_t
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

/// @ingroup openapi
/// An error that represents a document of an OpenAPI Description that nothing
/// could produce. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const sourcemeta::core::OpenAPIResolutionError error{
///     "https://example.com/openapi.json", sourcemeta::core::Pointer{"$ref"},
///     "https://example.com/shared.json", "Could not resolve"};
/// assert(error.identifier() == "https://example.com/shared.json");
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIResolutionError
    : public std::exception {
public:
  /// Create a resolution error from the document the reference was made in,
  /// where in it the reference sits, what it names, and a message
  OpenAPIResolutionError(JSON::String base, Pointer location,
                         JSON::String identifier, const char *message)
      : base_{std::move(base)}, location_{std::move(location)},
        identifier_{std::move(identifier)}, message_{message} {}
  OpenAPIResolutionError(JSON::String base, Pointer location,
                         JSON::String identifier, std::string message) = delete;
  OpenAPIResolutionError(JSON::String base, Pointer location,
                         JSON::String identifier,
                         std::string &&message) = delete;
  OpenAPIResolutionError(JSON::String base, Pointer location,
                         JSON::String identifier,
                         std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// The URI that nothing could produce a document for
  [[nodiscard]] auto identifier() const noexcept -> JSON::StringView {
    return this->identifier_;
  }

  /// Get where the reference is, as a pointer from the root of the document
  /// that makes it
  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->location_;
  }

  /// Get the base URI of the document that makes the reference
  [[nodiscard]] auto base() const noexcept -> JSON::StringView {
    return this->base_;
  }

private:
  JSON::String base_;
  Pointer location_;
  JSON::String identifier_;
  const char *message_;
};

/// @ingroup openapi
/// An error that represents a reference of an OpenAPI Description that names
/// something it may not. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const sourcemeta::core::OpenAPIReferenceError error{
///     "https://example.com/openapi.json", sourcemeta::core::Pointer{"$ref"},
///     "https://example.com/shared.json#/info", "Wrong kind"};
/// assert(error.identifier() == "https://example.com/shared.json#/info");
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIReferenceError
    : public std::exception {
public:
  /// Create a reference error from the document the reference was made in,
  /// where in it the reference sits, what it names, and a message
  OpenAPIReferenceError(JSON::String base, Pointer location,
                        JSON::String identifier, const char *message)
      : base_{std::move(base)}, location_{std::move(location)},
        identifier_{std::move(identifier)}, message_{message} {}
  OpenAPIReferenceError(JSON::String base, Pointer location,
                        JSON::String identifier, std::string message) = delete;
  OpenAPIReferenceError(JSON::String base, Pointer location,
                        JSON::String identifier,
                        std::string &&message) = delete;
  OpenAPIReferenceError(JSON::String base, Pointer location,
                        JSON::String identifier,
                        std::string_view message) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// The URI that the reference names, resolved and canonicalised
  [[nodiscard]] auto identifier() const noexcept -> JSON::StringView {
    return this->identifier_;
  }

  /// Get where the reference is, as a pointer from the root of the document
  /// that makes it
  [[nodiscard]] auto location() const noexcept -> const Pointer & {
    return this->location_;
  }

  /// Get the base URI of the document that makes the reference
  [[nodiscard]] auto base() const noexcept -> JSON::StringView {
    return this->base_;
  }

private:
  JSON::String base_;
  Pointer location_;
  JSON::String identifier_;
  const char *message_;
};

/// @ingroup openapi
/// An error that represents framing that ran past what the caller allowed it
/// to register. For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const sourcemeta::core::OpenAPIFrameLimitError error{100};
/// assert(error.limit() == 100);
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIFrameLimitError
    : public std::exception {
public:
  /// Create a framing limit error
  OpenAPIFrameLimitError(const std::uint64_t limit) : limit_{limit} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The OpenAPI Description exceeds the maximum number of locations "
           "that framing may register";
  }

  /// The maximum number of locations that framing was allowed to register
  [[nodiscard]] auto limit() const noexcept -> std::uint64_t {
    return this->limit_;
  }

private:
  std::uint64_t limit_;
};

/// @ingroup openapi
/// An error that represents bundling that ran past what the caller allowed it
/// to analyse. For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const sourcemeta::core::OpenAPIBundleLimitError error{100};
/// assert(error.limit() == 100);
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIBundleLimitError
    : public std::exception {
public:
  /// Create a bundling limit error
  OpenAPIBundleLimitError(const std::uint64_t limit) : limit_{limit} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return "The OpenAPI Description exceeds the maximum number of locations "
           "that bundling may analyse";
  }

  /// The maximum number of locations that bundling was allowed to register
  [[nodiscard]] auto limit() const noexcept -> std::uint64_t {
    return this->limit_;
  }

private:
  std::uint64_t limit_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
