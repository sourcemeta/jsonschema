#ifndef SOURCEMETA_CORE_OPENAPI_PATH_ITEM_H_
#define SOURCEMETA_CORE_OPENAPI_PATH_ITEM_H_

#include <sourcemeta/core/openapi.h>

#include "external_documentation.h"
#include "helpers.h"
#include "parameter.h"
#include "reference.h"
#include "request_body.h"
#include "response.h"
#include "security.h"
#include "server.h"

#include <sourcemeta/core/http.h>

#include <algorithm>   // std::ranges::find
#include <array>       // std::array
#include <set>         // std::set
#include <string_view> // std::string_view
#include <utility>     // std::move, std::pair
#include <vector>      // std::vector

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_RESPONSES{JSON::Object::hash("responses"sv)};
constexpr auto OPENAPI_HASH_CALLBACKS{JSON::Object::hash("callbacks"sv)};

constexpr std::array<JSON::StringView, 13> OPENAPI_PATH_ITEM_FIELDS_3_1{
    {"$ref"sv, "summary"sv, "description"sv, "servers"sv, "parameters"sv,
     "get"sv, "put"sv, "post"sv, "delete"sv, "options"sv, "head"sv, "patch"sv,
     "trace"sv}};

// OpenAPI Specification 3.2.1, Section 4.9 adds `query`, "a definition of a
// QUERY operation, as defined in RFC10008", and `additionalOperations`, "a map
// of additional operations on this path"
constexpr std::array<JSON::StringView, 15> OPENAPI_PATH_ITEM_FIELDS_3_2{
    {"$ref"sv, "summary"sv, "description"sv, "servers"sv, "parameters"sv,
     "get"sv, "put"sv, "post"sv, "delete"sv, "options"sv, "head"sv, "patch"sv,
     "trace"sv, "query"sv, "additionalOperations"sv}};

// The eight of 3.1 plus the `query` of 3.2, which names the QUERY method of
// RFC 10008. Looping over all nine under either revision is safe because the
// field table above has already turned down a `query` in a 3.1 document
constexpr std::array<OpenAPIField, 9> OPENAPI_PATH_ITEM_METHODS{
    {{.name = "get"sv, .hash = JSON::Object::hash("get"sv)},
     {.name = "put"sv, .hash = JSON::Object::hash("put"sv)},
     {.name = "post"sv, .hash = JSON::Object::hash("post"sv)},
     {.name = "delete"sv, .hash = JSON::Object::hash("delete"sv)},
     {.name = "options"sv, .hash = JSON::Object::hash("options"sv)},
     {.name = "head"sv, .hash = JSON::Object::hash("head"sv)},
     {.name = "patch"sv, .hash = JSON::Object::hash("patch"sv)},
     {.name = "trace"sv, .hash = JSON::Object::hash("trace"sv)},
     {.name = "query"sv, .hash = JSON::Object::hash("query"sv)}}};

// The methods those nine fields define, which the same section spells in
// uppercase in each of their descriptions, and which an `additionalOperations`
// key is therefore held against as written
constexpr std::array<JSON::StringView, 9> OPENAPI_PATH_ITEM_METHOD_NAMES{
    {"GET"sv, "PUT"sv, "POST"sv, "DELETE"sv, "OPTIONS"sv, "HEAD"sv, "PATCH"sv,
     "TRACE"sv, "QUERY"sv}};

constexpr auto OPENAPI_HASH_ADDITIONAL_OPERATIONS{
    JSON::Object::hash("additionalOperations"sv)};

constexpr std::array<JSON::StringView, 12> OPENAPI_OPERATION_FIELDS{
    {"tags"sv, "summary"sv, "description"sv, "externalDocs"sv, "operationId"sv,
     "parameters"sv, "requestBody"sv, "responses"sv, "callbacks"sv,
     "deprecated"sv, "security"sv, "servers"sv}};

// A Callback Object holds Path Item Objects, and a Path Item Object reaches
// back here through its Operations, so the two have to be declared apart
inline auto openapi_check_path_item(const JSON &value, const Pointer &base,
                                    OpenAPIWalk &walk) -> void;

// OpenAPI Specification 3.1.1, Section 4.8.18: "A map of possible out-of band
// callbacks related to the parent operation". Its keys are runtime
// expressions, which carry no requirement this Object can check
inline auto openapi_check_callbacks(const JSON &value, const Pointer &base,
                                    OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Callbacks);
  openapi_expect_object(value, base, "The Callback Object must be an object");
  std::vector<std::pair<JSON::String, JSON::String>> entries;
  for (const auto &entry : value.as_object()) {
    if (entry.first.starts_with(OPENAPI_EXTENSION_PREFIX)) {
      continue;
    }

    const auto location{openapi_child(base, entry.first)};
    openapi_check_path_item(entry.second, location, walk);
    entries.emplace_back(entry.first,
                         openapi_location_uri(walk.base, location));
  }

  walk.callbacks.insert_or_assign(openapi_location_uri(walk.base, base),
                                  std::move(entries));
}

inline auto openapi_check_callbacks_or_reference(const JSON &value,
                                                 const Pointer &base,
                                                 OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::Callbacks,
                             openapi_check_callbacks>(value, base, walk);
}

// OpenAPI Specification 3.1.1, Section 4.8.10: "Describes a single API
// operation on a path"
inline auto openapi_check_operation(const JSON &value, const Pointer &base,
                                    OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Operation);
  openapi_expect_object(value, base, "The Operation Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_OPERATION_FIELDS, base,
      "The Operation Object does not define this field");

  OpenAPIOperationRecord record;

  const auto *tags{value.try_at("tags", OPENAPI_HASH_TAGS)};
  if (tags != nullptr) {
    openapi_check_array_of_strings(
        *tags, openapi_child(base, "tags"sv),
        "The Operation Object tags must be an array",
        "The Operation Object tags must hold strings");
    record.tags.reserve(tags->size());
    for (const auto &tag : tags->as_array()) {
      record.tags.emplace_back(tag.to_string());
    }
  }

  openapi_check_optional_string(
      value, base, "summary"sv, OPENAPI_HASH_SUMMARY,
      "The Operation Object summary must be a string");

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Operation Object description must be a string");

  const auto *external_documentation{
      value.try_at("externalDocs", OPENAPI_HASH_EXTERNAL_DOCS)};
  if (external_documentation != nullptr) {
    openapi_check_external_documentation(
        *external_documentation, openapi_child(base, "externalDocs"sv), walk);
  }

  // OpenAPI Specification 3.1.1, Section 4.8.10: "operationId | string |
  // Unique string used to identify the operation. The id MUST be unique among
  // all operations described in the API"
  const auto *operation_id{
      value.try_at("operationId", OPENAPI_HASH_OPERATION_ID)};
  if (operation_id != nullptr) {
    const auto identifier{openapi_expect_string(
        *operation_id, base, "operationId"sv,
        "The Operation Object identifier must be a string")};
    const auto location{openapi_location_uri(walk.base, base)};
    const auto match{
        walk.operation_ids.try_emplace(JSON::String{identifier}, location)};
    if (!match.second && match.first->second != location) {
      throw OpenAPIError{openapi_child(base, "operationId"sv),
                         "The Operation Object identifiers must be unique"};
    }
  }

  const auto *parameters{value.try_at("parameters", OPENAPI_HASH_PARAMETERS)};
  if (parameters != nullptr) {
    record.parameters = openapi_check_parameters(
        *parameters, openapi_child(base, "parameters"sv),
        "The Operation Object parameters must be an array",
        "The Operation Object parameters must not repeat a name and location",
        walk);
  }

  const auto *request_body{
      value.try_at("requestBody", OPENAPI_HASH_REQUEST_BODY)};
  if (request_body != nullptr) {
    openapi_check_request_body_or_reference(
        *request_body, openapi_child(base, "requestBody"sv), walk);
  }

  const auto *responses{value.try_at("responses", OPENAPI_HASH_RESPONSES)};
  if (responses != nullptr) {
    openapi_check_responses(*responses, openapi_child(base, "responses"sv),
                            walk);
  }

  const auto *callbacks{value.try_at("callbacks", OPENAPI_HASH_CALLBACKS)};
  if (callbacks != nullptr) {
    const auto location{openapi_child(base, "callbacks"sv)};
    openapi_expect_object(*callbacks, location,
                          "The Operation Object callbacks must be an object");
    // Section 4.8.10: "callbacks | Map[string, Callback Object | Reference
    // Object] | A map of possible out-of band callbacks related to the parent
    // operation [...] The key is a unique identifier for the Callback Object".
    // Its keys are identifiers rather than field names, and like the webhooks
    // map and unlike the Paths Object this one carries no extension carve-out,
    // so a member named `x-` is a callback
    for (const auto &entry : callbacks->as_object()) {
      const auto callback{openapi_child(location, entry.first)};
      openapi_check_callbacks_or_reference(entry.second, callback, walk);
      record.callbacks.push_back(openapi_location_uri(walk.base, callback));
    }
  }

  const auto *deprecated{value.try_at("deprecated", OPENAPI_HASH_DEPRECATED)};
  if (deprecated != nullptr) {
    openapi_expect_boolean(*deprecated, base, "deprecated"sv,
                           "The Operation Object deprecated must be a boolean");
  }

  const auto *security{value.try_at("security", OPENAPI_HASH_SECURITY)};
  if (security != nullptr) {
    record.security = openapi_check_security(
        *security, openapi_child(base, "security"sv),
        "The Operation Object security must be an array", walk);
  }

  const auto *servers{value.try_at("servers", OPENAPI_HASH_SERVERS)};
  if (servers != nullptr) {
    record.servers = openapi_check_server_array(
        *servers, openapi_child(base, "servers"sv),
        "The Operation Object servers must be an array", walk);
  }

  walk.operation_records.insert_or_assign(openapi_location_uri(walk.base, base),
                                          std::move(record));
}

// OpenAPI Specification 3.1.1, Section 4.8.9: "Describes the operations
// available on a single path"
inline auto openapi_check_path_item(const JSON &value, const Pointer &base,
                                    OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::PathItem);
  openapi_expect_object(value, base, "The Path Item Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_PATH_ITEM_FIELDS_3_1, OPENAPI_PATH_ITEM_FIELDS_3_2, base,
      "The Path Item Object does not define this field", walk);

  // OpenAPI Specification 3.1.1, Section 4.8.9: "$ref | string | Allows for a
  // referenced definition of this path item. The value MUST be in the form of
  // a URI". Unlike a Reference Object this is a plain field, and the
  // specification leaves the behavior of its siblings undefined rather than
  // forbidding them
  const auto *target{value.try_at("$ref", OPENAPI_HASH_REF)};
  if (target != nullptr) {
    const auto reference{openapi_expect_uri_reference(
        *target, base, "$ref"sv,
        "The Path Item Object reference must be a string",
        "The Path Item Object reference must be a URI reference")};
    openapi_follow_reference(reference, openapi_child(base, "$ref"sv),
                             OpenAPIObjectKind::PathItem, walk);
  }

  openapi_check_optional_string(
      value, base, "summary"sv, OPENAPI_HASH_SUMMARY,
      "The Path Item Object summary must be a string");

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Path Item Object description must be a string");

  OpenAPIPathItemRecord record;

  const auto *servers{value.try_at("servers", OPENAPI_HASH_SERVERS)};
  if (servers != nullptr) {
    record.servers = openapi_check_server_array(
        *servers, openapi_child(base, "servers"sv),
        "The Path Item Object servers must be an array", walk);
  }

  const auto *parameters{value.try_at("parameters", OPENAPI_HASH_PARAMETERS)};
  if (parameters != nullptr) {
    record.parameters = openapi_check_parameters(
        *parameters, openapi_child(base, "parameters"sv),
        "The Path Item Object parameters must be an array",
        "The Path Item Object parameters must not repeat a name and location",
        walk);
  }

  for (const auto &method : OPENAPI_PATH_ITEM_METHODS) {
    const auto *operation{value.try_at(method.name, method.hash)};
    if (operation != nullptr) {
      const auto location{openapi_child(base, method.name)};
      openapi_check_operation(*operation, location, walk);
      record.operations.emplace_back(JSON::String{method.name},
                                     openapi_location_uri(walk.base, location));
    }
  }

  // OpenAPI Specification 3.2.1, Section 4.9: "The map key is the HTTP
  // method with the same capitalization that is to be sent in the request.
  // This map MUST NOT contain any entry for the methods that can be defined by
  // other fixed fields with Operation Object values (e.g. no `POST` entry, as
  // the `post` field is used for this method)". Which methods those are the
  // same section names one field at a time, `get` being "a definition of a GET
  // operation" and so on, and RFC 9110 Section 9.1: "The method token is
  // case-sensitive". So a key matches one of those methods or names another
  // method entirely, and comparing without case would turn down a key this
  // specification admits
  const auto *additional{
      value.try_at("additionalOperations", OPENAPI_HASH_ADDITIONAL_OPERATIONS)};
  if (additional != nullptr) {
    const auto location{openapi_child(base, "additionalOperations"sv)};
    openapi_expect_object(
        *additional, location,
        "The Path Item Object additional operations must be an object");
    for (const auto &entry : additional->as_object()) {
      // A key is "the HTTP method [...] that is to be sent in the request",
      // and RFC 9110 Section 9.1 has a method be a token, so a key that is no
      // token names no method that could be sent at all
      if (!http_is_token(entry.first)) {
        throw OpenAPIError{
            openapi_child(location, entry.first),
            "The Path Item Object additional operations must name an HTTP "
            "method"};
      }

      if (std::ranges::find(OPENAPI_PATH_ITEM_METHOD_NAMES, entry.first) !=
          OPENAPI_PATH_ITEM_METHOD_NAMES.cend()) {
        throw OpenAPIError{
            openapi_child(location, entry.first),
            "The Path Item Object additional operations must not name a "
            "method that a field of its own defines"};
      }

      const auto operation{openapi_child(location, entry.first)};
      openapi_check_operation(entry.second, operation, walk);
      record.operations.emplace_back(
          entry.first, openapi_location_uri(walk.base, operation));
    }
  }

  walk.path_items.insert_or_assign(openapi_location_uri(walk.base, base),
                                   std::move(record));
}

} // namespace sourcemeta::core

#endif
