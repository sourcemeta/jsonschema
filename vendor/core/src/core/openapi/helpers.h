#ifndef SOURCEMETA_CORE_OPENAPI_HELPERS_H_
#define SOURCEMETA_CORE_OPENAPI_HELPERS_H_

#include <sourcemeta/core/openapi.h>

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>        // std::ranges::find
#include <array>            // std::array
#include <cstddef>          // std::size_t
#include <cstdint>          // std::uint8_t, std::uint64_t
#include <initializer_list> // std::initializer_list
#include <limits>           // std::numeric_limits
#include <map>              // std::map
#include <optional>         // std::optional
#include <set>              // std::set
#include <string_view>      // std::string_view
#include <utility>          // std::pair, std::unreachable
#include <vector>           // std::vector

namespace sourcemeta::core {

using namespace std::string_view_literals;

// OpenAPI Specification 3.1.1, Section 4.9: "The field name MUST begin with
// `x-`, for example, `x-internal-id`"
constexpr auto OPENAPI_EXTENSION_PREFIX{"x-"sv};
constexpr auto OPENAPI_HASH_DEPRECATED{JSON::Object::hash("deprecated"sv)};

constexpr auto OPENAPI_HASH_DESCRIPTION{JSON::Object::hash("description"sv)};
constexpr auto OPENAPI_HASH_SUMMARY{JSON::Object::hash("summary"sv)};
constexpr auto OPENAPI_HASH_URL{JSON::Object::hash("url"sv)};
constexpr auto OPENAPI_HASH_NAME{JSON::Object::hash("name"sv)};
constexpr auto OPENAPI_HASH_IN{JSON::Object::hash("in"sv)};
constexpr auto OPENAPI_HASH_TAGS{JSON::Object::hash("tags"sv)};
constexpr auto OPENAPI_HASH_SERVERS{JSON::Object::hash("servers"sv)};

/// A fixed field name paired with the hash of that name, so that looking one
/// up does not have to hash it again on every Object read
struct OpenAPIField {
  JSON::StringView name;
  JSON::Object::hash_type hash;
};

// What a reference expects to find at the far end of itself, which is fixed by
// where the reference sits rather than by anything the target says about
// itself. OpenAPI Specification 3.1.1, Section 4.3.1 calls this "the expected
// type of the reference", and it is what the place a reference lands on is
// held to
enum class OpenAPIObjectKind : std::uint8_t {
  /// A whole OpenAPI Description, which is what the root of the document
  /// framed is recorded as
  Document,
  // The eleven a reference may expect to find, the last of them only from 3.2
  // onwards, which is where a `content` map and the Components Object both
  // learn to hold a Reference Object in place of a Media Type Object
  PathItem,
  Parameter,
  RequestBody,
  Response,
  Example,
  Header,
  Link,
  Callbacks,
  SecurityScheme,
  MediaType,
  // The rest are never referenced, but every Object gets a location
  Info,
  Contact,
  License,
  Server,
  ServerVariable,
  Components,
  Paths,
  Operation,
  ExternalDocumentation,
  Encoding,
  Responses,
  Tag,
  Reference,
  Schema,
  OAuthFlows,
  OAuthFlow,
  SecurityRequirement
};

inline auto openapi_kind_name(const OpenAPIObjectKind kind) noexcept
    -> JSON::StringView {
  switch (kind) {
    case OpenAPIObjectKind::Document:
      return "openapi"sv;
    case OpenAPIObjectKind::PathItem:
      return "path-item"sv;
    case OpenAPIObjectKind::Parameter:
      return "parameter"sv;
    case OpenAPIObjectKind::RequestBody:
      return "request-body"sv;
    case OpenAPIObjectKind::Response:
      return "response"sv;
    case OpenAPIObjectKind::Example:
      return "example"sv;
    case OpenAPIObjectKind::Header:
      return "header"sv;
    case OpenAPIObjectKind::Link:
      return "link"sv;
    case OpenAPIObjectKind::Callbacks:
      return "callback"sv;
    case OpenAPIObjectKind::SecurityScheme:
      return "security-scheme"sv;
    case OpenAPIObjectKind::Info:
      return "info"sv;
    case OpenAPIObjectKind::Contact:
      return "contact"sv;
    case OpenAPIObjectKind::License:
      return "license"sv;
    case OpenAPIObjectKind::Server:
      return "server"sv;
    case OpenAPIObjectKind::ServerVariable:
      return "server-variable"sv;
    case OpenAPIObjectKind::Components:
      return "components"sv;
    case OpenAPIObjectKind::Paths:
      return "paths"sv;
    case OpenAPIObjectKind::Operation:
      return "operation"sv;
    case OpenAPIObjectKind::ExternalDocumentation:
      return "external-documentation"sv;
    case OpenAPIObjectKind::MediaType:
      return "media-type"sv;
    case OpenAPIObjectKind::Encoding:
      return "encoding"sv;
    case OpenAPIObjectKind::Responses:
      return "responses"sv;
    case OpenAPIObjectKind::Tag:
      return "tag"sv;
    case OpenAPIObjectKind::Reference:
      return "reference"sv;
    case OpenAPIObjectKind::Schema:
      return "schema"sv;
    case OpenAPIObjectKind::OAuthFlows:
      return "oauth-flows"sv;
    case OpenAPIObjectKind::OAuthFlow:
      return "oauth-flow"sv;
    case OpenAPIObjectKind::SecurityRequirement:
      return "security-requirement"sv;
  }

  std::unreachable();
}

/// Where an Object that stands in for another leads. OpenAPI Specification
/// 3.1.1 has a Reference Object and a Path Item Object each declare at most
/// one `$ref`, and a Schema Object's `$ref` never reaches here, so this is a
/// field of the Object that makes it rather than a table of its own
struct OpenAPIReference {
  /// The value as the document wrote it
  JSON::String original;
  /// Where it points, resolved against the base and canonicalised. The
  /// document it names and the fragment it carries are that string either side
  /// of its `#`, so neither is repeated here
  JSON::String destination;
  /// Whether that destination is nowhere the frame holds, which is what makes
  /// a description one that has to be made whole before it describes anything
  bool dangling{false};
};

/// How an Operation Object is reached from the entry document. OpenAPI
/// Specification 3.1.1, Section 4.3.3: "only the entry document's Paths Object
/// contributes URLs to the described API", so what an operation is reached
/// through is a property of the route to it rather than of where it is
/// defined
enum class OpenAPIOperationKind : std::uint8_t { Path, Webhook, Callback };

inline auto
openapi_operation_kind_name(const OpenAPIOperationKind kind) noexcept
    -> JSON::StringView {
  switch (kind) {
    case OpenAPIOperationKind::Path:
      return "path"sv;
    case OpenAPIOperationKind::Webhook:
      return "webhook"sv;
    case OpenAPIOperationKind::Callback:
      return "callback"sv;
  }

  std::unreachable();
}

/// What a Path Item Object declares that the endpoints reaching it need. Two
/// endpoints may reach one Path Item, and following a reference reads its
/// target once, so this is kept rather than read again
struct OpenAPIPathItemRecord {
  /// The methods it declares, in the order this specification lists them, each
  /// against the location of the Operation Object it holds
  /// The method owns its string, as 3.2 lets a Path Item name one that the
  /// document spells out rather than one this module names itself
  std::vector<std::pair<JSON::String, JSON::String>> operations;
  /// Where the Server Objects it declares sit
  std::vector<JSON::String> servers;
  /// Where the Parameter Objects it declares sit, in the order it writes them.
  /// A position may hold a Reference Object rather than a Parameter Object
  std::vector<JSON::String> parameters;
};

/// What an Operation Object declares that the endpoint reaching it needs
struct OpenAPIOperationRecord {
  /// Where the Server Objects it declares sit
  std::vector<JSON::String> servers;
  /// Where the Security Requirement Objects it declares sit. Declaring none
  /// and declaring an empty array differ, as Section 4.8.10 makes the latter
  /// the way "to remove a top-level security declaration"
  std::optional<std::vector<JSON::String>> security;
  /// Where each Callback Object it declares sits, which is a Reference Object
  /// rather than a Callback Object when the description wrote one there
  std::vector<JSON::String> callbacks;
  /// Where the Parameter Objects it declares sit, in the order it writes them
  std::vector<JSON::String> parameters;
  /// The tags it names, in the order it writes them
  std::vector<JSON::String> tags;
};

/// A Path Item Object that the entry document exposes, and what it is exposed
/// as: a path template, a webhook name, or a runtime expression
struct OpenAPIEndpoint {
  OpenAPIOperationKind kind;
  JSON::String path;
  JSON::String path_item;
};

/// One operation of the described API, which is what an endpoint and the Path
/// Item it reaches come to between them
struct OpenAPIOperation {
  OpenAPIOperationKind kind;
  JSON::String path;
  JSON::String method;
  /// Where the Operation Object sits
  JSON::String origin;
  /// Where the Path Item Object that exposes it sits, which is the position
  /// that gives it a URL rather than the one that defines it. The two differ
  /// whenever a reference stands between them
  JSON::String endpoint;
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

struct OpenAPILocation {
  OpenAPIObjectKind type;
  Pointer pointer;
  /// Set on the root of a document and on every Schema Object position it
  /// holds: the default `$schema` in force there, resolved against the base. A
  /// Schema Object that declares its own overrides it, which is a matter for
  /// whatever reads inside one
  JSON::String dialect;
  /// Set on a Schema Object position alone: the base its document keys every
  /// location by, which is what a relative reference inside that schema
  /// resolves against until an `$id` says otherwise. RFC 3986 Section 5.1.1
  /// makes an `$id` the higher precedence source, so this is a default in the
  /// same way the dialect above is
  JSON::String base;
};

// What every check needs to reach beyond the Object in front of it: the
// document it is reading, so an error can name it, and everything the checks
// that only run once the walk is over will want. OpenAPI Specification 3.1.1,
// Section 4.8.10 makes operation identifiers unique "among all operations
// described in the API", which is wider than the one document read here, so a
// clash is only ever caught within it
struct OpenAPIWalk {
  JSON::String base;
  // The document the checks are reading, which a reference that stays inside
  // it resolves its fragment against
  const JSON *document{nullptr};
  // Kept against where each identifier was read, so that reaching one Operation
  // Object twice, which following a reference into the document being read
  // does, is told apart from two Operation Objects claiming one identifier.
  std::map<JSON::String, JSON::String> operation_ids;
  // A reference may name a place another one already reached, or lead back
  // round to itself, so where the walk has been is remembered. It is keyed by
  // the kind expected as well as by the place, because one place reached as
  // two kinds is a conflict, and reading it once as whichever reference was
  // walked first would let the order the description is written in decide what
  // it is. Appendix G of OAS 3.2 names this hazard and says the behaviour
  // "MAY be treated as an error if detected", so reading it as each kind in
  // turn surfaces the conflict rather than hiding it
  std::set<std::pair<JSON::String, OpenAPIObjectKind>> visited;

  /// Keyed the way a schema frame keys its own, by the base with the pointer
  /// as a fragment, or by the pointer alone when no base was established. The
  /// document an Object sits in is that key up to its fragment, so nothing
  /// records it a second time
  std::map<JSON::String, OpenAPILocation> locations;
  /// Every reference the description makes, whether or not it was followed,
  /// keyed by the location of the Object that makes it. Kept apart from the
  /// locations themselves only because reading one Object twice records it
  /// twice, and what it stands in for must survive that
  std::map<JSON::String, OpenAPIReference> references;
  /// Every Path Item Object and Operation Object read, along with the Callback
  /// Objects that hold more of them, all keyed by where they sit. The
  /// projection that turns these into operations runs once the walk is over,
  /// as a Path Item may be reached before the endpoint that exposes it
  ///
  /// What every Parameter Object read is called and where it goes, keyed by
  /// where it sits. Section 4.8.9 identifies a parameter "by a combination of a
  /// name and location", which is what tells an override from an addition
  std::map<JSON::String, std::pair<JSON::String, JSON::String>> parameters;
  /// Every Path Item Object read, keyed by where it sits
  std::map<JSON::String, OpenAPIPathItemRecord> path_items;
  /// Every Operation Object read, keyed by where it sits
  std::map<JSON::String, OpenAPIOperationRecord> operation_records;
  /// Every Callback Object read, keyed by where it sits, holding the
  /// expression each of its entries is named by and the Path Item it carries
  std::map<JSON::String, std::vector<std::pair<JSON::String, JSON::String>>>
      callbacks;
  /// What the entry document exposes, in the order it writes it
  std::vector<OpenAPIEndpoint> endpoints;
  /// Where the entry document's own Server Objects and Security Requirement
  /// Objects sit, which is what an operation declaring neither falls back on
  std::vector<JSON::String> servers;
  std::optional<std::vector<JSON::String>> security;
  /// The names the entry document declares as security schemes, which is what
  /// a Security Requirement Object anywhere in the description may name
  std::set<JSON::String> security_schemes;
  /// Where the entry document declares each Tag Object, by the name it gave
  /// it, which is the name an Operation Object's tags resolve against
  std::map<JSON::String, JSON::String> tags;
  /// What each Tag Object that declares a parent is called and which tag it
  /// names, keyed by where that Tag Object sits. Section 4.22 has the named
  /// tag exist and forbids a cycle, neither of which can be settled until
  /// every tag has been read
  std::map<JSON::String, std::pair<JSON::String, JSON::String>> tag_parents;
  /// Every name any document declares a Tag Object under. Section 4.22 has a
  /// parent name "a tag that MUST exist in the API description", which is the
  /// whole of it rather than the entry document alone, so this is a wider set
  /// than the one above it
  std::set<JSON::String> tag_names;
  /// The operation each Link Object names, keyed by where that Link Object
  /// sits. Section 4.3.3 has resolving one of these require "parsing all
  /// referenced documents prior to determining an `operationId` to be
  /// unresolvable", so nothing is decided about them until the walk is over
  std::map<JSON::String, JSON::String> operation_id_links;
  /// The revision the document declares, which is what settles which field
  /// table each of its Objects is held to
  OpenAPIVersion version{OpenAPIVersion::OPENAPI_3_1};
  /// The default `$schema` in force for the document being read, which every
  /// Schema Object position in it hands on
  JSON::String dialect;
  /// What the entry document says about the API, kept so that reading it once
  /// serves both the walk and the caller
  OpenAPIInfo info;
  /// What recording a location may still spend, and what the caller allowed in
  /// the first place, which is what running out reports rather than whatever
  /// was left of it by then
  std::uint64_t remaining{std::numeric_limits<std::uint64_t>::max()};
  std::uint64_t limit{std::numeric_limits<std::uint64_t>::max()};
};

// Where a position that stands in for another leads, following as far as the
// chain goes. A Reference Object may name another one, so this is a walk
// rather than a lookup, and a position that stands in for nothing is itself
inline auto openapi_resolve_position(const OpenAPIWalk &walk,
                                     JSON::String position) -> JSON::String;

// What the Parameter Object at a position is called and where it goes, or
// nothing when a reference the walk never followed stands in the way
inline auto openapi_parameter_identity(const OpenAPIWalk &walk,
                                       const JSON::String &position)
    -> const std::pair<JSON::String, JSON::String> *;

// Record a reference and read whatever it lands on. Defined alongside the
// document-level checks, as what it lands on has to be read as whichever
// Object the reference position expects, and declared here because a Reference
// Object is the thing that triggers it
inline auto openapi_follow_reference(JSON::StringView reference,
                                     const Pointer &origin,
                                     OpenAPIObjectKind expected,
                                     OpenAPIWalk &walk) -> void;

// The same two halves of that, for a position that names another one without
// writing a `$ref` member, and so has nothing for the frame to record
inline auto openapi_reference_target(JSON::StringView reference,
                                     const OpenAPIWalk &walk)
    -> std::optional<URI>;

inline auto openapi_follow_target(const URI &target, const Pointer &origin,
                                  OpenAPIObjectKind expected, OpenAPIWalk &walk)
    -> void;

inline auto openapi_resolve_position(const OpenAPIWalk &walk,
                                     JSON::String position) -> JSON::String {
  std::set<JSON::String> seen;
  while (seen.insert(position).second) {
    const auto alias{walk.references.find(position)};
    if (alias == walk.references.cend()) {
      break;
    }

    position = alias->second.destination;
  }

  return position;
}

inline auto openapi_parameter_identity(const OpenAPIWalk &walk,
                                       const JSON::String &position)
    -> const std::pair<JSON::String, JSON::String> * {
  const auto match{
      walk.parameters.find(openapi_resolve_position(walk, position))};
  return match == walk.parameters.cend() ? nullptr : &match->second;
}

inline auto openapi_child(const Pointer &base, const JSON::StringView field)
    -> Pointer {
  return base.concat(JSON::String{field});
}

inline auto openapi_child(const Pointer &base, const std::size_t index)
    -> Pointer {
  return base.concat(index);
}

// OpenAPI Specification 3.1.1, Section 4.2: "The schema exposes two types of
// fields: fixed fields, which have a declared name, and patterned fields,
// which have a declared pattern for the field name". These objects declare
// `^x-` as their only pattern, so a member that is neither is not a field that
// this specification defines
template <std::size_t Size>
auto openapi_reject_unknown_fields(
    const JSON &object, const std::array<JSON::StringView, Size> &fields,
    const Pointer &base, const char *message) -> void {
  for (const auto &entry : object.as_object()) {
    if (entry.first.starts_with(OPENAPI_EXTENSION_PREFIX) ||
        std::ranges::find(fields, entry.first) != fields.cend()) {
      continue;
    }

    throw OpenAPIError{openapi_child(base, entry.first), message};
  }
}

// A field table is a property of the revision a document declares, so an
// Object whose table grew between revisions has one array per revision and
// which of them applies is asked here rather than at every call site
template <std::size_t Size, std::size_t Later>
auto openapi_reject_unknown_fields(
    const JSON &object, const std::array<JSON::StringView, Size> &fields,
    const std::array<JSON::StringView, Later> &later, const Pointer &base,
    const char *message, const OpenAPIWalk &walk) -> void {
  if (walk.version == OpenAPIVersion::OPENAPI_3_2) {
    openapi_reject_unknown_fields(object, later, base, message);
  } else {
    openapi_reject_unknown_fields(object, fields, base, message);
  }
}

// A location is keyed the way a schema frame keys its own: the document base
// with the pointer hung off it as a fragment, or the pointer alone when no
// base was established, and the base by itself for the root of a document.
//
// RFC 6901 Section 6: "A JSON Pointer can be represented in a URI fragment
// identifier by encoding it into octets using UTF-8, while percent-encoding
// those characters not allowed by the fragment rule in [RFC3986]". A path
// template holds braces and a callback expression holds a `#`, neither of
// which a fragment admits, so writing the pointer out as it stands would give
// a key that is no URI and that a reference to the same place could never
// match
inline auto openapi_location_uri(const JSON::String &base,
                                 const Pointer &pointer) -> JSON::String {
  if (pointer.empty()) {
    return base;
  }

  // RFC 3986 Section 5.2.2 resolves a fragment-only reference by keeping every
  // other component of the base as it stands, and a base here carries no
  // fragment of its own, so appending is that resolution without parsing the
  // base again for every Object recorded. The suite asserts that this agrees
  // with resolving the two through a URI, wherever a location reports the
  // base it was built from
  JSON::String result{base};
  result.append(to_uri(pointer).recompose());
  return result;
}

// Resolve a URI reference against a base and canonicalise what it comes to,
// which is what makes two spellings of one place one place, both to the set
// that remembers where a walk has been and to a caller comparing a destination
// against a location. Without a base there is nothing to resolve against,
// though what comes back is canonicalised either way
inline auto openapi_resolve_uri(const JSON::StringView reference,
                                const JSON::String &base)
    -> std::optional<URI> {
  try {
    URI target{JSON::String{reference}};
    if (!base.empty()) {
      target.resolve_from(URI{base});
    }

    target.canonicalize();
    return target;
  } catch (const URIParseError &) {
    return std::nullopt;
  }
}

// The document a location key names, which is that key up to its fragment,
// which is why nothing repeats it on the entry a key leads to
inline auto openapi_document_uri(const JSON::String &uri) -> JSON::String {
  return JSON::String{take_until(uri, '#')};
}

// OpenAPI Specification 3.1.1, Section 4.6 determines a document's base URI
// "in accordance with RFC3986 Section 5.1.2 - 5.1.4", a range that starts at
// 5.1.2 and so leaves out 5.1.1, "Base URI Embedded in Content". A 3.1
// document therefore has no way of declaring its own base, and what remains is
// 5.1.3, "Base URI from the Retrieval URI", which only the caller can supply.
// Section 4.6 says as much: implementations "SHOULD allow users to provide
// documents with their intended retrieval URIs"
inline auto openapi_canonical_base(const std::string_view input)
    -> JSON::String {
  if (input.empty()) {
    return {};
  }

  std::optional<URI> base;
  try {
    base.emplace(input);
  } catch (const URIParseError &) {
    base.reset();
  }

  // RFC 3986 Section 5.2.1: "only the scheme component is required to be
  // present in a base URI". Anything without one cannot resolve a reference
  // RFC 3986 Section 5.2.2 resolves a reference against a base's scheme,
  // authority, path and query, and never against its fragment, so a fragment
  // is no part of what a base is. Keeping one would also put two of them in
  // every location this frame reports
  if (base.has_value() && base.value().scheme().has_value()) {
    base.value().canonicalize();
    const auto result{base.value().recompose_without_fragment()};
    if (result.has_value()) {
      return result.value();
    }
  }

  throw OpenAPIError{EMPTY_POINTER,
                     "The OpenAPI Description base must be a URI with a "
                     "scheme"};
}

// Every Object gets one of these. The nearest recorded ancestor is the parent,
// which holds because an Object is always recorded before anything inside it
//
// Appendix G of OAS 3.2, and Section 4.3.2 of 3.1, say of one place read as
// two
// kinds of Object:
//
//   the resulting behavior is implementation defined, and MAY be treated as
//   an error if detected. An example would be referencing an empty Schema
//   Object under `#/components/schemas` where a Path Item Object is expected,
//   as an empty object is valid for both types
//
// Detecting it is exactly what recording every Object by where it sits comes
// to, and letting the last read win would have the order the description
// happens to be written in decide what a place is, which is worse than saying
// so. What a reference reaches twice as the same kind is no conflict
inline auto openapi_record(OpenAPIWalk &walk, const Pointer &pointer,
                           const OpenAPIObjectKind kind,
                           JSON::String dialect = {}, JSON::String base = {})
    -> void {
  auto uri{openapi_location_uri(walk.base, pointer)};
  const auto known{walk.locations.find(uri)};
  // Reading one place twice records it once, so what an allowance is spent on
  // is the places a description holds rather than the times it is read
  if (known == walk.locations.cend()) {
    if (walk.remaining == 0) {
      throw OpenAPIFrameLimitError{walk.limit};
    }

    walk.remaining -= 1;
  }

  if (known != walk.locations.cend() && known->second.type != kind) {
    throw OpenAPIError{walk.base, pointer,
                       "This place is read as more than one kind of Object"};
  }

  walk.locations.insert_or_assign(std::move(uri),
                                  OpenAPILocation{.type = kind,
                                                  .pointer = pointer,
                                                  .dialect = std::move(dialect),
                                                  .base = std::move(base)});
}

inline auto openapi_expect_object(const JSON &value, const Pointer &location,
                                  const char *message) -> void {
  if (!value.is_object()) {
    throw OpenAPIError{location, message};
  }
}

inline auto openapi_expect_array(const JSON &value, const Pointer &location,
                                 const char *message) -> void {
  if (!value.is_array()) {
    throw OpenAPIError{location, message};
  }
}

inline auto openapi_expect_boolean(const JSON &value, const Pointer &base,
                                   const JSON::StringView field,
                                   const char *message) -> void {
  if (!value.is_boolean()) {
    throw OpenAPIError{openapi_child(base, field), message};
  }
}

// A Schema Object is where framing stops, and Section 4.8.24 permits it to be
// "the boolean value `true`" or "the boolean value `false`" as well as an
// object, which is all the meta-schema asserts of one it leaves unvalidated.
// Its location is the handoff: a pointer, the base its key carries, and the
// dialect in force, which is everything a JSON Schema implementation needs to
// take it from here
inline auto openapi_expect_schema(const JSON &value, const Pointer &location,
                                  const char *message, OpenAPIWalk &walk)
    -> void {
  if (!value.is_object() && !value.is_boolean()) {
    throw OpenAPIError{location, message};
  }

  // A Schema Object is a handoff rather than something this module reads, so
  // its position carries both of the things whatever reads inside it needs:
  // the dialect in force and the base to resolve against
  openapi_record(walk, location, OpenAPIObjectKind::Schema, walk.dialect,
                 walk.base);
}

inline auto openapi_check_map_of_strings(const JSON &value,
                                         const Pointer &location,
                                         const char *type_message,
                                         const char *entry_message) -> void {
  openapi_expect_object(value, location, type_message);
  for (const auto &entry : value.as_object()) {
    if (!entry.second.is_string()) {
      throw OpenAPIError{openapi_child(location, entry.first), entry_message};
    }
  }
}

inline auto openapi_check_array_of_strings(const JSON &value,
                                           const Pointer &location,
                                           const char *type_message,
                                           const char *entry_message) -> void {
  openapi_expect_array(value, location, type_message);
  std::size_t index{0};
  for (const auto &entry : value.as_array()) {
    if (!entry.is_string()) {
      throw OpenAPIError{openapi_child(location, index), entry_message};
    }

    index += 1;
  }
}

// The expressions a template declares between curly braces. OpenAPI
// Specification 3.1.1, Section 3.5: "Path templating refers to the usage of
// template expressions, delimited by curly braces (`{}`), to mark a section of
// a URL path as replaceable using path parameters", and Section 4.8.5 names
// server variables the same way. Nothing there says what an unbalanced brace
// means, so a run that never closes is no expression
inline auto openapi_brace_expressions(const JSON::StringView value)
    -> std::vector<JSON::StringView> {
  std::vector<JSON::StringView> result;
  std::size_t cursor{0};
  while (cursor < value.size()) {
    const auto open{value.find('{', cursor)};
    if (open == JSON::StringView::npos) {
      break;
    }

    const auto close{value.find('}', open)};
    if (close == JSON::StringView::npos) {
      break;
    }

    result.push_back(value.substr(open + 1, close - open - 1));
    cursor = close + 1;
  }

  return result;
}

inline auto openapi_expect_string(const JSON &value, const Pointer &base,
                                  const JSON::StringView field,
                                  const char *message) -> JSON::StringView {
  if (!value.is_string()) {
    throw OpenAPIError{openapi_child(base, field), message};
  }

  return value.to_string();
}

// A field an Object may leave out, which is checked only where it is written.
// The overwhelming majority of what this specification asks of a field is that
// it holds a string, so that shape is worth reading as one line
inline auto openapi_check_optional_string(const JSON &value,
                                          const Pointer &base,
                                          const JSON::StringView field,
                                          const JSON::Object::hash_type hash,
                                          const char *message) -> void {
  const auto *member{value.try_at(field, hash)};
  if (member != nullptr) {
    openapi_expect_string(*member, base, field, message);
  }
}

// A field an Object must declare, handed back already found, so that what
// follows reads what is there rather than what might be
inline auto openapi_require(const JSON &value, const JSON::StringView field,
                            const JSON::Object::hash_type hash,
                            const Pointer &base, const char *message)
    -> const JSON & {
  const auto *member{value.try_at(field, hash)};
  if (member == nullptr) {
    throw OpenAPIError{base, message};
  }

  return *member;
}

// OpenAPI Specification 3.1.1, Section 4.6: "Unless specified otherwise, all
// fields that are URIs MAY be relative references as defined by RFC3986", so
// what these fields hold is a URI reference rather than an absolute URI
inline auto openapi_expect_uri_reference(const JSON &value, const Pointer &base,
                                         const JSON::StringView field,
                                         const char *type_message,
                                         const char *syntax_message)
    -> JSON::StringView {
  const auto result{openapi_expect_string(value, base, field, type_message)};
  if (!URI::is_uri_reference(result)) {
    throw OpenAPIError{openapi_child(base, field), syntax_message};
  }

  return result;
}

inline auto openapi_expect_enumeration(
    const JSON &value, const Pointer &base, const JSON::StringView field,
    const std::initializer_list<JSON::StringView> options,
    const char *type_message, const char *value_message) -> JSON::StringView {
  const auto result{openapi_expect_string(value, base, field, type_message)};
  if (std::ranges::find(options, result) == options.end()) {
    throw OpenAPIError{openapi_child(base, field), value_message};
  }

  return result;
}

} // namespace sourcemeta::core

#endif
