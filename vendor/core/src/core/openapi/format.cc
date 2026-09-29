#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/openapi.h>

#include "components.h"
#include "content.h"
#include "document.h"
#include "example.h"
#include "external_documentation.h"
#include "helpers.h"
#include "info.h"
#include "link.h"
#include "parameter.h"
#include "path_item.h"
#include "request_body.h"
#include "response.h"
#include "security.h"
#include "server.h"
#include "tag.h"

#include <algorithm> // std::ranges::find
#include <array>     // std::array, std::to_array
#include <cassert>   // assert
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint16_t
#include <iterator>  // std::distance
#include <limits>    // std::numeric_limits
#include <span>      // std::span
#include <utility>   // std::unreachable

namespace sourcemeta::core {

namespace {

// The order in which the fields of each Object are meant to appear, where the
// position of an entry is the rank of the field it names. Each table unions
// what 3.1 and 3.2 define, and what every variant of an Object defines, so
// nothing here branches on the revision a document declares.
//
// An entry of `x-` stands for every Specification Extension rather than for a
// field of that name, and it sits where it does because such a member is almost
// always an annotation. A kind that declares neither identity nor metadata
// therefore ranks it first, which is also where its size would put it
constexpr auto FIELDS_DOCUMENT{std::to_array<JSON::StringView>(
    {"openapi", "$self", "jsonSchemaDialect", "info", "x-", "externalDocs",
     "security", "servers", "tags", "webhooks", "paths", "components"})};

constexpr auto FIELDS_INFO{std::to_array<JSON::StringView>(
    {"title", "version", "summary", "description", "termsOfService", "x-",
     "contact", "license"})};

constexpr auto FIELDS_CONTACT{
    std::to_array<JSON::StringView>({"name", "x-", "url", "email"})};

constexpr auto FIELDS_LICENSE{
    std::to_array<JSON::StringView>({"name", "identifier", "x-", "url"})};

constexpr auto FIELDS_SERVER{std::to_array<JSON::StringView>(
    {"name", "description", "x-", "url", "variables"})};

constexpr auto FIELDS_SERVER_VARIABLE{
    std::to_array<JSON::StringView>({"description", "x-", "default", "enum"})};

constexpr auto FIELDS_COMPONENTS{std::to_array<JSON::StringView>(
    {"x-", "responses", "parameters", "examples", "requestBodies", "mediaTypes",
     "headers", "securitySchemes", "links", "callbacks", "pathItems",
     "schemas"})};

constexpr auto FIELDS_PATH_ITEM{std::to_array<JSON::StringView>(
    {"summary", "description", "x-", "$ref", "servers", "parameters", "get",
     "query", "head", "post", "put", "patch", "delete", "options", "trace",
     "additionalOperations"})};

constexpr auto FIELDS_OPERATION{std::to_array<JSON::StringView>(
    {"operationId", "summary", "description", "externalDocs", "deprecated",
     "x-", "tags", "security", "servers", "parameters", "requestBody",
     "responses", "callbacks"})};

constexpr auto FIELDS_PARAMETER{std::to_array<JSON::StringView>(
    {"name", "in", "description", "required", "deprecated", "example",
     "examples", "x-", "style", "explode", "allowEmptyValue", "allowReserved",
     "schema", "content"})};

constexpr auto FIELDS_REQUEST_BODY{std::to_array<JSON::StringView>(
    {"description", "required", "x-", "content"})};

constexpr auto FIELDS_RESPONSE{std::to_array<JSON::StringView>(
    {"summary", "description", "x-", "headers", "links", "content"})};

constexpr auto FIELDS_EXAMPLE{std::to_array<JSON::StringView>(
    {"summary", "description", "x-", "externalValue", "value", "dataValue",
     "serializedValue"})};

constexpr auto FIELDS_HEADER{std::to_array<JSON::StringView>(
    {"description", "required", "deprecated", "example", "examples", "x-",
     "style", "explode", "schema", "content"})};

constexpr auto FIELDS_LINK{std::to_array<JSON::StringView>(
    {"operationId", "operationRef", "description", "x-", "parameters", "server",
     "requestBody"})};

constexpr auto FIELDS_SECURITY_SCHEME{std::to_array<JSON::StringView>(
    {"type", "description", "deprecated", "x-", "name", "in", "scheme",
     "bearerFormat", "openIdConnectUrl", "oauth2MetadataUrl", "flows"})};

constexpr auto FIELDS_OAUTH_FLOWS{std::to_array<JSON::StringView>(
    {"x-", "implicit", "password", "clientCredentials", "authorizationCode",
     "deviceAuthorization"})};

constexpr auto FIELDS_OAUTH_FLOW{std::to_array<JSON::StringView>(
    {"x-", "authorizationUrl", "deviceAuthorizationUrl", "tokenUrl",
     "refreshUrl", "scopes"})};

constexpr auto FIELDS_TAG{
    std::to_array<JSON::StringView>({"name", "parent", "kind", "summary",
                                     "description", "externalDocs", "x-"})};

constexpr auto FIELDS_EXTERNAL_DOCS{
    std::to_array<JSON::StringView>({"description", "x-", "url"})};

constexpr auto FIELDS_ENCODING{std::to_array<JSON::StringView>(
    {"x-", "contentType", "style", "explode", "allowReserved", "headers",
     "encoding", "prefixEncoding", "itemEncoding"})};

constexpr auto FIELDS_MEDIA_TYPE{std::to_array<JSON::StringView>(
    {"example", "examples", "x-", "encoding", "prefixEncoding", "itemEncoding",
     "schema", "itemSchema"})};

// OpenAPI Specification 3.1.1, Section 4.8.23: "This object cannot be extended
// with additional properties, and any properties added SHALL be ignored".
// Ignoring is what the specification asks for, so framing lets such a member
// through and this is the one table that is not total
constexpr auto FIELDS_REFERENCE{
    std::to_array<JSON::StringView>({"summary", "description", "$ref"})};

// The fields of each Object whose value is a map the description's author keys,
// so that what it holds is sorted rather than ranked. A map that the frame
// reports as an Object of its own is absent from these, as it sorts itself
constexpr auto MAPS_DOCUMENT{std::to_array<JSON::StringView>({"webhooks"})};

constexpr auto MAPS_SERVER{std::to_array<JSON::StringView>({"variables"})};

constexpr auto MAPS_COMPONENTS{std::to_array<JSON::StringView>(
    {"schemas", "responses", "parameters", "examples", "requestBodies",
     "headers", "securitySchemes", "links", "callbacks", "pathItems",
     "mediaTypes"})};

constexpr auto MAPS_PATH_ITEM{
    std::to_array<JSON::StringView>({"additionalOperations"})};

constexpr auto MAPS_OPERATION{std::to_array<JSON::StringView>({"callbacks"})};

constexpr auto MAPS_PARAMETER{
    std::to_array<JSON::StringView>({"examples", "content"})};

constexpr auto MAPS_REQUEST_BODY{std::to_array<JSON::StringView>({"content"})};

constexpr auto MAPS_RESPONSE{
    std::to_array<JSON::StringView>({"headers", "links", "content"})};

constexpr auto MAPS_HEADER{
    std::to_array<JSON::StringView>({"examples", "content"})};

constexpr auto MAPS_LINK{std::to_array<JSON::StringView>({"parameters"})};

constexpr auto MAPS_OAUTH_FLOW{std::to_array<JSON::StringView>({"scopes"})};

constexpr auto MAPS_ENCODING{
    std::to_array<JSON::StringView>({"headers", "encoding"})};

constexpr auto MAPS_MEDIA_TYPE{
    std::to_array<JSON::StringView>({"examples", "encoding"})};

// A table is total when it ranks every field the specification defines for the
// Object, which is what the arrays framing holds a document to spell out. So a
// document that framed cannot hold a field that reaches no rank.
//
// These two are immediate functions rather than merely constant ones because
// the assertions below are the only callers there will ever be. Asking the
// compiler to hold them to that leaves no runtime symbol for anything to
// wonder why the tests never reach
template <std::size_t Fields, std::size_t Admitted>
consteval auto
ranks_every_field(const std::array<JSON::StringView, Fields> &fields,
                  const std::array<JSON::StringView, Admitted> &admitted)
    -> bool {
  for (const auto &field : admitted) {
    if (std::ranges::find(fields, field) == fields.cend()) {
      return false;
    }
  }

  return true;
}

// And it names nothing else, which is what catches a table misspelling a field
// into a rank that nothing ever reaches
template <std::size_t Fields, typename... Admitted>
consteval auto
defines_every_rank(const std::array<JSON::StringView, Fields> &fields,
                   const Admitted &...admitted) -> bool {
  for (const auto &field : fields) {
    if (field == OPENAPI_EXTENSION_PREFIX) {
      continue;
    }

    if (!(... || (std::ranges::find(admitted, field) != admitted.cend()))) {
      return false;
    }
  }

  return true;
}

static_assert(ranks_every_field(FIELDS_DOCUMENT, OPENAPI_ROOT_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_DOCUMENT, OPENAPI_ROOT_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_DOCUMENT, OPENAPI_ROOT_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_INFO, OPENAPI_INFO_FIELDS));
static_assert(defines_every_rank(FIELDS_INFO, OPENAPI_INFO_FIELDS));

static_assert(ranks_every_field(FIELDS_CONTACT, OPENAPI_CONTACT_FIELDS));
static_assert(defines_every_rank(FIELDS_CONTACT, OPENAPI_CONTACT_FIELDS));

static_assert(ranks_every_field(FIELDS_LICENSE, OPENAPI_LICENSE_FIELDS));
static_assert(defines_every_rank(FIELDS_LICENSE, OPENAPI_LICENSE_FIELDS));

static_assert(ranks_every_field(FIELDS_SERVER, OPENAPI_SERVER_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_SERVER, OPENAPI_SERVER_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_SERVER, OPENAPI_SERVER_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_SERVER_VARIABLE,
                                OPENAPI_SERVER_VARIABLE_FIELDS));
static_assert(defines_every_rank(FIELDS_SERVER_VARIABLE,
                                 OPENAPI_SERVER_VARIABLE_FIELDS));

static_assert(ranks_every_field(FIELDS_COMPONENTS,
                                OPENAPI_COMPONENTS_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_COMPONENTS,
                                OPENAPI_COMPONENTS_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_COMPONENTS,
                                 OPENAPI_COMPONENTS_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_PATH_ITEM,
                                OPENAPI_PATH_ITEM_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_PATH_ITEM,
                                OPENAPI_PATH_ITEM_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_PATH_ITEM,
                                 OPENAPI_PATH_ITEM_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_OPERATION, OPENAPI_OPERATION_FIELDS));
static_assert(defines_every_rank(FIELDS_OPERATION, OPENAPI_OPERATION_FIELDS));

static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_CONTENT_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_CONTENT_FIELDS_3_2));
static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_SCHEMA_FIELDS));
static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_RESERVED_SCHEMA_FIELDS_3_2));
static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_QUERY_SCHEMA_FIELDS));
static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_PARAMETER,
                                OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_PARAMETER,
                                 OPENAPI_PARAMETER_QUERY_SCHEMA_FIELDS,
                                 OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_REQUEST_BODY,
                                OPENAPI_REQUEST_BODY_FIELDS));
static_assert(defines_every_rank(FIELDS_REQUEST_BODY,
                                 OPENAPI_REQUEST_BODY_FIELDS));

static_assert(ranks_every_field(FIELDS_RESPONSE, OPENAPI_RESPONSE_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_RESPONSE, OPENAPI_RESPONSE_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_RESPONSE, OPENAPI_RESPONSE_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_EXAMPLE, OPENAPI_EXAMPLE_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_EXAMPLE, OPENAPI_EXAMPLE_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_EXAMPLE, OPENAPI_EXAMPLE_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_HEADER, OPENAPI_HEADER_SCHEMA_FIELDS));
static_assert(ranks_every_field(FIELDS_HEADER,
                                OPENAPI_HEADER_CONTENT_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_HEADER,
                                OPENAPI_HEADER_CONTENT_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_HEADER, OPENAPI_HEADER_SCHEMA_FIELDS,
                                 OPENAPI_HEADER_CONTENT_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_LINK, OPENAPI_LINK_FIELDS));
static_assert(defines_every_rank(FIELDS_LINK, OPENAPI_LINK_FIELDS));

static_assert(ranks_every_field(FIELDS_SECURITY_SCHEME,
                                OPENAPI_SECURITY_SCHEME_APIKEY_FIELDS_3_2));
static_assert(ranks_every_field(
    FIELDS_SECURITY_SCHEME, OPENAPI_SECURITY_SCHEME_HTTP_BEARER_FIELDS_3_2));
static_assert(ranks_every_field(FIELDS_SECURITY_SCHEME,
                                OPENAPI_SECURITY_SCHEME_OAUTH2_FIELDS_3_2));
static_assert(ranks_every_field(FIELDS_SECURITY_SCHEME,
                                OPENAPI_SECURITY_SCHEME_OIDC_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_SECURITY_SCHEME,
                                 OPENAPI_SECURITY_SCHEME_APIKEY_FIELDS_3_2,
                                 OPENAPI_SECURITY_SCHEME_HTTP_BEARER_FIELDS_3_2,
                                 OPENAPI_SECURITY_SCHEME_OAUTH2_FIELDS_3_2,
                                 OPENAPI_SECURITY_SCHEME_OIDC_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_OAUTH_FLOWS,
                                OPENAPI_OAUTH_FLOWS_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_OAUTH_FLOWS,
                                OPENAPI_OAUTH_FLOWS_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_OAUTH_FLOWS,
                                 OPENAPI_OAUTH_FLOWS_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_OAUTH_FLOW,
                                OPENAPI_OAUTH_FLOW_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_OAUTH_FLOW,
                                OPENAPI_OAUTH_FLOW_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_OAUTH_FLOW,
                                 OPENAPI_OAUTH_FLOW_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_TAG, OPENAPI_TAG_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_TAG, OPENAPI_TAG_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_TAG, OPENAPI_TAG_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_EXTERNAL_DOCS,
                                OPENAPI_EXTERNAL_DOCS_FIELDS));
static_assert(defines_every_rank(FIELDS_EXTERNAL_DOCS,
                                 OPENAPI_EXTERNAL_DOCS_FIELDS));

static_assert(ranks_every_field(FIELDS_ENCODING, OPENAPI_ENCODING_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_ENCODING, OPENAPI_ENCODING_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_ENCODING, OPENAPI_ENCODING_FIELDS_3_2));

static_assert(ranks_every_field(FIELDS_MEDIA_TYPE,
                                OPENAPI_MEDIA_TYPE_FIELDS_3_1));
static_assert(ranks_every_field(FIELDS_MEDIA_TYPE,
                                OPENAPI_MEDIA_TYPE_FIELDS_3_2));
static_assert(defines_every_rank(FIELDS_MEDIA_TYPE,
                                 OPENAPI_MEDIA_TYPE_FIELDS_3_2));

// A map a table names has to be a field the same Object ranks, or the lookup
// that sorts it would never find anything
static_assert(ranks_every_field(FIELDS_DOCUMENT, MAPS_DOCUMENT));
static_assert(ranks_every_field(FIELDS_SERVER, MAPS_SERVER));
static_assert(ranks_every_field(FIELDS_COMPONENTS, MAPS_COMPONENTS));
static_assert(ranks_every_field(FIELDS_PATH_ITEM, MAPS_PATH_ITEM));
static_assert(ranks_every_field(FIELDS_OPERATION, MAPS_OPERATION));
static_assert(ranks_every_field(FIELDS_PARAMETER, MAPS_PARAMETER));
static_assert(ranks_every_field(FIELDS_REQUEST_BODY, MAPS_REQUEST_BODY));
static_assert(ranks_every_field(FIELDS_RESPONSE, MAPS_RESPONSE));
static_assert(ranks_every_field(FIELDS_HEADER, MAPS_HEADER));
static_assert(ranks_every_field(FIELDS_LINK, MAPS_LINK));
static_assert(ranks_every_field(FIELDS_OAUTH_FLOW, MAPS_OAUTH_FLOW));
static_assert(ranks_every_field(FIELDS_ENCODING, MAPS_ENCODING));
static_assert(ranks_every_field(FIELDS_MEDIA_TYPE, MAPS_MEDIA_TYPE));

// What ordering a kind of Object asks for: how to rank its fields, and which of
// them hold a map whose keys the description's author chose
struct KindRow {
  std::span<const JSON::StringView> fields;
  std::span<const JSON::StringView> maps;
  bool sorts_own_keys{false};
};

auto row_of(const OpenAPIObjectKind kind) -> KindRow {
  switch (kind) {
    case OpenAPIObjectKind::Document:
      return {.fields = FIELDS_DOCUMENT, .maps = MAPS_DOCUMENT};
    case OpenAPIObjectKind::Info:
      return {.fields = FIELDS_INFO, .maps = {}};
    case OpenAPIObjectKind::Contact:
      return {.fields = FIELDS_CONTACT, .maps = {}};
    case OpenAPIObjectKind::License:
      return {.fields = FIELDS_LICENSE, .maps = {}};
    case OpenAPIObjectKind::Server:
      return {.fields = FIELDS_SERVER, .maps = MAPS_SERVER};
    case OpenAPIObjectKind::ServerVariable:
      return {.fields = FIELDS_SERVER_VARIABLE, .maps = {}};
    case OpenAPIObjectKind::Components:
      return {.fields = FIELDS_COMPONENTS, .maps = MAPS_COMPONENTS};
    case OpenAPIObjectKind::PathItem:
      return {.fields = FIELDS_PATH_ITEM, .maps = MAPS_PATH_ITEM};
    case OpenAPIObjectKind::Operation:
      return {.fields = FIELDS_OPERATION, .maps = MAPS_OPERATION};
    case OpenAPIObjectKind::Parameter:
      return {.fields = FIELDS_PARAMETER, .maps = MAPS_PARAMETER};
    case OpenAPIObjectKind::RequestBody:
      return {.fields = FIELDS_REQUEST_BODY, .maps = MAPS_REQUEST_BODY};
    case OpenAPIObjectKind::Response:
      return {.fields = FIELDS_RESPONSE, .maps = MAPS_RESPONSE};
    case OpenAPIObjectKind::Example:
      return {.fields = FIELDS_EXAMPLE, .maps = {}};
    case OpenAPIObjectKind::Header:
      return {.fields = FIELDS_HEADER, .maps = MAPS_HEADER};
    case OpenAPIObjectKind::Link:
      return {.fields = FIELDS_LINK, .maps = MAPS_LINK};
    case OpenAPIObjectKind::SecurityScheme:
      return {.fields = FIELDS_SECURITY_SCHEME, .maps = {}};
    case OpenAPIObjectKind::OAuthFlows:
      return {.fields = FIELDS_OAUTH_FLOWS, .maps = {}};
    case OpenAPIObjectKind::OAuthFlow:
      return {.fields = FIELDS_OAUTH_FLOW, .maps = MAPS_OAUTH_FLOW};
    case OpenAPIObjectKind::Tag:
      return {.fields = FIELDS_TAG, .maps = {}};
    case OpenAPIObjectKind::ExternalDocumentation:
      return {.fields = FIELDS_EXTERNAL_DOCS, .maps = {}};
    case OpenAPIObjectKind::Encoding:
      return {.fields = FIELDS_ENCODING, .maps = MAPS_ENCODING};
    case OpenAPIObjectKind::MediaType:
      return {.fields = FIELDS_MEDIA_TYPE, .maps = MAPS_MEDIA_TYPE};
    case OpenAPIObjectKind::Reference:
      return {.fields = FIELDS_REFERENCE, .maps = {}};
    // The four Objects this specification defines as a map alone, whose keys
    // are a path template, a status code, a runtime expression and the name of
    // a security scheme. There is no field to rank, so these sort themselves
    case OpenAPIObjectKind::Paths:
    case OpenAPIObjectKind::Responses:
    case OpenAPIObjectKind::Callbacks:
    case OpenAPIObjectKind::SecurityRequirement:
      return {.fields = {}, .maps = {}, .sorts_own_keys = true};
    // Handled before this is ever asked
    case OpenAPIObjectKind::Schema:
      return {};
  }

  std::unreachable();
}

// Byte order on the key, which is the whole of what a map whose keys the author
// chose needs. In a Responses Object it groups each class of status code with
// the range that generalises it, so `200` and `201` sort before `2XX` and that
// whole group sorts before `300`, while `default` lands after every code and
// every range because a letter follows every digit, and the extensions land
// last. In a Paths Object it puts every path before the extensions beside them,
// as a path begins with a slash
auto compare_keys(const JSON::String &left, const JSON::String &right) -> bool {
  return left < right;
}

// The rank of a field within a kind's table, where every Specification
// Extension takes the rank of the placeholder entry that stands for all of
// them. A field the table does not name sorts last, which only a Reference
// Object can reach
struct FieldComparison {
  std::span<const JSON::StringView> fields;

  [[nodiscard]] auto rank(const JSON::String &field) const -> std::uint16_t {
    constexpr auto UNRECOGNISED{std::numeric_limits<std::uint16_t>::max()};
    const auto match{std::ranges::find(
        this->fields, field.starts_with(OPENAPI_EXTENSION_PREFIX)
                          ? OPENAPI_EXTENSION_PREFIX
                          : JSON::StringView{field})};
    if (match == this->fields.end()) {
      return UNRECOGNISED;
    }

    return static_cast<std::uint16_t>(
        std::distance(this->fields.begin(), match));
  }

  auto operator()(const JSON::String &left, const JSON::String &right) const
      -> bool {
    const auto left_rank{this->rank(left)};
    const auto right_rank{this->rank(right)};
    if (left_rank == right_rank) {
      return left < right;
    }

    return left_rank < right_rank;
  }
};

auto format_object(JSON &document, const OpenAPIFrame::Location &location)
    -> void {
  // What sits inside a Schema Object is JSON Schema's to order, and the pass
  // over the schemas has already done it. Ranking one here against an empty
  // table would put its keywords in alphabetical order and undo that
  if (location.type == OpenAPIObjectKind::Schema) {
    return;
  }

  auto &object{get(document, location.pointer)};
  // A position the frame reports may hold something other than an object, as a
  // Path Item Object holding nothing but a reference is read as the Object it
  // stands in for
  if (!object.is_object()) {
    return;
  }

  const auto row{row_of(location.type)};
  if (row.sorts_own_keys) {
    object.reorder(compare_keys);
    return;
  }

  object.reorder(FieldComparison{.fields = row.fields});
  for (const auto &field : row.maps) {
    auto *map{object.try_at(field)};
    if (map != nullptr && map->is_object()) {
      map->reorder(compare_keys);
    }
  }
}

} // namespace

auto openapi_format(JSON &document, const OpenAPIFrame &frame) -> void {
  assert(document.is_object());

  // OpenAPI Specification 3.1.1, Section 4.3 leaves a Schema Object to JSON
  // Schema, so ordering one is that implementation's to do. It goes first
  // because the frame of those schemas holds pointers that borrow this
  // document's own property names, and reordering an Object that holds a schema
  // would leave them naming something else. Nothing this specification defines
  // sits inside a Schema Object, so every schema position is deeper than the
  // Object holding it and doing this first is enough to be safe
  schema_format(document, frame.schemas());

  // Unlike the schema frame above, these locations own the pointers they
  // report, so reordering one Object leaves the rest addressable and no order
  // among them is required
  frame.for_each_object(
      [&document](const auto &, const auto &location) -> void {
        format_object(document, location);
      });
}

} // namespace sourcemeta::core
