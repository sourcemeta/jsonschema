#ifndef SOURCEMETA_CORE_OPENAPI_EXAMPLE_H_
#define SOURCEMETA_CORE_OPENAPI_EXAMPLE_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"
#include "reference.h"

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_VALUE{JSON::Object::hash("value"sv)};
constexpr auto OPENAPI_HASH_EXTERNAL_VALUE{
    JSON::Object::hash("externalValue"sv)};
constexpr auto OPENAPI_HASH_EXAMPLE{JSON::Object::hash("example"sv)};
constexpr auto OPENAPI_HASH_EXAMPLES{JSON::Object::hash("examples"sv)};

constexpr auto OPENAPI_HASH_DATA_VALUE{JSON::Object::hash("dataValue"sv)};
constexpr auto OPENAPI_HASH_SERIALIZED_VALUE{
    JSON::Object::hash("serializedValue"sv)};

constexpr std::array<JSON::StringView, 4> OPENAPI_EXAMPLE_FIELDS_3_1{
    {"summary"sv, "description"sv, "value"sv, "externalValue"sv}};

// OpenAPI Specification 3.2.1, Section 4.19 adds `dataValue` and
// `serializedValue`
constexpr std::array<JSON::StringView, 6> OPENAPI_EXAMPLE_FIELDS_3_2{
    {"summary"sv, "description"sv, "value"sv, "externalValue"sv, "dataValue"sv,
     "serializedValue"sv}};

// OpenAPI Specification 3.1.1, Section 4.8.19: "An object grouping an internal
// or external example value with basic `summary` and `description` metadata"
inline auto openapi_check_example(const JSON &value, const Pointer &base,
                                  OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Example);
  openapi_expect_object(value, base, "The Example Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_EXAMPLE_FIELDS_3_1, OPENAPI_EXAMPLE_FIELDS_3_2, base,
      "The Example Object does not define this field", walk);

  openapi_check_optional_string(value, base, "summary"sv, OPENAPI_HASH_SUMMARY,
                                "The Example Object summary must be a string");

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Example Object description must be a string");

  // OpenAPI Specification 3.2.1, Section 4.19: "dataValue | Any | An example
  // of the data structure [...] If this field is present, `value` MUST be
  // absent"
  const auto *data{value.try_at("dataValue", OPENAPI_HASH_DATA_VALUE)};
  if (data != nullptr && value.try_at("value", OPENAPI_HASH_VALUE) != nullptr) {
    throw OpenAPIError{
        base, "The Example Object data value and value are mutually exclusive"};
  }

  // Section 4.19: "serializedValue | string | An example of the serialized
  // form of the value [...] If this field is present, `value`, and
  // `externalValue` MUST be absent". The `externalValue` field states the
  // other half of that pair the same way
  const auto *serialized{
      value.try_at("serializedValue", OPENAPI_HASH_SERIALIZED_VALUE)};
  if (serialized != nullptr) {
    openapi_expect_string(
        *serialized, base, "serializedValue"sv,
        "The Example Object serialized value must be a string");

    if (value.try_at("value", OPENAPI_HASH_VALUE) != nullptr) {
      throw OpenAPIError{base,
                         "The Example Object serialized value and value are "
                         "mutually exclusive"};
    }

    if (value.try_at("externalValue", OPENAPI_HASH_EXTERNAL_VALUE) != nullptr) {
      throw OpenAPIError{base,
                         "The Example Object serialized value and external "
                         "value are mutually exclusive"};
    }
  }

  // OpenAPI Specification 3.1.1, Section 4.8.19: "externalValue | string | A
  // URI that identifies the literal example"
  const auto *external{
      value.try_at("externalValue", OPENAPI_HASH_EXTERNAL_VALUE)};
  if (external != nullptr) {
    openapi_expect_uri_reference(
        *external, base, "externalValue"sv,
        "The Example Object external value must be a string",
        "The Example Object external value must be a URI reference");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.19: "The `value` field and
  // `externalValue` field are mutually exclusive". Any JSON value is a legal
  // `value`, so there is nothing else to check about it
  if (external != nullptr &&
      value.try_at("value", OPENAPI_HASH_VALUE) != nullptr) {
    throw OpenAPIError{
        base, "The Example Object value and external value are mutually "
              "exclusive"};
  }
}

inline auto openapi_check_example_or_reference(const JSON &value,
                                               const Pointer &base,
                                               OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::Example, openapi_check_example>(
      value, base, walk);
}

// The Parameter, Media Type and Header Objects all carry this pair, and all
// three state that "The `example` field is mutually exclusive of the
// `examples` field"
inline auto openapi_check_examples(const JSON &value, const Pointer &base,
                                   const char *exclusive_message,
                                   const char *type_message, OpenAPIWalk &walk)
    -> void {
  const auto *single{value.try_at("example", OPENAPI_HASH_EXAMPLE)};
  const auto *multiple{value.try_at("examples", OPENAPI_HASH_EXAMPLES)};
  if (single != nullptr && multiple != nullptr) {
    throw OpenAPIError{base, exclusive_message};
  }

  if (multiple == nullptr) {
    return;
  }

  const auto location{openapi_child(base, "examples"sv)};
  openapi_expect_object(*multiple, location, type_message);
  for (const auto &entry : multiple->as_object()) {
    openapi_check_example_or_reference(
        entry.second, openapi_child(location, entry.first), walk);
  }
}

} // namespace sourcemeta::core

#endif
