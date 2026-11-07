#ifndef SOURCEMETA_CORE_OPENAPI_TYPES_H_
#define SOURCEMETA_CORE_OPENAPI_TYPES_H_

#ifndef SOURCEMETA_CORE_OPENAPI_EXPORT
#include <sourcemeta/core/openapi_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/memory.h>

#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// What kind of Object an OpenAPI Description holds at a given position. For
/// a position a reference names, this is what it expects to find at the far
/// end of itself, which is fixed by where the reference sits rather than by
/// anything the target says about itself. OpenAPI Specification 3.1.1,
/// Section 4.3.1 calls this "the expected type of the reference", and it is
/// what the place a reference lands on is held to
enum class OpenAPIObjectKind : std::uint8_t {
  /// A whole OpenAPI Description, which is what the root of the document
  /// framed is recorded as
  Document,
  // The eleven a reference may expect to find, the last of them only from 3.2
  // onwards, which is where a `content` map and the Components Object both
  // learn to hold a Reference Object in place of a Media Type Object
  /// A Path Item Object, which describes the operations available on one path
  PathItem,
  /// A Parameter Object, which describes one parameter of an operation
  Parameter,
  /// A Request Body Object, which describes the body an operation takes
  RequestBody,
  /// A Response Object, which describes one response of an operation
  Response,
  /// An Example Object, which pairs an example value with its metadata
  Example,
  /// A Header Object, which describes one header of a response or an encoding
  Header,
  /// A Link Object, which names a design time relationship to an operation
  Link,
  /// A Callback Object, which maps runtime expressions to out of band
  /// requests
  Callbacks,
  /// A Security Scheme Object, which describes one way of authenticating
  SecurityScheme,
  /// A Media Type Object, which describes one representation of a body
  MediaType,
  // The rest are never referenced, but every Object gets a location
  /// An Info Object, which carries the metadata about the API
  Info,
  /// A Contact Object, which names who to reach about the API
  Contact,
  /// A License Object, which names the license the API is offered under
  License,
  /// A Server Object, which names a host the API is served from
  Server,
  /// A Server Variable Object, which describes one substitution a server URL
  /// template takes
  ServerVariable,
  /// A Components Object, which holds the Objects a description reuses
  Components,
  /// A Paths Object, which maps path templates to what describes them
  Paths,
  /// An Operation Object, which describes one operation on a path
  Operation,
  /// An External Documentation Object, which points at documentation held
  /// elsewhere
  ExternalDocumentation,
  /// An Encoding Object, which describes how one property of a body is
  /// serialised
  Encoding,
  /// A Responses Object, which maps status codes to what describes them
  Responses,
  /// A Tag Object, which adds metadata to a tag that the operations name
  Tag,
  /// A Reference Object, which stands in for another Object
  Reference,
  /// A Schema Object, whose contents are JSON Schema's to make sense of
  /// rather than this specification's
  Schema,
  /// An OAuth Flows Object, which holds the flows an OAuth scheme supports
  OAuthFlows,
  /// An OAuth Flow Object, which describes one flow a scheme supports
  OAuthFlow,
  /// A Security Requirement Object, which names the schemes that apply
  SecurityRequirement
};

/// How an Operation Object is reached from the entry document. OpenAPI
/// Specification 3.1.1, Section 4.3.3: "only the entry document's Paths
/// Object contributes URLs to the described API", so what an operation is
/// reached through is a property of the route to it rather than of where it
/// is defined
enum class OpenAPIOperationKind : std::uint8_t {
  /// Reached through the Paths Object of the entry document
  Path,
  /// Reached through the webhooks that the entry document declares
  Webhook,
  /// Reached through a Callback Object that an operation declares
  Callback
};

/// @ingroup openapi
/// The name that a frame exports a kind of Object as, which is the name the
/// specification gives that Object hyphenated and in lower case. For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::openapi_kind_name(
///            sourcemeta::core::OpenAPIObjectKind::PathItem) ==
///        "path-item");
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_kind_name(const OpenAPIObjectKind kind) noexcept
    -> JSON::StringView;

/// @ingroup openapi
/// The name that a frame exports a route to an operation as. For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::openapi_operation_kind_name(
///            sourcemeta::core::OpenAPIOperationKind::Webhook) ==
///        "webhook");
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_operation_kind_name(const OpenAPIOperationKind kind) noexcept
    -> JSON::StringView;

/// @ingroup openapi
/// What a sourcemeta::core::OpenAPIResolver hands back: either a document it
/// owns, or a reference to one that outlives the call
using OpenAPIResolverResult = OwnedOrReference<JSON>;

/// @ingroup openapi
/// How bundling reaches the other documents that an OpenAPI Description is
/// split across. A URI that names nothing is reported by handing back no
/// value, and whatever else a URI names is handed back as it stands, for
/// bundling to decide whether it is a document a description may span. For
/// example:
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

} // namespace sourcemeta::core

#endif
