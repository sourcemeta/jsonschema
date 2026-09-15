#ifndef SOURCEMETA_CORE_OPENAPI_LINK_H_
#define SOURCEMETA_CORE_OPENAPI_LINK_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"
#include "reference.h"
#include "server.h"

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_OPERATION_REF{JSON::Object::hash("operationRef"sv)};
constexpr auto OPENAPI_HASH_OPERATION_ID{JSON::Object::hash("operationId"sv)};
constexpr auto OPENAPI_HASH_PARAMETERS{JSON::Object::hash("parameters"sv)};
constexpr auto OPENAPI_HASH_REQUEST_BODY{JSON::Object::hash("requestBody"sv)};
constexpr auto OPENAPI_HASH_SERVER{JSON::Object::hash("server"sv)};

constexpr std::array<JSON::StringView, 6> OPENAPI_LINK_FIELDS{
    {"operationRef"sv, "operationId"sv, "parameters"sv, "requestBody"sv,
     "description"sv, "server"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.20: "The Link Object represents a
// possible design-time link for a response"
inline auto openapi_check_link(const JSON &value, const Pointer &base,
                               OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Link);
  openapi_expect_object(value, base, "The Link Object must be an object");
  openapi_reject_unknown_fields(value, OPENAPI_LINK_FIELDS, base,
                                "The Link Object does not define this field");

  // OpenAPI Specification 3.1.1, Section 4.8.20: "operationRef | string | A
  // URI reference to an OAS operation. This field is mutually exclusive of the
  // `operationId` field"
  const auto *operation_reference{
      value.try_at("operationRef", OPENAPI_HASH_OPERATION_REF)};
  const auto *operation_identifier{
      value.try_at("operationId", OPENAPI_HASH_OPERATION_ID)};

  if (operation_reference != nullptr && operation_identifier != nullptr) {
    throw OpenAPIError{base,
                       "The Link Object operation reference and operation "
                       "identifier are mutually exclusive"};
  }

  if (operation_reference == nullptr && operation_identifier == nullptr) {
    throw OpenAPIError{base,
                       "The Link Object must declare an operation reference or "
                       "an operation identifier"};
  }

  // Section 4.8.20: "The identified or reference operation MUST be unique, and
  // in the case of an `operationId`, it MUST be resolved within the scope of
  // the OpenAPI Description". What operations the description declares is not
  // known until every document has been read, so this is only written down
  if (operation_identifier != nullptr) {
    const auto identifier{openapi_expect_string(
        *operation_identifier, base, "operationId"sv,
        "The Link Object operation identifier must be a string")};
    walk.operation_id_links.insert_or_assign(
        openapi_location_uri(walk.base, base), JSON::String{identifier});
  }

  // OpenAPI Specification 3.1.1, Section 4.8.20: "parameters | Map[string,
  // Any | {expression}]". Any is any JSON value, so a constant of any type is
  // as legal as a runtime expression and only the map itself is checked
  const auto *parameters{value.try_at("parameters", OPENAPI_HASH_PARAMETERS)};
  if (parameters != nullptr) {
    openapi_expect_object(*parameters, openapi_child(base, "parameters"sv),
                          "The Link Object parameters must be an object");
  }

  openapi_check_optional_string(value, base, "description"sv,
                                OPENAPI_HASH_DESCRIPTION,
                                "The Link Object description must be a string");

  const auto *server{value.try_at("server", OPENAPI_HASH_SERVER)};
  if (server != nullptr) {
    openapi_check_server(*server, openapi_child(base, "server"sv), walk);
  }

  // Any value at all is a legal `requestBody`, so there is nothing to check

  // Section 4.8.20: "operationRef | string | A URI reference to an OAS
  // operation [...] and MUST point to an Operation Object". So unlike every
  // other URI-valued field this one is a reference to follow, and what it
  // lands on has to hold up as an Operation Object. Followed last, so that the
  // Link Object's own fields are settled before anything it points at is read
  if (operation_reference != nullptr) {
    const auto reference{openapi_expect_uri_reference(
        *operation_reference, base, "operationRef"sv,
        "The Link Object operation reference must be a string",
        "The Link Object operation reference must be a URI reference")};
    openapi_follow_reference(reference, openapi_child(base, "operationRef"sv),
                             OpenAPIObjectKind::Operation, walk);
  }
}

inline auto openapi_check_link_or_reference(const JSON &value,
                                            const Pointer &base,
                                            OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::Link, openapi_check_link>(
      value, base, walk);
}

} // namespace sourcemeta::core

#endif
