#ifndef SOURCEMETA_CORE_OPENAPI_EXTERNAL_DOCUMENTATION_H_
#define SOURCEMETA_CORE_OPENAPI_EXTERNAL_DOCUMENTATION_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_EXTERNAL_DOCS{JSON::Object::hash("externalDocs"sv)};

constexpr std::array<JSON::StringView, 2> OPENAPI_EXTERNAL_DOCS_FIELDS{
    {"description"sv, "url"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.11: "Allows referencing an external
// resource for extended documentation"
inline auto openapi_check_external_documentation(const JSON &value,
                                                 const Pointer &base,
                                                 OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::ExternalDocumentation);
  if (!value.is_object()) {
    throw OpenAPIError{base,
                       "The External Documentation Object must be an object"};
  }

  openapi_reject_unknown_fields(
      value, OPENAPI_EXTERNAL_DOCS_FIELDS, base,
      "The External Documentation Object does not define this field");

  // OpenAPI Specification 3.1.1, Section 4.8.11: "url | string | REQUIRED. The
  // URI for the target documentation. This MUST be in the form of a URI".
  // Unlike the Server Object, this one does state a requirement on its form,
  // and it names no template variables, so it is checked
  const auto &url{
      openapi_require(value, "url"sv, OPENAPI_HASH_URL, base,
                      "The External Documentation Object must declare a URI")};

  openapi_expect_uri_reference(
      url, base, "url"sv,
      "The External Documentation Object URI must be a string",
      "The External Documentation Object URI must be a URI reference");

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The External Documentation Object description must be a string");
}

// OpenAPI Specification 3.1.1, Section 4.8.1: "externalDocs | External
// Documentation Object | Additional external documentation"
inline auto openapi_check_root_external_documentation(const JSON &document,
                                                      OpenAPIWalk &walk)
    -> void {
  const auto *value{
      document.try_at("externalDocs", OPENAPI_HASH_EXTERNAL_DOCS)};
  if (value == nullptr) {
    return;
  }

  openapi_check_external_documentation(*value, Pointer{"externalDocs"}, walk);
}

} // namespace sourcemeta::core

#endif
