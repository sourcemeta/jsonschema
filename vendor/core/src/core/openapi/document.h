#ifndef SOURCEMETA_CORE_OPENAPI_DOCUMENT_H_
#define SOURCEMETA_CORE_OPENAPI_DOCUMENT_H_

#include <sourcemeta/core/openapi.h>

#include <sourcemeta/core/uri.h>

#include "components.h"
#include "content.h"
#include "example.h"
#include "external_documentation.h"
#include "helpers.h"
#include "info.h"
#include "link.h"
#include "parameter.h"
#include "path_item.h"
#include "paths.h"
#include "reference.h"
#include "request_body.h"
#include "response.h"
#include "security.h"
#include "server.h"
#include "tag.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t
#include <limits>      // std::numeric_limits
#include <map>         // std::map
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <utility>     // std::move, std::swap, std::unreachable
#include <vector>      // std::vector

namespace sourcemeta::core {

// OpenAPI Specification 3.1.1, Section 4.8.24.1: "The OpenAPI Schema Object
// dialect for this version of the specification is identified by the URI
// `https://spec.openapis.org/oas/3.1/dialect/base`"
constexpr auto OPENAPI_DIALECT_3_1{
    "https://spec.openapis.org/oas/3.1/dialect/base"sv};

// OpenAPI Specification 3.2.1, Section 4.24.1 names the same thing by a dated
// URI instead: it "is identified by the URI of the form
// `https://spec.openapis.org/oas/3.2/dialect/YYYY-MM-DD` [...] see the list of
// current schemas for the specific URI". One date is published for 3.2, and it
// is the one this repository already resolves.
//
// A document that declares 3.2.0 gets this one as well, although that patch
// reads "identified by the URI
// `https://spec.openapis.org/oas/3.1/dialect/base`" and so names the dialect of
// the revision before it. 3.2.1 Section 2.1 makes a revision the
// `major`.`minor` pair alone, so what a later patch of one says is what the
// whole of it says, and the earlier wording is a mistake that patch corrects
// rather than a rule of its own
constexpr auto OPENAPI_DIALECT_3_2{
    "https://spec.openapis.org/oas/3.2/dialect/2025-09-17"sv};

// The revision a document declares settles which dialect its Schema Objects
// fall back on, as each revision publishes one of its own
inline auto openapi_dialect(const OpenAPIVersion version) -> JSON::StringView {
  switch (version) {
    case OpenAPIVersion::OPENAPI_3_1:
      return OPENAPI_DIALECT_3_1;
    case OpenAPIVersion::OPENAPI_3_2:
      return OPENAPI_DIALECT_3_2;
  }

  std::unreachable();
}

constexpr auto OPENAPI_HASH_OPENAPI{JSON::Object::hash("openapi"sv)};
constexpr auto OPENAPI_HASH_JSON_SCHEMA_DIALECT{
    JSON::Object::hash("jsonSchemaDialect"sv)};

constexpr std::array<JSON::StringView, 10> OPENAPI_ROOT_FIELDS_3_1{
    {"openapi"sv, "info"sv, "jsonSchemaDialect"sv, "servers"sv, "paths"sv,
     "webhooks"sv, "components"sv, "security"sv, "tags"sv, "externalDocs"sv}};

// OpenAPI Specification 3.2.1, Section 4.1 adds `$self`, "the self-assigned
// URI of this document, which also serves as its base URI"
constexpr std::array<JSON::StringView, 11> OPENAPI_ROOT_FIELDS_3_2{
    {"openapi"sv, "info"sv, "jsonSchemaDialect"sv, "servers"sv, "paths"sv,
     "webhooks"sv, "components"sv, "security"sv, "tags"sv, "externalDocs"sv,
     "$self"sv}};

constexpr auto OPENAPI_HASH_SELF{JSON::Object::hash("$self"sv)};

inline auto openapi_check_document(const JSON &document, OpenAPIWalk &walk)
    -> void;

// What a document's `$self` establishes as its base, or nothing when it
// establishes none. OpenAPI Specification 3.2.1, Section 4.1: the field
// "provides the self-assigned URI of this document, which also serves as its
// base URI in accordance with RFC3986 Section 5.1.1", and 3.2.1
// Section 4.1.2.2.1: "If `$self` is a relative URI reference, it is resolved
// against the next possible base URI source ([RFC3986] Section 5.1.2
// [...] 5.1.4) before being used for the resolution of other relative URI
// references". That source is whatever base is in force here, which is the
// retrieval URI for the entry document and the URI a reference named for any
// other. RFC 3986
// Section 5.2.1 has only the scheme required of a base, so a relative `$self`
// with nothing absolute to resolve against establishes nothing, and
// Section 5.2.2 never resolves against a fragment, so one written here is
// dropped rather than read. The specification's own published schema turns such
// a `$self` down outright, which 3.2.1 Section 4 makes it no place to: "If the
// JSON Schema differs from this section, then this section MUST be considered
// authoritative", and the section it differs from asks only for a URI reference
inline auto openapi_document_base(const JSON::StringView self,
                                  const OpenAPIWalk &walk)
    -> std::optional<JSON::String> {
  auto target{openapi_reference_target(self, walk)};
  if (!target.has_value() || !target.value().scheme().has_value()) {
    return std::nullopt;
  }

  return target.value().recompose_without_fragment();
}

// OpenAPI Specification 3.1.1, Section 4.3.1 lists five ways an implementation
// MAY tell what a referenced document is, and this takes the second of them,
// "Detecting OpenAPI documents through the root `openapi` field". A document
// without one may be a bare holder of referenceable Objects or a Schema Object,
// and 3.1 gives no ground to turn either down
inline auto openapi_is_document(const JSON &document) -> bool {
  return document.is_object() &&
         document.try_at("openapi", OPENAPI_HASH_OPENAPI) != nullptr;
}

// Check an Object that a reference expected to find where it landed, which is
// somewhere in the document being read. The base pointer is where that Object
// sits, which is what keeps reading the same one twice from recording it twice
//
// A reference that expects a whole Description can no longer land anywhere, as
// only the document handed over is read, but the kind it would name is a kind
// all the same and this stays exhaustive over them
inline auto openapi_check_object(const OpenAPIObjectKind expected,
                                 const JSON &value, const Pointer &base,
                                 OpenAPIWalk &walk) -> void {
  switch (expected) {
    case OpenAPIObjectKind::Document:
      openapi_check_document(value, walk);
      return;
    case OpenAPIObjectKind::PathItem:
      openapi_check_path_item(value, base, walk);
      return;
    case OpenAPIObjectKind::Parameter:
      openapi_check_parameters_entry(value, base, walk);
      return;
    case OpenAPIObjectKind::RequestBody:
      openapi_check_request_body_or_reference(value, base, walk);
      return;
    case OpenAPIObjectKind::Response:
      openapi_check_response_or_reference(value, base, walk);
      return;
    case OpenAPIObjectKind::Example:
      openapi_check_example_or_reference(value, base, walk);
      return;
    case OpenAPIObjectKind::Header:
      openapi_check_header_or_reference(value, base, walk);
      return;
    case OpenAPIObjectKind::Link:
      openapi_check_link_or_reference(value, base, walk);
      return;
    case OpenAPIObjectKind::Callbacks:
      openapi_check_callbacks_or_reference(value, base, walk);
      return;
    case OpenAPIObjectKind::SecurityScheme:
      openapi_check_security_scheme_or_reference(value, base, walk);
      return;
    // What a reference from a `content` map or from the Components Object
    // expects from 3.2 onwards, and what nothing expects before that
    case OpenAPIObjectKind::MediaType:
      openapi_check_media_type_or_reference(value, base, walk);
      return;
    // Not what a `$ref` ever expects, but what a Link Object's
    // `operationRef` always does
    case OpenAPIObjectKind::Operation:
      openapi_check_operation(value, base, walk);
      return;
    default:
      // No other kind is ever what a reference expects to find
      std::unreachable();
  }
}

// A reference that stays within the document it was written in. There is
// nothing to resolve, since the document is already at hand, but the fragment
// still has to land on something and what it lands on still has to be the
// Object the reference position expects
inline auto openapi_follow_internal_reference(const URI &target,
                                              const bool names_a_fragment,
                                              const OpenAPIObjectKind expected,
                                              OpenAPIWalk &walk) -> void {
  // A reference naming no fragment names the document itself, which is an
  // OpenAPI Description and has already been read
  if (!names_a_fragment || walk.document == nullptr) {
    return;
  }

  // OpenAPI Specification 3.1.1, Section 4.6: "If the representation of the
  // referenced document is JSON or YAML, then the fragment identifier SHOULD
  // be interpreted as a JSON-Pointer as per RFC6901". That is a SHOULD rather
  // than a MUST, so a fragment shaped like anything else is left alone
  const auto pointer{fragment_to_pointer(target)};
  if (!pointer.has_value()) {
    return;
  }

  // A reference may point at another reference, and round again, so where this
  // has already been is remembered by the whole URI rather than by the
  // document alone
  if (!walk.visited.insert({target.recompose(), expected}).second) {
    return;
  }

  // Section 4.8.23 requires a `$ref` to "be in the form of a URI" and says
  // nothing about it having to resolve, so a fragment that lands on nothing is
  // not an error this specification lets us report. The reference is recorded
  // either way, and a frame holding one that lands nowhere does not stand
  // alone
  const auto *destination{try_get(*walk.document, pointer.value())};
  if (destination == nullptr) {
    return;
  }

  // What it lands on has to hold up as the Object the reference expected,
  // which is what catches a reference pointing at the wrong kind of thing
  openapi_check_object(expected, *destination, pointer.value(), walk);
}

// Where something written here lands, which is what it says resolved against
// the document it sits in, or nothing at all when those bytes are no URI
// reference
inline auto openapi_reference_target(const JSON::StringView reference,
                                     const OpenAPIWalk &walk)
    -> std::optional<URI> {
  return openapi_resolve_uri(reference, walk.base);
}

// Reading whatever a reference landed on, which is the same work wherever the
// reference was written. Recording it is the business of the caller, as a
// position that names another one through something other than a `$ref` member
// is not a reference the frame writes down
inline auto openapi_follow_target(const URI &target, const Pointer &origin,
                                  const OpenAPIObjectKind expected,
                                  OpenAPIWalk &walk) -> void {
  const auto identifier{target.recompose_without_fragment()};
  const auto names_a_fragment{target.fragment().has_value()};

  // A reference that leaves the document it was written in is recorded and
  // left at that. Framing reads the document it was handed rather than
  // fetching, so a description that spans more than one is one the frame does
  // not stand alone for. Both a bare fragment and the document's own URI
  // written out in full name the document being read
  if (identifier.has_value() && identifier.value() != walk.base) {
    return;
  }

  if (!names_a_fragment && walk.document != nullptr &&
      openapi_is_document(*walk.document)) {
    throw OpenAPIError{walk.base, origin,
                       "This reference must name a document that holds only "
                       "what the reference expects"};
  }

  openapi_follow_internal_reference(target, names_a_fragment, expected, walk);
}

inline auto openapi_follow_reference(const JSON::StringView reference,
                                     const Pointer &origin,
                                     const OpenAPIObjectKind expected,
                                     OpenAPIWalk &walk) -> void {
  const auto target{openapi_reference_target(reference, walk)};
  if (!target.has_value()) {
    // The value was already held to the form of a URI reference where it was
    // read, so what fails here is resolving it against the base, which leaves
    // nothing to record and nothing to land on
    return;
  }

  // Recorded before anything is decided about it, so that a reference nobody
  // follows is still one the description makes. It is keyed by the Object that
  // makes it rather than by its `$ref` member, so that where a reference comes
  // from is a location like any other, while an error still points at the
  // member that holds the problem
  walk.references.insert_or_assign(
      openapi_location_uri(walk.base, origin.initial()),
      OpenAPIReference{.original = JSON::String{reference},
                       .destination = target.value().recompose(),
                       .dangling = false,
                       .expected = expected,
                       .origin = origin});

  // OpenAPI Specification 3.2.1, Section 4.1.2: "all documents in an OAD MUST
  // have either an OpenAPI Object or a Schema Object at the root". A Schema
  // Object is what a Schema Object reference names, which never reaches here,
  // so every document that a reference of the shell may name holds an OpenAPI
  // Object. That is not one of the kinds any position here expects to find,
  // so a reference that names such a document whole has landed on the wrong
  // thing whatever kind it expected. Section 4.8.9 and Section 4.8.20 say as
  // much of the two positions they speak of, and the rest follows from what a
  // document may hold rather than from what those two sections single out.
  //
  // No revision of 3.1 says that much, so this holds one of its documents to
  // a rule its own text does not carry. What it does carry is a choice:
  // Section 4.3.1 of 3.1.1 reads "Implementations MAY support complete-document
  // parsing in any of the following ways", one of which is "Detecting a
  // document containing a referenceable Object at its root based on the
  // expected type of the reference". Reading a whole document as the Object a
  // reference wants is what that permits and what this declines, which leaves
  // one rule for both revisions rather than a 3.1 that takes what 3.2 forbids
  openapi_follow_target(target.value(), origin, expected, walk);
}

// OpenAPI Specification 3.1.1, Section 4.8.1: "This is the root object of the
// OpenAPI Description"
inline auto openapi_check_document(const JSON &document, OpenAPIWalk &walk)
    -> void {
  try {
    // Section 4.2: "An OpenAPI Document that conforms to the OpenAPI
    // Specification is itself a JSON object"
    if (!document.is_object()) {
      throw OpenAPIError{EMPTY_POINTER,
                         "The OpenAPI Description must be an object"};
    }

    // Section 4.8.1: "openapi | string | REQUIRED"
    const auto &version{openapi_require(
        document, "openapi"sv, OPENAPI_HASH_OPENAPI, EMPTY_POINTER,
        "The OpenAPI Description must declare its version")};

    if (!version.is_string()) {
      throw OpenAPIError{Pointer{"openapi"},
                         "The OpenAPI version must be a string"};
    }

    const auto revision{openapi_version(document)};
    if (!revision.has_value()) {
      throw OpenAPIError{Pointer{"openapi"},
                         "Unsupported OpenAPI Specification version"};
    }

    // Every field table below is the one this document's own revision
    // publishes, so which revision that is has to be settled before the first
    // of them is consulted
    walk.version = revision.value();

    // The version comes before the field table on purpose, so that a document
    // of another OpenAPI revision is told what it is rather than being
    // reported field by field
    openapi_reject_unknown_fields(
        document, OPENAPI_ROOT_FIELDS_3_1, OPENAPI_ROOT_FIELDS_3_2,
        EMPTY_POINTER, "The OpenAPI Object does not define this field", walk);

    // 3.2.1 Section 4.1: "$self | string | This string MUST be in the form of a
    // URI reference as defined by RFC3986 Section 4.1". Only 3.2 defines the
    // field, and the table above has already turned it down for anything
    // earlier. What it establishes is the base that every location in this
    // document is keyed by, so it is settled before anything records one
    const auto *self{document.try_at("$self", OPENAPI_HASH_SELF)};
    if (self != nullptr) {
      const auto reference{openapi_expect_uri_reference(
          *self, EMPTY_POINTER, "$self"sv,
          "The OpenAPI Description self identifier must be a string",
          "The OpenAPI Description self identifier must be a URI reference")};

      // A fragment written here is admitted, which the specification's own
      // published schema for this revision is stricter than. That schema
      // spells the field `{"format": "uri-reference", "pattern": "^[^#]*$"}`
      // and gives its reason in a comment of its own:
      //
      //   MUST NOT contain a fragment
      //
      // but 3.2.1 Section 4 settles which of the two answers for this: "This
      // text is the only normative description of the format. A JSON Schema is
      // hosted on spec.openapis.org for informational purposes. If the JSON
      // Schema differs from this section, then this section MUST be considered
      // authoritative". The text asks only for a URI reference, and RFC 3986
      // Section 4.1 admits a fragment in one, so the pattern is a rule the
      // normative prose does not carry and is not enforced here. Nothing is
      // lost by taking it, as Section 5.2.2 never resolves a reference against
      // a fragment, which is why what a fragment names is dropped rather than
      // read
      auto established{openapi_document_base(reference, walk)};
      if (established.has_value()) {
        walk.base = std::move(established.value());

        // 3.2.1 Section 4.1.1: "To ensure interoperability, references MUST use
        // the target document's `$self` URI if the `$self` field is present".
        // So this is the URI the document answers to, and one that names it by
        // where it was retrieved from instead names another document, which
        // the same paragraph calls "not interoperable" and NOT RECOMMENDED
      }
    }

    // Section 4.8.30: "The name used for each property MUST correspond to a
    // security scheme declared in the Security Schemes under the Components
    // Object". Section 4.3.3 leaves which document's Components Object that is
    // to the implementation and says "it is RECOMMENDED that tools resolve from
    // the entry document, rather than the current document", so the names come
    // from there. They are taken before anything else is read, as a document a
    // reference brings in may declare a requirement long before the entry
    // document's own components have been walked Section 4.8.10 has an
    // Operation Object's tags name Tag Objects "found under the root OpenAPI
    // Object", and Section 4.3.3 recommends the entry document for the same
    // reason it does for security schemes, so both sets of names come from
    // there and both come before anything else is read
    //
    // A document the description reaches takes those names from the entry
    // document and none of its own. Adding its own would let it name a scheme
    // that the description it becomes part of does not declare, which is a
    // requirement that reads fine here and cannot be met once the Object
    // holding it sits in the document that does describe the API
    if (!walk.referenced) {
      openapi_collect_security_schemes(document, walk);
      openapi_collect_tags(document, walk);
    }

    // OpenAPI Specification 3.2.1, Section 4.1 binds every document that holds
    // an OpenAPI Object: "In addition to the required fields, at least one of
    // the `components`, `paths`, or `webhooks` fields MUST be present".
    //
    // 3.1.1, Section 3.1 binds the description instead, and names what it is
    // made of while doing so: "An OpenAPI Description (OAD) [...] is composed
    // of an entry document [...] and any/all of its referenced documents [...]
    // and MUST contain at least one `paths` field, `components` field, or
    // `webhooks` field". 3.1.0 asked it of a document and 3.1.1 moved the
    // subject, so a document of that revision that another one reaches is free
    // to hold none of the three as long as the description holds one
    if ((!walk.referenced || walk.version == OpenAPIVersion::OPENAPI_3_2) &&
        document.try_at("paths", OPENAPI_HASH_PATHS) == nullptr &&
        document.try_at("components", OPENAPI_HASH_COMPONENTS) == nullptr &&
        document.try_at("webhooks", OPENAPI_HASH_WEBHOOKS) == nullptr) {
      throw OpenAPIError{
          EMPTY_POINTER,
          "The OpenAPI Description must declare paths, components or webhooks"};
    }

    // What the frame reports is what the entry document says, and an entry
    // document whose title and version are both the empty string is a legal
    // one, so which document this is has to be asked rather than inferred from
    // the values already held
    walk.info = openapi_parse_info(document, walk);

    // Section 4.8.1: "jsonSchemaDialect | string | The default value for the
    // `$schema` keyword within Schema Objects [...] This MUST be in the form
    // of a URI". It is checked rather than applied, as applying it is what
    // framing a Schema Object will do
    const auto *dialect{
        document.try_at("jsonSchemaDialect", OPENAPI_HASH_JSON_SCHEMA_DIALECT)};
    if (dialect != nullptr) {
      openapi_expect_uri_reference(
          *dialect, EMPTY_POINTER, "jsonSchemaDialect"sv,
          "The OpenAPI dialect must be a string",
          "The OpenAPI dialect must be a URI reference");
    }

    // Section 4.8.24.1: "To allow use of a different default `$schema` value
    // for all Schema Objects contained within an OAS document, a
    // `jsonSchemaDialect` value may be set within the OpenAPI Object. If this
    // default is not set, then the OAS dialect schema id MUST be used for
    // these Schema Objects". What a
    // Schema Object says about itself overrides this, which is a matter for
    // whatever reads inside one
    JSON::String effective_dialect{openapi_dialect(walk.version)};
    if (dialect != nullptr) {
      // Already checked above, so this parses
      URI resolved{dialect->to_string()};
      if (!walk.base.empty()) {
        resolved.resolve_from(URI{walk.base});
      }

      resolved.canonicalize();
      effective_dialect = resolved.recompose();
    }

    walk.dialect = effective_dialect;
    openapi_record(walk, EMPTY_POINTER, OpenAPIObjectKind::Document,
                   std::move(effective_dialect));

    openapi_check_servers(document, walk);
    openapi_check_root_external_documentation(document, walk);
    openapi_check_tags(document, walk);
    openapi_check_components(document, walk);
    openapi_check_paths(document, walk);
    openapi_check_webhooks(document, walk);

    const auto *security{document.try_at("security", OPENAPI_HASH_SECURITY)};
    if (security != nullptr) {
      auto locations{openapi_check_security(
          *security, Pointer{"security"},
          "The OpenAPI Description security must be an array", walk)};

      // Section 4.8.10 has an Operation Object override "any declared
      // top-level security", and Section 4.3.3 makes the entry document the one
      // that describes the API, so this is the declaration it overrides
      walk.security = std::move(locations);
    }
  } catch (const OpenAPIError &error) {
    // Every check reports where in the document the problem is, and this is
    // the only place that knows the base to name it by
    if (!error.base().empty() || walk.base.empty()) {
      throw;
    }

    throw OpenAPIError{walk.base, error.location(), error.what()};
  }
}

// OpenAPI Specification 3.2.1, Section 4.22, of a Tag Object's `parent`: "The
// named tag MUST exist in the API description, and circular references between
// parent and child tags MUST NOT be used". A description spans every document
// it references, so a parent naming a tag this one does not declare is only
// missing where nothing is missing, and a cycle is a property of the tags held
// rather than of any one tag
inline auto openapi_check_tag_parents(
    const OpenAPIWalk &walk,
    const std::map<JSON::String, OpenAPILocation> &locations, const bool whole)
    -> void {
  std::map<JSON::String, JSON::String> parents;
  for (const auto &[location, edge] : walk.tag_parents) {
    if (whole && !walk.tag_names.contains(edge.second)) {
      throw openapi_error_at(locations, location,
                             "The Tag Object parent must name a tag the "
                             "OpenAPI Description declares",
                             "parent"sv);
    }

    parents.insert_or_assign(edge.first, edge.second);
  }

  // Walking upward from each tag terminates at a tag with no parent unless the
  // chain comes back round, and a chain longer than the number of edges has
  // come back round
  for (const auto &[location, edge] : walk.tag_parents) {
    auto name{edge.first};
    for (std::size_t step = 0; step <= parents.size(); step += 1) {
      const auto next{parents.find(name)};
      if (next == parents.cend()) {
        break;
      }

      name = next->second;
      if (step == parents.size()) {
        throw openapi_error_at(locations, location,
                               "The Tag Object parents must not form a cycle",
                               "parent"sv);
      }
    }
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.20: "The identified or reference
// operation MUST be unique, and in the case of an `operationId`, it MUST be
// resolved within the scope of the OpenAPI Description". Section 4.3.3 goes on
// that "This requires parsing all referenced documents prior to determining an
// `operationId` to be unresolvable", so nothing is decided here until every
// document of the description is held at once
inline auto openapi_check_operation_id_links(
    const OpenAPIWalk &walk,
    const std::map<JSON::String, OpenAPILocation> &locations) -> void {
  for (const auto &[location, identifier] : walk.operation_id_links) {
    // Section 4.8.20 goes on to say that an operation reached through a Path
    // Item referenced more than once "cannot be resolved unambiguously", and
    // that "in such ambiguous cases, the resulting behavior is
    // implementation-defined and MAY result in an error". So naming nothing at
    // all is the violation, and naming something twice over is not
    if (!walk.operation_ids.contains(identifier)) {
      throw openapi_error_at(locations, location,
                             "The Link Object operation identifier must name "
                             "an operation the OpenAPI Description declares",
                             "operationId"sv);
    }
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.24 hands a Schema Object over to
// whatever reads JSON Schema, whole and on its own. One sitting within another
// is a place that implementation would be handed twice over, once by itself
// and once as part of something larger, which is not a reading it has any
// account of. Which places those are is settled by the walk as a whole, so
// this cannot be decided while one is still being read
inline auto openapi_check_schema_positions(const OpenAPIWalk &walk) -> void {
  for (const auto &location : walk.locations) {
    if (location.second.type != OpenAPIObjectKind::Schema) {
      continue;
    }

    // Which place holds this one is what the walk recorded of each place
    // above it rather than anything the order of them suggests. A location is
    // held under a key that sorts as a string, and Section 4.8.7 admits both
    // `-` and `.` into a component name, either of which falls below the `/`
    // that separates a place from what sits within it. So a sibling named
    // that way comes between an Object and its own contents, which is why
    // this asks after every place above rather than the one just read
    auto prefix{location.second.pointer};
    while (!prefix.empty()) {
      prefix.pop_back();
      const auto enclosing{
          walk.locations.find(openapi_location_uri(walk.base, prefix))};
      if (enclosing != walk.locations.cend() &&
          enclosing->second.type == OpenAPIObjectKind::Schema) {
        throw OpenAPIError{
            walk.base, location.second.pointer,
            "A Schema Object must not sit within another Schema Object"};
      }
    }
  }
}

// Every operation the description exposes, worked out from a walk that has
// settled. OpenAPI Specification 3.1.1, Section 4.8.9 has a templated path
// correspond to the path parameters the Path Item Object and its operations
// declare, and which parameters those are is only settled once every Path Item
// the description reaches is at hand, so this is where that is decided
auto openapi_project(const OpenAPIWalk &walk) -> std::vector<OpenAPIOperation>;

// Everything the checks need in order to start from nothing, which is a walk
// of the given document keyed by the given base. A 3.2 document may name
// itself, so what the walk ends up keyed by is what it reports rather than
// what it was handed
// Section 4.3.3: "For resolving component and tag name connections from a
// referenced (non-entry) document, it is RECOMMENDED that tools resolve from
// the entry document, rather than the current document. This allows Security
// Scheme Objects and Tag Objects to be defined next to the API's deployment
// information [...] and treated as an interface for referenced documents to
// access". A document read on its own has no entry document to resolve from,
// so what one names is settled by whoever reads it as part of a description
inline auto openapi_analyse(const JSON &document, JSON::String base,
                            const std::uint64_t max_locations =
                                std::numeric_limits<std::uint64_t>::max(),
                            const OpenAPIWalk *entry = nullptr) -> OpenAPIWalk {
  OpenAPIWalk walk{.base = base,
                   .retrieval = std::move(base),
                   .document = &document,
                   .operation_ids = {},
                   .visited = {},
                   .locations = {},
                   .references = {},
                   .parameters = {},
                   .path_items = {},
                   .operation_records = {},
                   .callbacks = {},
                   .endpoints = {},
                   .servers = {},
                   .security = {},
                   .security_schemes = {},
                   .security_references = {},
                   .tags = {},
                   .tag_parents = {},
                   .tag_names = {},
                   .operation_id_links = {},
                   .version = OpenAPIVersion::OPENAPI_3_1,
                   .dialect = {},
                   .info = {},
                   .remaining = max_locations,
                   .limit = max_locations};
  // The names of the entry document are in scope before this document's own
  // are read, as what it declares itself adds to them rather than replaces
  // them
  if (entry != nullptr) {
    walk.referenced = true;
    walk.security_schemes = entry->security_schemes;
    // Both of what a tag name settles come from there too, as the names a
    // parent may claim and the Tag Objects an operation resolves to are two
    // readings of one set rather than two sets
    walk.tags = entry->tags;
    walk.tag_names = entry->tag_names;
  }

  openapi_check_document(document, walk);
  openapi_check_schema_positions(walk);
  return walk;
}

} // namespace sourcemeta::core

#endif
