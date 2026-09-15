#ifndef SOURCEMETA_CORE_OPENAPI_REQUEST_BODY_H_
#define SOURCEMETA_CORE_OPENAPI_REQUEST_BODY_H_

#include <sourcemeta/core/openapi.h>

#include "content.h"
#include "helpers.h"
#include "reference.h"

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr std::array<JSON::StringView, 3> OPENAPI_REQUEST_BODY_FIELDS{
    {"description"sv, "content"sv, "required"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.13: "Describes a single request
// body"
inline auto openapi_check_request_body(const JSON &value, const Pointer &base,
                                       OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::RequestBody);
  openapi_expect_object(value, base,
                        "The Request Body Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_REQUEST_BODY_FIELDS, base,
      "The Request Body Object does not define this field");

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Request Body Object description must be a string");

  const auto *required{value.try_at("required", OPENAPI_HASH_REQUIRED)};
  if (required != nullptr) {
    openapi_expect_boolean(
        *required, base, "required"sv,
        "The Request Body Object required must be a boolean");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.13: "content | Map[string, Media
  // Type Object] | REQUIRED. The content of the request body"
  const auto &content{
      openapi_require(value, "content"sv, OPENAPI_HASH_CONTENT, base,
                      "The Request Body Object must declare a content")};

  openapi_check_content(content, openapi_child(base, "content"sv),
                        "The Request Body Object content must be an object",
                        walk);
}

inline auto openapi_check_request_body_or_reference(const JSON &value,
                                                    const Pointer &base,
                                                    OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::RequestBody,
                             openapi_check_request_body>(value, base, walk);
}

} // namespace sourcemeta::core

#endif
