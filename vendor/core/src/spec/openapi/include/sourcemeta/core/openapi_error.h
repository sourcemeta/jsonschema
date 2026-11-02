#ifndef SOURCEMETA_CORE_OPENAPI_ERROR_H_
#define SOURCEMETA_CORE_OPENAPI_ERROR_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/error.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdint> // std::uint64_t
#include <utility> // std::move

namespace sourcemeta::core {

SOURCEMETA_CORE_ERROR_WARNINGS_PUSH

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
///     "The Info Object is required", sourcemeta::core::Pointer{"info"}};
/// assert(error.location() == sourcemeta::core::Pointer{"info"});
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIError : public Error {
public:
  /// Construct an error from a message and where the problem is
  OpenAPIError(const StaticString message, Pointer location)
      : Error{message}, location_{std::move(location)} {}

  /// Construct an error that names the document the problem is in, which is
  /// what a caller that framed more than one of them tells them apart by
  OpenAPIError(const StaticString message, JSON::String base, Pointer location)
      : Error{message}, base_{std::move(base)}, location_{std::move(location)} {
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
///     "Could not resolve", "https://example.com/openapi.json",
///     sourcemeta::core::Pointer{"$ref"}, "https://example.com/shared.json"};
/// assert(error.identifier() == "https://example.com/shared.json");
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIResolutionError : public Error {
public:
  /// Create a resolution error from a message, the document the reference was
  /// made in, where in it the reference sits, and what it names
  OpenAPIResolutionError(const StaticString message, JSON::String base,
                         Pointer location, JSON::String identifier)
      : Error{message}, base_{std::move(base)}, location_{std::move(location)},
        identifier_{std::move(identifier)} {}

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
///     "Wrong kind", "https://example.com/openapi.json",
///     sourcemeta::core::Pointer{"$ref"},
///     "https://example.com/shared.json#/info"};
/// assert(error.identifier() == "https://example.com/shared.json#/info");
/// ```
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIReferenceError : public Error {
public:
  /// Create a reference error from a message, the document the reference was
  /// made in, where in it the reference sits, and what it names
  OpenAPIReferenceError(const StaticString message, JSON::String base,
                        Pointer location, JSON::String identifier)
      : Error{message}, base_{std::move(base)}, location_{std::move(location)},
        identifier_{std::move(identifier)} {}

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
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIFrameLimitError : public Error {
public:
  /// Create a framing limit error
  OpenAPIFrameLimitError(const std::uint64_t limit) noexcept
      : Error{"The OpenAPI Description exceeds the maximum number of "
              "locations that framing may register"},
        limit_{limit} {}

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
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIBundleLimitError : public Error {
public:
  /// Create a bundling limit error
  OpenAPIBundleLimitError(const std::uint64_t limit) noexcept
      : Error{"The OpenAPI Description exceeds the maximum number of "
              "locations that bundling may analyse"},
        limit_{limit} {}

  /// The maximum number of locations that bundling was allowed to register
  [[nodiscard]] auto limit() const noexcept -> std::uint64_t {
    return this->limit_;
  }

private:
  std::uint64_t limit_;
};

SOURCEMETA_CORE_ERROR_WARNINGS_POP

} // namespace sourcemeta::core

#endif
