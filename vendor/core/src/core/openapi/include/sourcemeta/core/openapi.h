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

#include <concepts>    // std::invocable, std::predicate
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <functional>  // std::function, std::less
#include <limits>      // std::numeric_limits
#include <map>         // std::map
#include <memory>      // std::unique_ptr
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <vector>      // std::vector

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
/// The OpenAPI Description versions that this module recognises. The
/// enumerators run in the order the revisions were published, so that
/// comparing them says what the specification says when it gives the revision
/// a field arrived in
enum class OpenAPIVersion : std::uint8_t {
  /// The OpenAPI Specification 3.0 revision
  OPENAPI_3_0,
  /// The OpenAPI Specification 3.1 revision
  OPENAPI_3_1,
  /// The OpenAPI Specification 3.2 revision
  OPENAPI_3_2
};

/// @ingroup openapi
/// Determine the version of an OpenAPI Description from its `openapi` field
/// without framing it, returning no value for a version we do not recognise
/// and for anything that declares no such field to read.
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
/// The feature set a version designates, which OpenAPI Specification 3.1.1,
/// Section 4.1 spells as "the `major`.`minor` portion of the version string".
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::openapi_version_name(
///            sourcemeta::core::OpenAPIVersion::OPENAPI_3_1) == "3.1");
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_version_name(const OpenAPIVersion version) noexcept
    -> JSON::StringView;

/// @ingroup openapi
/// The version that an OpenAPI Description declares, exactly as it wrote it,
/// returning no value for anything that declares none. A description naming a
/// revision this module does not recognise reports one here all the same, which
/// is what lets a caller quote it back. The result borrows from the document it
/// was read from, so that document must outlive it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"({
///   "openapi": "3.0.4",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {}
/// })")};
///
/// assert(sourcemeta::core::openapi_version_string(document).value() ==
///        "3.0.4");
/// assert(!sourcemeta::core::openapi_version(document).has_value());
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_version_string(const JSON &document)
    -> std::optional<JSON::StringView>;

/// @ingroup openapi
/// Whether a value declares itself an OpenAPI Description, whichever revision
/// it names and whether or not it holds up as one. This is what tells a
/// description from a schema before either is read. For example:
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
/// assert(sourcemeta::core::openapi_is_document(document));
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_is_document(const JSON &document) -> bool;

/// @ingroup openapi
/// The base that every place of an OpenAPI Description is addressed by, given
/// the URI the document was retrieved from. That is the URI the document names
/// itself with, where the revision it declares admits one and what it names can
/// be made absolute, and the retrieval URI canonicalised otherwise. Asking this
/// is how a caller holding one document tells a problem reported within it from
/// one reported in a document reached from it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// const auto document{sourcemeta::core::parse_json(R"({
///   "openapi": "3.2.0",
///   "$self": "https://example.com/api",
///   "info": { "title": "Example", "version": "1.0.0" },
///   "paths": {}
/// })")};
///
/// assert(sourcemeta::core::openapi_base(
///            document, "https://example.com/openapi.json") ==
///        "https://example.com/api");
/// ```
///
/// A retrieval URI that establishes nothing is given as the empty URI
/// reference, which is answered by whatever the document names itself with, and
/// by the empty URI reference when that is nothing absolute either, leaving
/// every relative reference the description makes relative. Anything else must
/// carry a scheme, as RFC 3986 Section 5.2.1 requires one of a base, and one
/// that does not is refused by throwing sourcemeta::core::OpenAPIError, as
/// reading a description under such a base is refused too
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_base(const JSON &document, const std::string_view retrieval)
    -> JSON::String;

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
/// A frame is analysed once, on construction, and what it reports never
/// changes afterwards. Reading one is not thread safe even so, as it answers
/// out of caches it fills as it goes. A frame cannot be copied or moved, so it
/// is built where it is read.
class SOURCEMETA_CORE_OPENAPI_EXPORT OpenAPIFrame {
public:
  /// What kind of Object an OpenAPI Description holds at a given position. For
  /// a position a reference names, this is what it expects to find at the far
  /// end of itself, which is fixed by where the reference sits rather than by
  /// anything the target says about itself. OpenAPI Specification 3.1.1,
  /// Section 4.3.1 calls this "the expected type of the reference", and it is
  /// what the place a reference lands on is held to
  enum class ObjectKind : std::uint8_t {
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
  enum class OperationKind : std::uint8_t {
    /// Reached through the Paths Object of the entry document
    Path,
    /// Reached through the webhooks that the entry document declares
    Webhook,
    /// Reached through a Callback Object that an operation declares
    Callback
  };

  /// Where an Object that stands in for another leads. OpenAPI Specification
  /// 3.1.1 has a Reference Object and a Path Item Object each declare at most
  /// one `$ref`, and a Schema Object's `$ref` never reaches here, so this is a
  /// field of the Object that makes it rather than a table of its own
  struct Reference {
    /// The value as the document wrote it
    JSON::String original;
    /// Where it points, resolved against the base and canonicalised. The
    /// document it names and the fragment it carries are that string either
    /// side of its `#`, so neither is repeated here
    JSON::String destination;
    /// Whether that destination is nowhere the frame holds, which is what makes
    /// a description one that has to be made whole before it describes anything
    bool dangling{false};
    /// What the position that spells it expects to find at the far end, which
    /// OpenAPI Specification 3.1.1, Section 4.3.1 fixes by where the reference
    /// sits rather than by anything the target says about itself
    ObjectKind expected{ObjectKind::Document};
    /// Where the member that spells it sits, which is the one place a rewrite
    /// of this reference has to write to
    Pointer origin;
  };

  /// One operation of the described API, which is what an endpoint and the Path
  /// Item it reaches come to between them
  struct Operation {
    /// How the entry document exposes it
    OperationKind kind;
    /// The path template, the webhook name, or the runtime expression that
    /// exposes it, according to how it is reached
    JSON::String path;
    /// The method it answers to, as the field that declares it is spelled
    JSON::String method;
    /// Where the Operation Object sits
    JSON::String origin;
    /// Where the Path Item Object that exposes it sits, which is the position
    /// that gives it a URL rather than the one that defines it. The two differ
    /// whenever a reference stands between them
    JSON::String endpoint;
    /// Where the Operation Object that a Callback Object hangs off sits, with
    /// no value for an operation the Paths Object or the webhooks exposes.
    /// Section 4.8.10 has a Callback Object be "a map of possible out-of band
    /// callbacks related to the parent operation", and what the expression it
    /// is keyed by evaluates against is that Object's request, so one Callback
    /// Object two Operation Objects reach describes one callback for each of
    /// them
    std::optional<JSON::String> parent{std::nullopt};
    /// Where the Server Objects in force sit, empty when nothing declares any,
    /// in which case Section 4.8.1 puts a single Server Object with a `url` of
    /// `/` in their place
    std::vector<JSON::String> servers;
    /// Where the Security Requirement Objects in force sit
    std::vector<JSON::String> security;
    /// Where the Parameter Objects in force sit, which is what the Path Item
    /// Object declares once anything the Operation Object overrides is taken
    /// out, followed by what the Operation Object declares itself
    std::vector<JSON::String> parameters;
    /// Where the Tag Object each of its tags names sits, in the order the
    /// operation wrote them, with no value where the entry document declares no
    /// tag by that name. Section 4.8.1 permits exactly that: "Not all tags that
    /// are used by the Operation Object must be declared"
    std::vector<std::optional<JSON::String>> tags;
  };

  /// Where a Discriminator Object names a schema, by the name of a component or
  /// by URI. OpenAPI Specification 3.1.1, Section 4.3 lists the URI form of a
  /// `mapping` among the fields that connect the documents of a description,
  /// and Section 4.3.3 lists the name form among the connections it makes by
  /// name, so either way one of these is a place the description reaches for
  struct Discriminator {
    /// Where the mapping value sits, as a pointer from the root of the document
    Pointer origin;
    /// Where it points, resolved and canonicalised
    JSON::String destination;
    /// What it resolved against, which is the nearest identifier an enclosing
    /// schema declares for the URI form, and the description itself for the
    /// name form
    JSON::String scope;
    /// Whether that destination is no schema the frame holds, which counts a
    /// position that the schemas record without a schema of its own as nowhere
    /// to land
    bool dangling{false};
  };

  /// Where an Object sits in a description, and what the frame knows
  /// about it
  struct Location {
    /// What kind of Object sits here
    ObjectKind type;
    /// Where in the document it sits, as a pointer from the root
    Pointer pointer;
    /// The URI of the Object that holds this one, which is the nearest position
    /// above it that the frame records, with no value for the root of a
    /// document. It borrows from the key the frame keys that Object by, so the
    /// frame must outlive it
    std::optional<JSON::StringView> parent{std::nullopt};
    /// Set on the root of a document and on every Schema Object position it
    /// holds, empty elsewhere: the default `$schema` in force there, resolved
    /// against the base. A Schema Object that declares its own overrides it, so
    /// this is not what that schema is written in. The frame of the schemas a
    /// description holds answers that
    JSON::String default_dialect;
    /// Set on a Schema Object position alone, empty elsewhere: the base its
    /// document keys every location by, which is what a relative reference
    /// inside that schema resolves against until an `$id` says otherwise. RFC
    /// 3986 Section 5.1.1 makes an `$id` the higher precedence source, so this
    /// is a default in the same way the dialect above is
    JSON::String base;
  };

  /// The Objects a frame holds, keyed by the URI addressing each one.
  /// Comparison is transparent so that a lookup may take a view of a URI
  /// without building a string of it
  using Locations = std::map<JSON::String, Location, std::less<>>;

  /// The references a frame holds, keyed by the URI of the Object that makes
  /// each one
  using References = std::map<JSON::String, Reference, std::less<>>;

  /// Frame an OpenAPI Description from a given document. That document must
  /// outlive the frame, as the metadata it reports borrows from it. The given
  /// base need not, as the frame canonicalises it into a string of its own
  ///
  /// The base is the retrieval URI of the document. OpenAPI 3.1 offers a
  /// document no way of declaring an identity of its own, so under that
  /// revision this is the only way to give the description one. From 3.2
  /// onwards a document may declare `$self`, which takes precedence once it is
  /// absolute, resolving against this when relative and standing aside when
  /// neither gives it a scheme
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
  /// and a resolver of your own for one that is not. The frame keeps the
  /// resolver and asks it again when exporting, so whatever it reaches for has
  /// to be there for as long as the frame is
  ///
  /// The places the description holds and the places its Schema Objects hold
  /// are places of the one description, so they spend from the one allowance.
  /// Bound it to throw sourcemeta::core::OpenAPIFrameLimitError rather than
  /// register past it, which reports the allowance the caller set rather than
  /// whatever was left of it
  ///
  /// A base that establishes nothing is given as the empty URI reference, which
  /// leaves the document to name itself and leaves every relative reference it
  /// makes relative where it names nothing absolute either. Anything else must
  /// carry a scheme, and one that does not is refused before the document is
  /// read, which is why such a refusal names no place within it
  ///
  /// A document that does not conform to the specification is rejected here
  /// rather than reported back, by throwing sourcemeta::core::OpenAPIError.
  /// What sits inside a Schema Object is held to JSON Schema instead, so one
  /// naming a dialect nothing resolves throws
  /// sourcemeta::core::SchemaResolutionError, one declaring an identifier or a
  /// reference that is no URI throws sourcemeta::core::SchemaKeywordError, and
  /// two colliding on an identifier or on an anchor throw
  /// sourcemeta::core::SchemaFrameError and
  /// sourcemeta::core::SchemaAnchorCollisionError respectively. One whose
  /// dialect or base dialect cannot be settled at all throws
  /// sourcemeta::core::SchemaUnknownDialectError or
  /// sourcemeta::core::SchemaUnknownBaseDialectError
  OpenAPIFrame(
      const JSON &document, const SchemaWalker &walker,
      const SchemaResolver &resolver, const std::string_view default_base = "",
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
  /// supplied when it declares none or when what it declares cannot be made
  /// absolute, in either case stripped of any fragment. For example:
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
  /// the shell around them does. The dialect a Schema Object names is not one
  /// of those, as a schema is under no obligation to carry the meta-schema it
  /// is written against. For example:
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

  /// Every Object the description holds, keyed by the URI addressing each one
  [[nodiscard]] auto locations() const noexcept -> const Locations &;

  /// Every reference the description makes, keyed by the URI of the Object that
  /// makes it rather than by its `$ref` member, so that where a reference comes
  /// from is a location like any other
  [[nodiscard]] auto references() const noexcept -> const References &;

  /// Every scheme a Security Requirement Object names by the URI of one, which
  /// OpenAPI Specification 3.2.1 admits alongside the name of a component.
  /// These are kept apart from the references above because a single such
  /// Object may name several, while every other way of naming an Object is
  /// written down where the Object that makes it sits
  [[nodiscard]] auto security_references() const noexcept -> const References &;

  /// Every operation the described API exposes
  [[nodiscard]] auto operations() const noexcept
      -> const std::vector<Operation> &;

  /// Every URI a Discriminator Object names, which is a reference the schemas
  /// hold rather than one the shell around them does
  [[nodiscard]] auto discriminators() const noexcept
      -> const std::vector<Discriminator> &;

  /// Iterate over every Object the description holds, along with the URI the
  /// frame keys it by. For example:
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
  ///
  /// frame.for_each_object([](const auto &uri, const auto &location) {
  ///   if (location.type ==
  ///       sourcemeta::core::OpenAPIFrame::ObjectKind::Schema) {
  ///     std::cout << "a schema at " << uri << "\n";
  ///   }
  /// });
  /// ```
  template <std::invocable<const JSON::String &, const Location &> F>
  auto for_each_object(const F &callback) const -> void {
    for (const auto &entry : this->locations()) {
      callback(entry.first, entry.second);
    }
  }

  /// Check whether any Object the description holds satisfies the predicate.
  /// For example:
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
  ///
  /// assert(frame.any_object([](const auto &, const auto &location) {
  ///   return location.type ==
  ///     sourcemeta::core::OpenAPIFrame::ObjectKind::Paths;
  /// }));
  /// ```
  template <std::predicate<const JSON::String &, const Location &> F>
  [[nodiscard]] auto any_object(const F &predicate) const -> bool {
    for (const auto &entry : this->locations()) {
      if (predicate(entry.first, entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over every reference the description makes, along with the URI of
  /// the Object that makes it. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <iostream>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "paths": { "/users": { "$ref": "#/components/pathItems/Users" } },
  ///   "components": { "pathItems": { "Users": {} } }
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver};
  ///
  /// frame.for_each_reference([](const auto &origin, const auto &reference) {
  ///   std::cout << origin << " -> " << reference.destination << "\n";
  /// });
  /// ```
  template <std::invocable<const JSON::String &, const Reference &> F>
  auto for_each_reference(const F &callback) const -> void {
    for (const auto &entry : this->references()) {
      callback(entry.first, entry.second);
    }
  }

  /// Iterate over every scheme a Security Requirement Object names by the URI
  /// of one, along with the URI of the Object that names it. For example:
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
  ///
  /// frame.for_each_security_reference(
  ///     [](const auto &origin, const auto &reference) {
  ///       std::cout << origin << " -> " << reference.destination << "\n";
  ///     });
  /// ```
  template <std::invocable<const JSON::String &, const Reference &> F>
  auto for_each_security_reference(const F &callback) const -> void {
    for (const auto &entry : this->security_references()) {
      callback(entry.first, entry.second);
    }
  }

  /// Iterate over every operation the described API exposes. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/json.h>
  /// #include <sourcemeta/core/openapi.h>
  /// #include <iostream>
  ///
  /// const auto document{sourcemeta::core::parse_json(R"({
  ///   "openapi": "3.1.1",
  ///   "info": { "title": "Example", "version": "1.0.0" },
  ///   "paths": { "/users": { "get": { "responses": {
  ///     "200": { "description": "Some users" } } } } }
  /// })")};
  ///
  /// const sourcemeta::core::OpenAPIFrame frame{
  ///     document, sourcemeta::core::schema_walker,
  ///     sourcemeta::core::schema_resolver};
  ///
  /// frame.for_each_operation([](const auto &operation) {
  ///   std::cout << operation.method << " " << operation.path << "\n";
  /// });
  /// ```
  template <std::invocable<const Operation &> F>
  auto for_each_operation(const F &callback) const -> void {
    for (const auto &operation : this->operations()) {
      callback(operation);
    }
  }

  /// Iterate over every URI a Discriminator Object names. For example:
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
  ///
  /// frame.for_each_discriminator([](const auto &discriminator) {
  ///   std::cout << discriminator.destination << "\n";
  /// });
  /// ```
  template <std::invocable<const Discriminator &> F>
  auto for_each_discriminator(const F &callback) const -> void {
    for (const auto &discriminator : this->discriminators()) {
      callback(discriminator);
    }
  }

  /// The Object a URI names, or nothing when the frame holds none. This is what
  /// turns the destination of a reference into the Object it lands on, as every
  /// destination is one of the URIs this frame addresses its Objects by. For
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
  ///     sourcemeta::core::schema_resolver,
  ///     "https://example.com/openapi.json"};
  ///
  /// assert(frame.traverse("https://example.com/openapi.json#/info")->type ==
  ///        sourcemeta::core::OpenAPIFrame::ObjectKind::Info);
  /// ```
  [[nodiscard]] auto traverse(const JSON::StringView uri) const
      -> const Location *;

  /// The URI this frame addresses the given position by, which is the inverse
  /// of traversal. For example:
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
  ///
  /// assert(frame.uri(sourcemeta::core::Pointer{"info"}) ==
  ///        "https://example.com/openapi.json#/info");
  /// ```
  [[nodiscard]] auto uri(const Pointer &pointer) const -> JSON::String;

  /// How many Objects the description holds
  [[nodiscard]] auto object_count() const noexcept -> std::size_t;

  /// How many references the description makes, not counting what a Security
  /// Requirement Object names by the URI of a scheme
  [[nodiscard]] auto reference_count() const noexcept -> std::size_t;

  /// Export the frame as JSON. This is the complete state of the frame. It asks
  /// the resolver the frame kept, so a meta-schema that has gone out of reach
  /// since throws sourcemeta::core::SchemaResolutionError here rather than at
  /// construction.
  ///
  /// Pass the tracker that read the document to report where every place the
  /// export names by a pointer sits within it, which counts the places its
  /// Schema Objects hold as much as the Objects around them. A place the
  /// tracker holds nothing for reports a null position, and where no tracker is
  /// given no position is reported at all. For example:
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
  ///
  /// assert(frame.to_json().at("version").to_string() == "3.1");
  /// ```
  [[nodiscard]] auto to_json(
      const std::optional<PointerPositionTracker> &tracker = std::nullopt) const
      -> JSON;

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
/// The name that a frame exports a kind of Object as, which is the name the
/// specification gives that Object hyphenated and in lower case. For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::openapi_kind_name(
///            sourcemeta::core::OpenAPIFrame::ObjectKind::PathItem) ==
///        "path-item");
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_kind_name(const OpenAPIFrame::ObjectKind kind) noexcept
    -> JSON::StringView;

/// @ingroup openapi
/// The name that a frame exports a route to an operation as. For example:
///
/// ```cpp
/// #include <sourcemeta/core/openapi.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::openapi_operation_kind_name(
///            sourcemeta::core::OpenAPIFrame::OperationKind::Webhook) ==
///        "webhook");
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_operation_kind_name(
    const OpenAPIFrame::OperationKind kind) noexcept -> JSON::StringView;

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
///
/// This function reorders an OpenAPI Description in place, following an
/// opinionated OpenAPI aware order, and hands every Schema Object it holds to
/// the JSON Schema formatter. Note that doing so invalidates the given frame,
/// as the locations it holds point into the document. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <sourcemeta/core/openapi.h>
///
/// #include <iostream>
///
/// auto document = sourcemeta::core::parse_json(R"JSON({
///   "info": { "version": "1.0.0", "title": "Example" },
///   "paths": {},
///   "openapi": "3.1.1"
/// })JSON");
///
/// const sourcemeta::core::OpenAPIFrame frame{
///     document, sourcemeta::core::schema_walker,
///     sourcemeta::core::schema_resolver};
///
/// sourcemeta::core::openapi_format(document, frame);
/// sourcemeta::core::prettify(document, std::cout);
/// ```
SOURCEMETA_CORE_OPENAPI_EXPORT
auto openapi_format(JSON &document, const OpenAPIFrame &frame) -> void;

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
/// than the one the entry document declares. The specification does not ask
/// for that. It is a choice this makes, as what this produces is one document
/// that declares one revision, and there is none to pick that can express both
/// what one revision holds and what another does. This overload mutates the
/// input document. For example:
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
/// the entry document declares, which is a choice this makes rather than one
/// the specification asks for. This overload returns a new document, without
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
