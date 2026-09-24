#ifndef SOURCEMETA_CORE_OPENAPI_H_
#define SOURCEMETA_CORE_OPENAPI_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/openapi_error.h>
// NOLINTEND(misc-include-cleaner)

#include <cstdint>     // std::uint8_t, std::uint64_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <memory>      // std::unique_ptr
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

/// @defgroup openapi OpenAPI
/// @brief A growing implementation of the OpenAPI Specification.
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

/// @ingroup openapi
/// What a sourcemeta::core::OpenAPIResolver hands back: either a document it
/// owns, or a reference to one that outlives the call
using OpenAPIResolverResult = OwnedOrReference<JSON>;

/// @ingroup openapi
/// How bundling reaches the other documents that an OpenAPI Description is
/// split across. Every document handed back must itself be an OpenAPI
/// Description, and a URI that names nothing is reported by handing back no
/// value. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <string_view>
///
/// static auto resolver(const std::string_view identifier)
///     -> sourcemeta::core::OpenAPIResolverResult {
///   if (identifier == "https://example.com/shared.json") {
///     return sourcemeta::core::parse_json(R"JSON({
///       "openapi": "3.1.1",
///       "info": { "title": "Shared", "version": "1.0.0" },
///       "components": {}
///     })JSON");
///   }
///
///   return std::nullopt;
/// }
/// ```
using OpenAPIResolver = std::function<OpenAPIResolverResult(std::string_view)>;

/// @ingroup openapi
/// Everything bundling takes beyond the document and how to reach the rest
struct OpenAPIBundleOptions {
  /// A callback to report what bundling embedded, as the URI of the place it
  /// came from and a pointer from the root of the document it landed at.
  /// Bundling renames what it embeds to hold up as a component name, so this
  /// is the only way to know which place became which component
  using Callback =
      std::function<void(JSON::StringView, const sourcemeta::core::Pointer &)>;

  /// A callback to name what bundling embeds, given the URI of the place it
  /// came from and the Components Object member it goes under. Whatever it
  /// hands back is held to the keys that the specification admits and to
  /// being one the description does not already give a meaning to, so it is
  /// what bundling starts from rather than the last word
  using Namer = std::function<JSON::String(JSON::StringView, JSON::StringView)>;

  /// The URI the document was retrieved from, which every relative reference
  /// it makes resolves against. A document that names itself takes that name
  /// as its base instead, leaving this as the one a relative such name
  /// resolves against
  std::string_view default_base{};
  /// The maximum number of locations that analysis may register. How many
  /// documents bundling ends up reading follows from what the resolvers hand
  /// back rather than from the document the caller passed in, and every walk
  /// and every frame that bundling constructs spends from this one allowance,
  /// throwing sourcemeta::core::OpenAPIBundleLimitError once it runs out.
  ///
  /// Bundling settles by reading what it has produced so far over and over
  /// until a pass brings nothing new in, so this bounds the reading rather
  /// than the result. One place counts once per pass that goes by it and once
  /// more for each document brought in alongside it, which puts the allowance
  /// a whole description needs well above the number of places it holds. Note
  /// too that a document is read in full before anything charges for it, so
  /// this bounds how many oversized documents are read rather than whether
  /// one is
  std::uint64_t max_locations{std::numeric_limits<std::uint64_t>::max()};
  /// A callback to report each place that bundling embedded
  Callback callback{};
  /// A callback to name each place that bundling embeds
  Namer namer{};
};

/// @ingroup openapi
/// Bundle an OpenAPI Description by embedding everything it references from
/// another document into its own Components Object. The walker and the
/// resolver are what reading inside a Schema Object takes, and the OpenAPI
/// resolver is how the rest of the description is reached. No document the
/// description spans may declare a revision of the OpenAPI Specification other
/// than the one the entry document declares, as what this produces is one
/// document that declares one, and a revision neither holds every field of
/// another nor reads what they share by the same rules. This overload mutates
/// the input document. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
/// #include <string_view>
///
/// static auto resolver(const std::string_view identifier)
///     -> sourcemeta::core::OpenAPIResolverResult {
///   assert(identifier == "https://example.com/shared.json");
///   return sourcemeta::core::parse_json(R"JSON({
///     "openapi": "3.1.1",
///     "info": { "title": "Shared", "version": "1.0.0" },
///     "components": {
///       "responses": { "NotFound": { "description": "Not found" } }
///     }
///   })JSON");
/// }
///
/// auto document{sourcemeta::core::parse_json(R"JSON({
///   "openapi": "3.1.1",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {
///     "/pets": {
///       "get": {
///         "responses": {
///           "404": { "$ref": "shared.json#/components/responses/NotFound" }
///         }
///       }
///     }
///   }
/// })JSON")};
///
/// sourcemeta::core::openapi_bundle(
///     document, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver, resolver,
///     {.default_base = "https://example.com/openapi.json"});
///
/// assert(document.at("components").at("responses").defines("NotFound"));
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_bundle(JSON &document, const SchemaWalker &walker,
                    const SchemaResolver &schema_resolver,
                    const OpenAPIResolver &resolver,
                    const OpenAPIBundleOptions &options = {}) -> void;

/// @ingroup openapi
/// Bundle an OpenAPI Description by embedding everything it references from
/// another document into its own Components Object. No document the description
/// spans may declare a revision of the OpenAPI Specification other than the one
/// the entry document declares. This overload returns a new document, without
/// mutating the input. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
/// #include <string_view>
///
/// static auto resolver(const std::string_view identifier)
///     -> sourcemeta::core::OpenAPIResolverResult {
///   assert(identifier == "https://example.com/shared.json");
///   return sourcemeta::core::parse_json(R"JSON({
///     "openapi": "3.1.1",
///     "info": { "title": "Shared", "version": "1.0.0" },
///     "components": {
///       "responses": { "NotFound": { "description": "Not found" } }
///     }
///   })JSON");
/// }
///
/// const auto document{sourcemeta::core::parse_json(R"JSON({
///   "openapi": "3.1.1",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {
///     "/pets": {
///       "get": {
///         "responses": {
///           "404": { "$ref": "shared.json#/components/responses/NotFound" }
///         }
///       }
///     }
///   }
/// })JSON")};
///
/// const auto result{sourcemeta::core::openapi_bundle(
///     document, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver, resolver,
///     {.default_base = "https://example.com/openapi.json"})};
///
/// assert(result.at("components").at("responses").defines("NotFound"));
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_bundle(const JSON &document, const SchemaWalker &walker,
                    const SchemaResolver &schema_resolver,
                    const OpenAPIResolver &resolver,
                    const OpenAPIBundleOptions &options = {}) -> JSON;

} // namespace sourcemeta::core

#endif
