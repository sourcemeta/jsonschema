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
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <utility>     // std::move, std::swap, std::unreachable

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
// is the one this repository already resolves
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
// base URI in accordance with RFC3986 Section 5.1.1", and Section 4.7.1.1: "If
// `$self` is a relative URI reference, it is resolved against the next
// possible base URI source before being used". That next source is whatever
// base is in force here, which is the retrieval URI for the entry document and
// the URI a reference named for any other. RFC 3986 Section 5.2.1 has only the
// scheme required of a base, so a relative `$self` with nothing absolute to
// resolve against establishes nothing, and Section 5.2.2 never resolves
// against a fragment, so one written here is no part of the base either
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
  try {
    URI target{JSON::String{reference}};
    if (!walk.base.empty()) {
      target.resolve_from(URI{walk.base});
    }

    // Canonicalising here is what makes two spellings of one place one place,
    // both to the set that remembers where the walk has been and to a caller
    // comparing a destination against a location
    target.canonicalize();
    return target;
  } catch (const URIParseError &) {
    return std::nullopt;
  }
}

// Reading whatever a reference landed on, which is the same work wherever the
// reference was written. Recording it is the business of the caller, as a
// position that names another one through something other than a `$ref` member
// is not a reference the frame writes down
inline auto openapi_follow_target(const URI &target, const Pointer &origin,
                                  const OpenAPIObjectKind expected,
                                  const bool demands_its_own_kind,
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

  if (demands_its_own_kind && !names_a_fragment && walk.document != nullptr &&
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
                       .destination = target.value().recompose()});

  // Section 4.8.9, of a Path Item Object's `$ref`: "the referenced structure
  // MUST be in the form of a Path Item Object", and Section 4.8.20, of a Link
  // Object's `operationRef`: it "MUST point to an Operation Object". A
  // document that declares a root `openapi` field is an OpenAPI Description
  // and neither of those, so a reference from one of those two positions that
  // names such a document whole has landed on the wrong thing. Section 4.3.1's
  // detection settles how a document is read, which is a separate question
  // from whether a reference was allowed to point at it. Section 4.8.23 holds
  // a Reference Object to nothing but the form of a URI, and those two
  // positions are the only ones a `$ref` reaches either kind from, so what
  // the demand really follows is the position rather than the kind
  openapi_follow_target(target.value(), origin, expected,
                        expected == OpenAPIObjectKind::PathItem ||
                            expected == OpenAPIObjectKind::Operation,
                        walk);
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

    // Section 4.1: "$self | string | This string MUST be in the form of a URI
    // reference as defined by RFC3986 Section 4.1". Only 3.2 defines the
    // field, and the table above has already turned it down for anything
    // earlier. What it establishes is the base that every location in this
    // document is keyed by, so it is settled before anything records one
    const auto *self{document.try_at("$self", OPENAPI_HASH_SELF)};
    if (self != nullptr) {
      const auto reference{openapi_expect_uri_reference(
          *self, EMPTY_POINTER, "$self"sv,
          "The OpenAPI Description self identifier must be a string",
          "The OpenAPI Description self identifier must be a URI reference")};

      // The specification's own published schema for this revision spells the
      // field `{"format": "uri-reference", "pattern": "^[^#]*$"}` and says
      // why in a comment of its own:
      //
      //   MUST NOT contain a fragment
      //
      // which RFC 3986 Section 5.1 agrees with, a base URI carrying none. The
      // pattern turns down the character rather than a fragment component, so
      // an empty one is refused here too
      if (reference.find('#') != JSON::StringView::npos) {
        throw OpenAPIError{
            walk.base, openapi_child(EMPTY_POINTER, "$self"sv),
            "The OpenAPI Description self identifier must not contain a "
            "fragment"};
      }

      auto established{openapi_document_base(reference, walk)};
      if (established.has_value()) {
        walk.base = std::move(established.value());

        // Section 4.7.1: "To ensure interoperability, references MUST use the
        // target document's `$self` URI if the `$self` field is present". So
        // this is the URI the document answers to, and one that names it by
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
    openapi_collect_security_schemes(document, walk);
    openapi_collect_tags(document, walk);

    // Section 3.1: an OpenAPI Description "MUST contain at least one paths
    // field, components field, or webhooks field"
    if (document.try_at("paths", OPENAPI_HASH_PATHS) == nullptr &&
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
    // default is not set, then the OAS dialect schema id MUST be used". What a
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

} // namespace sourcemeta::core

#endif
