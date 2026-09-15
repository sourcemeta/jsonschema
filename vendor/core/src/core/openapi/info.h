#ifndef SOURCEMETA_CORE_OPENAPI_INFO_H_
#define SOURCEMETA_CORE_OPENAPI_INFO_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"

#include <sourcemeta/core/email.h>

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_INFO{JSON::Object::hash("info"sv)};
constexpr auto OPENAPI_HASH_TITLE{JSON::Object::hash("title"sv)};
constexpr auto OPENAPI_HASH_TERMS_OF_SERVICE{
    JSON::Object::hash("termsOfService"sv)};
constexpr auto OPENAPI_HASH_CONTACT{JSON::Object::hash("contact"sv)};
constexpr auto OPENAPI_HASH_LICENSE{JSON::Object::hash("license"sv)};
constexpr auto OPENAPI_HASH_VERSION{JSON::Object::hash("version"sv)};
constexpr auto OPENAPI_HASH_EMAIL{JSON::Object::hash("email"sv)};
constexpr auto OPENAPI_HASH_IDENTIFIER{JSON::Object::hash("identifier"sv)};

constexpr std::array<JSON::StringView, 7> OPENAPI_INFO_FIELDS{
    {"title"sv, "summary"sv, "description"sv, "termsOfService"sv, "contact"sv,
     "license"sv, "version"sv}};

constexpr std::array<JSON::StringView, 3> OPENAPI_CONTACT_FIELDS{
    {"name"sv, "url"sv, "email"sv}};

constexpr std::array<JSON::StringView, 3> OPENAPI_LICENSE_FIELDS{
    {"name"sv, "identifier"sv, "url"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.3: "Contact information for the
// exposed API"
inline auto openapi_parse_contact(const JSON &value, const Pointer &base,
                                  OpenAPIWalk &walk) -> OpenAPIContact {
  openapi_record(walk, base, OpenAPIObjectKind::Contact);
  if (!value.is_object()) {
    throw OpenAPIError{base, "The Contact Object must be an object"};
  }

  openapi_reject_unknown_fields(
      value, OPENAPI_CONTACT_FIELDS, base,
      "The Contact Object does not define this field");

  OpenAPIContact result;

  const auto *name{value.try_at("name", OPENAPI_HASH_NAME)};
  if (name != nullptr) {
    result.name = openapi_expect_string(
        *name, base, "name"sv, "The Contact Object name must be a string");
  }

  const auto *url{value.try_at("url", OPENAPI_HASH_URL)};
  if (url != nullptr) {
    result.url = openapi_expect_uri_reference(
        *url, base, "url"sv, "The Contact Object URI must be a string",
        "The Contact Object URI must be a URI reference");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.3: "The email address of the
  // contact person/organization. This MUST be in the form of an email address"
  const auto *email{value.try_at("email", OPENAPI_HASH_EMAIL)};
  if (email != nullptr) {
    const auto address{openapi_expect_string(
        *email, base, "email"sv, "The Contact Object email must be a string")};
    if (!is_email(address)) {
      throw OpenAPIError{openapi_child(base, "email"sv),
                         "The Contact Object email must be an email address"};
    }

    result.email = address;
  }

  return result;
}

// OpenAPI Specification 3.1.1, Section 4.8.4: "License information for the
// exposed API"
inline auto openapi_parse_license(const JSON &value, const Pointer &base,
                                  OpenAPIWalk &walk) -> OpenAPILicense {
  openapi_record(walk, base, OpenAPIObjectKind::License);
  if (!value.is_object()) {
    throw OpenAPIError{base, "The License Object must be an object"};
  }

  openapi_reject_unknown_fields(
      value, OPENAPI_LICENSE_FIELDS, base,
      "The License Object does not define this field");

  OpenAPILicense result;

  // OpenAPI Specification 3.1.1, Section 4.8.4: "name | string | REQUIRED. The
  // license name used for the API"
  const auto &name{openapi_require(value, "name"sv, OPENAPI_HASH_NAME, base,
                                   "The License Object must declare a name")};

  result.name = openapi_expect_string(
      name, base, "name"sv, "The License Object name must be a string");

  // The specification states no requirement on the syntax of this field, only
  // that it is "An SPDX license expression for the API", so it is recorded as
  // it was written
  const auto *identifier{value.try_at("identifier", OPENAPI_HASH_IDENTIFIER)};
  if (identifier != nullptr) {
    result.identifier =
        openapi_expect_string(*identifier, base, "identifier"sv,
                              "The License Object identifier must be a string");
  }

  const auto *url{value.try_at("url", OPENAPI_HASH_URL)};
  if (url != nullptr) {
    result.url = openapi_expect_uri_reference(
        *url, base, "url"sv, "The License Object URI must be a string",
        "The License Object URI must be a URI reference");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.4: "The `identifier` field is
  // mutually exclusive of the `url` field"
  if (result.identifier.has_value() && result.url.has_value()) {
    throw OpenAPIError{
        base, "The License Object identifier and URI are mutually exclusive"};
  }

  return result;
}

// Read the Info Object of an OpenAPI Description, turning down anything the
// specification does not permit. The result borrows from the given document
inline auto openapi_parse_info(const JSON &document, OpenAPIWalk &walk)
    -> OpenAPIInfo {
  // OpenAPI Specification 3.1.1, Section 4.8.1: "info | Info Object |
  // REQUIRED. Provides metadata about the API"
  const auto &value{
      openapi_require(document, "info"sv, OPENAPI_HASH_INFO, EMPTY_POINTER,
                      "The OpenAPI Description must provide an Info Object")};

  const Pointer base{"info"};
  openapi_record(walk, base, OpenAPIObjectKind::Info);
  if (!value.is_object()) {
    throw OpenAPIError{base, "The Info Object must be an object"};
  }

  openapi_reject_unknown_fields(value, OPENAPI_INFO_FIELDS, base,
                                "The Info Object does not define this field");

  OpenAPIInfo result;

  // OpenAPI Specification 3.1.1, Section 4.8.2: "title | string | REQUIRED.
  // The title of the API"
  const auto &title{openapi_require(value, "title"sv, OPENAPI_HASH_TITLE, base,
                                    "The Info Object must declare a title")};

  result.title = openapi_expect_string(
      title, base, "title"sv, "The Info Object title must be a string");

  // OpenAPI Specification 3.1.1, Section 4.8.2: "version | string | REQUIRED.
  // The version of the OpenAPI Document"
  const auto &version{
      openapi_require(value, "version"sv, OPENAPI_HASH_VERSION, base,
                      "The Info Object must declare a version")};

  result.version = openapi_expect_string(
      version, base, "version"sv, "The Info Object version must be a string");

  const auto *summary{value.try_at("summary", OPENAPI_HASH_SUMMARY)};
  if (summary != nullptr) {
    result.summary =
        openapi_expect_string(*summary, base, "summary"sv,
                              "The Info Object summary must be a string");
  }

  // The specification permits CommonMark here but does not require it, so
  // there is nothing to check beyond the type
  const auto *description{
      value.try_at("description", OPENAPI_HASH_DESCRIPTION)};
  if (description != nullptr) {
    result.description =
        openapi_expect_string(*description, base, "description"sv,
                              "The Info Object description must be a string");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.2: "A URI for the Terms of
  // Service for the API. This MUST be in the form of a URI"
  const auto *terms{
      value.try_at("termsOfService", OPENAPI_HASH_TERMS_OF_SERVICE)};
  if (terms != nullptr) {
    result.terms_of_service = openapi_expect_uri_reference(
        *terms, base, "termsOfService"sv,
        "The Info Object terms of service must be a string",
        "The Info Object terms of service must be a URI reference");
  }

  const auto *contact{value.try_at("contact", OPENAPI_HASH_CONTACT)};
  if (contact != nullptr) {
    result.contact =
        openapi_parse_contact(*contact, openapi_child(base, "contact"sv), walk);
  }

  const auto *license{value.try_at("license", OPENAPI_HASH_LICENSE)};
  if (license != nullptr) {
    result.license =
        openapi_parse_license(*license, openapi_child(base, "license"sv), walk);
  }

  return result;
}

} // namespace sourcemeta::core

#endif
