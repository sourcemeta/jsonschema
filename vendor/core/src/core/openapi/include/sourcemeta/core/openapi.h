#ifndef SOURCEMETA_CORE_OPENAPI_H_
#define SOURCEMETA_CORE_OPENAPI_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/openapi_error.h>
// NOLINTEND(misc-include-cleaner)

#include <cstdint>     // std::uint8_t, std::uint64_t
#include <limits>      // std::numeric_limits
#include <memory>      // std::unique_ptr
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

/// @defgroup openapi OpenAPI
/// @brief A growing implementation of the OpenAPI Specification 3.1.
///
/// This module reports where an OpenAPI Description declares its JSON Schemas
/// and leaves what is inside them to a JSON Schema implementation.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// ```

namespace sourcemeta::core {

/// @ingroup openapi
/// The OpenAPI Description versions that this module recognises
enum class OpenAPIVersion : std::uint8_t {
  /// The OpenAPI Specification 3.1 revision
  OPENAPI_3_1,
  /// The OpenAPI Specification 3.2 revision
  OPENAPI_3_2
};

/// @ingroup openapi
/// Determine the version of an OpenAPI Description from its `openapi` field
/// without framing it, returning no value for a version we do not recognise.
/// The patch component of the field carries no meaning, so every `3.1.x`
/// release maps to the same result. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"({
///   "openapi": "3.1.1",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {}
/// })")};
///
/// assert(sourcemeta::core::openapi_version(document).value() ==
///        sourcemeta::core::OpenAPIVersion::OPENAPI_3_1);
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_version(const JSON &document) -> std::optional<OpenAPIVersion>;

/// @ingroup openapi
/// The contact information that an OpenAPI Description declares for the API.
/// Every value borrows from the document it was read from, so that document
/// must outlive this
struct OpenAPIContact {
  /// The identifying name of the contact person or organisation
  std::optional<JSON::StringView> name{std::nullopt};
  /// Where to find the contact information, as a URI reference
  std::optional<JSON::StringView> url{std::nullopt};
  /// The email address of the contact person or organisation
  std::optional<JSON::StringView> email{std::nullopt};
};

/// @ingroup openapi
/// The license information that an OpenAPI Description declares for the API.
/// Every value borrows from the document it was read from, so that document
/// must outlive this
struct OpenAPILicense {
  /// The license name used for the API
  JSON::StringView name{};
  /// The SPDX license expression for the API, recorded as it was written, as
  /// the specification states no requirement on its syntax
  std::optional<JSON::StringView> identifier{std::nullopt};
  /// Where to find the license used for the API, as a URI reference
  std::optional<JSON::StringView> url{std::nullopt};
};

/// @ingroup openapi
/// The metadata that an OpenAPI Description declares about the API it
/// describes. Every value borrows from the document it was read from, so that
/// document must outlive this
struct OpenAPIInfo {
  /// The title of the API
  JSON::StringView title{};
  /// The version of the document, which is unrelated to the version of the
  /// OpenAPI Specification that it declares
  JSON::StringView version{};
  /// A short summary of the API
  std::optional<JSON::StringView> summary{std::nullopt};
  /// A description of the API, which may be written in CommonMark
  std::optional<JSON::StringView> description{std::nullopt};
  /// Where to find the terms of service for the API, as a URI reference
  std::optional<JSON::StringView> terms_of_service{std::nullopt};
  /// The contact information for the API
  std::optional<OpenAPIContact> contact{std::nullopt};
  /// The license information for the API
  std::optional<OpenAPILicense> license{std::nullopt};
};

/// @ingroup openapi
/// A static analysis pass over an OpenAPI Description that computes the
/// locations it exposes, the references between them, the operations it
/// describes, and where its JSON Schemas begin. It does not look inside those
/// schemas. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <iostream>
///
/// const auto document{sourcemeta::core::parse_json(R"({
///   "openapi": "3.1.1",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {}
/// })")};
///
/// const sourcemeta::core::OpenAPIFrame frame{
///     document, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver};
/// sourcemeta::core::prettify(frame.to_json(), std::cout);
/// std::cout << std::endl;
/// ```
///
/// A frame is analysed once, on construction, and is immutable afterwards.
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIFrame {
public:
  /// Frame an OpenAPI Description from a given document. That document must
  /// outlive the frame, as the metadata it reports borrows from it. The given
  /// base need not, as the frame canonicalises it into a string of its own
  ///
  /// The base is the retrieval URI of the document. OpenAPI 3.1 offers a
  /// document no way of declaring an identity of its own, so under that
  /// revision this is the only way to give the description one. From 3.2
  /// onwards a document may declare `$self`, which takes precedence and is
  /// resolved against this when relative
  ///
  /// Only the given document is read. A reference that leaves it is recorded
  /// and left there, and a frame holding one of those does not stand alone
  ///
  /// The walker and the resolver are what reading inside a Schema Object
  /// takes, as a Schema Object is JSON Schema's to make sense of rather than
  /// this specification's. Neither is defaulted, as which dialects a
  /// description may be written against is the caller's to state: pass
  /// sourcemeta::core::schema_walker and
  /// sourcemeta::core::schema_resolver for the dialects that are published,
  /// and a resolver of your own for one that is not
  ///
  /// A document that does not conform to the specification is rejected here
  /// rather than reported back
  OpenAPIFrame(
      const JSON &document, const SchemaWalker &walker,
      const SchemaResolver &resolver, std::string_view default_base = "",
      std::uint64_t max_locations = std::numeric_limits<std::uint64_t>::max());

  ~OpenAPIFrame();

  // We rely on internal caches that would be dangling otherwise
  OpenAPIFrame(const OpenAPIFrame &) = delete;
  auto operator=(const OpenAPIFrame &) -> OpenAPIFrame & = delete;
  OpenAPIFrame(OpenAPIFrame &&) = delete;
  auto operator=(OpenAPIFrame &&) -> OpenAPIFrame & = delete;

  /// Get the version of the OpenAPI Specification that the entry document
  /// declares. The patch component of that declaration carries no meaning, so
  /// every `3.1.x` release reports the same version. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <cassert>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "paths": {}
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver};
  /// assert(frame.version() ==
  ///        sourcemeta::core::OpenAPIVersion::OPENAPI_3_1);
  /// ```
  [[nodiscard]] auto version() const noexcept -> OpenAPIVersion;

  /// Get the metadata that the entry document declares about the API. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <cassert>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "paths": {}
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver};
  /// assert(frame.info().title == "Example");
  /// assert(frame.info().version == "1.0.0");
  /// assert(!frame.info().license.has_value());
  /// ```
  [[nodiscard]] auto info() const noexcept -> const OpenAPIInfo &;

  /// Get the base URI that relative references in the entry document resolve
  /// against, canonicalised, or the empty URI reference when nothing
  /// established one, which leaves those references relative. It is the
  /// `$self` the entry document declares, and the retrieval URI the caller
  /// supplied when it declares none. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <cassert>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "paths": {}
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver,
  ///     "https://example.com/openapi.json"};
  /// assert(frame.base() == "https://example.com/openapi.json");
  /// ```
  [[nodiscard]] auto base() const noexcept -> JSON::StringView;

  /// Check whether everything this description references is inside what was
  /// framed, which counts what its Schema Objects reference as much as what
  /// the shell around them does. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <cassert>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "paths": {}
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver};
  /// assert(frame.standalone());
  /// ```
  [[nodiscard]] auto standalone() const noexcept -> bool;

  /// Get the frame of every Schema Object the description holds, which a
  /// `schema` location names its part of by key. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <cassert>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "components": { "schemas": { "Pet": { "type": "object" } } }
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver,
  ///     "https://example.com/openapi.json"};
  ///
  /// assert(frame.schemas()
  ///            .location(sourcemeta::core::SchemaReferenceType::Static,
  ///                      "https://example.com/openapi.json"
  ///                      "#/components/schemas/Pet")
  ///            .has_value());
  /// ```
  [[nodiscard]] auto schemas() const noexcept -> const SchemaFrame &;

  /// Export the frame as JSON. This is the complete state of the frame, and
  /// for now its only window
  [[nodiscard]] auto to_json() const -> JSON;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  struct Internal;
  std::unique_ptr<Internal> internal_;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};

} // namespace sourcemeta::core

#endif
