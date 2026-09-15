#ifndef SOURCEMETA_CORE_OPENAPI_RESPONSE_H_
#define SOURCEMETA_CORE_OPENAPI_RESPONSE_H_

#include <sourcemeta/core/openapi.h>

#include "content.h"
#include "helpers.h"
#include "link.h"
#include "reference.h"

#include <sourcemeta/core/http.h>

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_LINKS{JSON::Object::hash("links"sv)};
constexpr auto OPENAPI_HASH_DEFAULT{JSON::Object::hash("default"sv)};

constexpr std::array<JSON::StringView, 4> OPENAPI_RESPONSE_FIELDS_3_1{
    {"description"sv, "headers"sv, "content"sv, "links"sv}};

// OpenAPI Specification 3.2.1, Section 4.17 adds `summary`
constexpr std::array<JSON::StringView, 5> OPENAPI_RESPONSE_FIELDS_3_2{
    {"description"sv, "headers"sv, "content"sv, "links"sv, "summary"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.17: "Describes a single response
// from an API operation"
inline auto openapi_check_response(const JSON &value, const Pointer &base,
                                   OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Response);
  openapi_expect_object(value, base, "The Response Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_RESPONSE_FIELDS_3_1, OPENAPI_RESPONSE_FIELDS_3_2, base,
      "The Response Object does not define this field", walk);

  // OpenAPI Specification 3.1.1, Section 4.8.17: "description | string |
  // REQUIRED. A description of the response". OpenAPI Specification 3.2.1,
  // Section 4.17 drops that word from the same row, and the published
  // meta-schemas draw the same line, the 3.1 one requiring the field and the
  // 3.2 one requiring nothing of a Response Object. This is the one field 3.2
  // stops requiring
  const auto *description{
      value.try_at("description", OPENAPI_HASH_DESCRIPTION)};
  if (description == nullptr && walk.version != OpenAPIVersion::OPENAPI_3_2) {
    throw OpenAPIError{base, "The Response Object must declare a description"};
  }

  if (description != nullptr) {
    openapi_expect_string(*description, base, "description"sv,
                          "The Response Object description must be a string");
  }

  // OpenAPI Specification 3.2.1, Section 4.17: "summary | string"
  openapi_check_optional_string(value, base, "summary"sv, OPENAPI_HASH_SUMMARY,
                                "The Response Object summary must be a string");

  openapi_check_headers(value, base,
                        "The Response Object headers must be an object", walk);

  const auto *content{value.try_at("content", OPENAPI_HASH_CONTENT)};
  if (content != nullptr) {
    openapi_check_content(*content, openapi_child(base, "content"sv),
                          "The Response Object content must be an object",
                          walk);
  }

  const auto *links{value.try_at("links", OPENAPI_HASH_LINKS)};
  if (links != nullptr) {
    const auto location{openapi_child(base, "links"sv)};
    openapi_expect_object(*links, location,
                          "The Response Object links must be an object");
    for (const auto &entry : links->as_object()) {
      openapi_check_link_or_reference(
          entry.second, openapi_child(location, entry.first), walk);
    }
  }
}

inline auto openapi_check_response_or_reference(const JSON &value,
                                                const Pointer &base,
                                                OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::Response,
                             openapi_check_response>(value, base, walk);
}

// OpenAPI Specification 3.1.1, Section 4.8.16 keys this map by HTTP status
// code, and the meta-schema reads the allowed shapes as three digits from
// `100` through `599`, or a leading digit followed by `XX` for a whole range
inline auto openapi_is_status_code(const JSON::StringView code) noexcept
    -> bool {
  // The range form is this specification's own, HTTP knowing nothing of it
  if (code.size() == 3 && code.front() >= '1' && code.front() <= '5' &&
      code[1] == 'X' && code[2] == 'X') {
    return true;
  }

  return http_is_status_code(code);
}

// OpenAPI Specification 3.1.1, Section 4.8.16: "A container for the expected
// responses of an operation"
inline auto openapi_check_responses(const JSON &value, const Pointer &base,
                                    OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Responses);
  openapi_expect_object(value, base, "The Responses Object must be an object");

  // The meta-schema bounds this map from below, and requires a `default` when
  // no status code is named
  if (value.empty()) {
    throw OpenAPIError{base, "The Responses Object must not be empty"};
  }

  bool names_a_status_code{false};
  for (const auto &entry : value.as_object()) {
    if (entry.first.starts_with(OPENAPI_EXTENSION_PREFIX)) {
      continue;
    }

    const auto location{openapi_child(base, entry.first)};
    if (entry.first == "default"sv) {
      openapi_check_response_or_reference(entry.second, location, walk);
      continue;
    }

    if (!openapi_is_status_code(entry.first)) {
      throw OpenAPIError{location,
                         "The Responses Object keys must be an HTTP status "
                         "code or a status code range"};
    }

    names_a_status_code = true;
    openapi_check_response_or_reference(entry.second, location, walk);
  }

  if (!names_a_status_code &&
      value.try_at("default", OPENAPI_HASH_DEFAULT) == nullptr) {
    throw OpenAPIError{
        base, "The Responses Object must declare a default or a status code"};
  }
}

} // namespace sourcemeta::core

#endif
