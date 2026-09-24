#ifndef SOURCEMETA_CORE_OPENAPI_PARAMETER_H_
#define SOURCEMETA_CORE_OPENAPI_PARAMETER_H_

#include <sourcemeta/core/openapi.h>

#include "content.h"
#include "example.h"
#include "helpers.h"
#include "reference.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <set>         // std::set
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <vector>      // std::vector

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_ALLOW_EMPTY_VALUE{
    JSON::Object::hash("allowEmptyValue"sv)};

// The fields a Parameter Object admits depend on how it serialises and on
// where it sits, which is how the meta-schema reads it, holding the
// serialisation fields behind a dependent schema on `schema` and
// `allowEmptyValue` and `allowReserved` behind the location
constexpr std::array<JSON::StringView, 6> OPENAPI_PARAMETER_CONTENT_FIELDS_3_1{
    {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
     "content"sv}};

// OpenAPI Specification 3.2.1, Section 4.12.2.1 moves `example` and
// `examples` into the Parameter Object's common fields, whose table says of
// them and of the rest: "These fields MAY be used with either `content` or
// `schema`".
// In 3.1 both sat in the table for use with `schema` alone, which is where the
// published meta-schemas draw the same line, the 3.1 one holding them behind
// `schema` and the 3.2 one alongside it
constexpr std::array<JSON::StringView, 8> OPENAPI_PARAMETER_CONTENT_FIELDS_3_2{
    {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
     "content"sv, "example"sv, "examples"sv}};

constexpr std::array<JSON::StringView, 10> OPENAPI_PARAMETER_SCHEMA_FIELDS{
    {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
     "schema"sv, "style"sv, "explode"sv, "example"sv, "examples"sv}};

// 3.2.1 Section 4.12, of `allowReserved`: "This field only applies to `in` and
// `style` values that automatically percent-encode (that is: `in: path`,
// `in: query`, and `in: cookie` with `style: form`)". 3.1 held the same field
// to `query` alone
constexpr std::array<JSON::StringView, 11>
    OPENAPI_PARAMETER_RESERVED_SCHEMA_FIELDS_3_2{
        {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
         "schema"sv, "style"sv, "explode"sv, "example"sv, "examples"sv,
         "allowReserved"sv}};

constexpr std::array<JSON::StringView, 12>
    OPENAPI_PARAMETER_QUERY_SCHEMA_FIELDS{
        {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
         "schema"sv, "style"sv, "explode"sv, "example"sv, "examples"sv,
         "allowEmptyValue"sv, "allowReserved"sv}};

constexpr std::array<JSON::StringView, 7>
    OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_1{
        {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
         "content"sv, "allowEmptyValue"sv}};

constexpr std::array<JSON::StringView, 9>
    OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_2{
        {"name"sv, "in"sv, "description"sv, "required"sv, "deprecated"sv,
         "content"sv, "allowEmptyValue"sv, "example"sv, "examples"sv}};

// Section 4.8.12.2.1 holds `allowEmptyValue` among the fields that "MAY be used
// with either `content` or `schema`" and restricts it in its own Description
// cell alone: "This field is valid only for `query` parameters". So a
// parameter elsewhere writing it writes a field this Object defines rather
// than one it does not, which is a different thing to be turned down for
constexpr std::array<JSON::StringView, 1>
    OPENAPI_PARAMETER_INAPPLICABLE_COMMON_FIELDS{{"allowEmptyValue"sv}};

// 3.2.1 Section 4.12.2.2 holds `allowReserved` among the fields for use with
// `schema` and restricts it the same way: "This field only applies to `in`
// and `style` values that automatically percent-encode". 3.1 restricts the
// same field to `query` alone. A `content` form reaches neither field table,
// so this stands for the `schema` form where both fields are written down
constexpr std::array<JSON::StringView, 2>
    OPENAPI_PARAMETER_INAPPLICABLE_SCHEMA_FIELDS{
        {"allowEmptyValue"sv, "allowReserved"sv}};

constexpr auto OPENAPI_PARAMETER_INAPPLICABLE_MESSAGE{
    "The Parameter Object does not admit this field as declared"};

// 3.2.1 Section 4.12 scopes `allowReserved` to the locations that
// percent-encode of their own accord. A query parameter has a field table of
// its own, so what is asked here is whether one of the other two locations is
// such a place, and a cookie parameter takes the `form` style when it declares
// none
inline auto openapi_parameter_admits_reserved(const JSON::StringView location,
                                              const JSON &value) -> bool {
  if (location == "path"sv) {
    return true;
  }

  if (location != "cookie"sv) {
    return false;
  }

  const auto *style{value.try_at("style", OPENAPI_HASH_STYLE)};
  return style == nullptr ||
         (style->is_string() && style->to_string() == "form"sv);
}

// OpenAPI Specification 3.1.1, Section 4.8.12: "Describes a single operation
// parameter"
inline auto openapi_check_parameter(const JSON &value, const Pointer &base,
                                    OpenAPIWalk &walk)
    -> std::pair<JSON::StringView, JSON::StringView> {
  openapi_record(walk, base, OpenAPIObjectKind::Parameter);
  openapi_expect_object(value, base, "The Parameter Object must be an object");
  const auto location{openapi_location_uri(walk.base, base)};

  // OpenAPI Specification 3.1.1, Section 4.8.12: "name | string | REQUIRED.
  // The name of the parameter"
  const auto &name{openapi_require(value, "name"sv, OPENAPI_HASH_NAME, base,
                                   "The Parameter Object must declare a name")};

  const auto parameter_name{openapi_expect_string(
      name, base, "name"sv, "The Parameter Object name must be a string")};

  // OpenAPI Specification 3.1.1, Section 4.8.12: "in | string | REQUIRED. The
  // location of the parameter. Possible values are `"query"`, `"header"`,
  // `"path"` or `"cookie"`"
  const auto &location_field{
      openapi_require(value, "in"sv, OPENAPI_HASH_IN, base,
                      "The Parameter Object must declare a location")};

  // OpenAPI Specification 3.2.1, Section 4.12 adds a fifth location,
  // `querystring`, "a parameter that treats the entire URL query string as a
  // value". An enumeration is as much a part of a revision's field table as
  // the field names are
  const auto parameter_location{
      walk.version == OpenAPIVersion::OPENAPI_3_2
          ? openapi_expect_enumeration(
                location_field, base, "in"sv,
                {"query"sv, "header"sv, "path"sv, "cookie"sv, "querystring"sv},
                "The Parameter Object location must be a string",
                "The Parameter Object location is not one this specification "
                "defines")
          : openapi_expect_enumeration(
                location_field, base, "in"sv,
                {"query"sv, "header"sv, "path"sv, "cookie"sv},
                "The Parameter Object location must be a string",
                "The Parameter Object location is not one this specification "
                "defines")};

  const auto *schema{value.try_at("schema", OPENAPI_HASH_SCHEMA)};
  const auto *content{value.try_at("content", OPENAPI_HASH_CONTENT)};

  if (schema != nullptr && content != nullptr) {
    throw OpenAPIError{
        base, "The Parameter Object schema and content are mutually exclusive"};
  }

  if (schema == nullptr && content == nullptr) {
    throw OpenAPIError{
        base, "The Parameter Object must declare a schema or a content"};
  }

  // 3.2.1 Section 4.12, of `querystring`: its value "MUST be specified using
  // the `content` field"
  if (parameter_location == "querystring"sv && content == nullptr) {
    throw OpenAPIError{base,
                       "A querystring Parameter Object must declare a content"};
  }

  const auto in_query{parameter_location == "query"sv};
  if (schema == nullptr) {
    if (in_query) {
      openapi_reject_unknown_fields(
          value, OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_1,
          OPENAPI_PARAMETER_QUERY_CONTENT_FIELDS_3_2, base,
          "The Parameter Object does not define this field", walk);
    } else {
      openapi_reject_unknown_fields(
          value, OPENAPI_PARAMETER_CONTENT_FIELDS_3_1,
          OPENAPI_PARAMETER_CONTENT_FIELDS_3_2, base,
          "The Parameter Object does not define this field", walk,
          OPENAPI_PARAMETER_INAPPLICABLE_COMMON_FIELDS,
          OPENAPI_PARAMETER_INAPPLICABLE_MESSAGE);
    }
  } else if (in_query) {
    openapi_reject_unknown_fields(
        value, OPENAPI_PARAMETER_QUERY_SCHEMA_FIELDS, base,
        "The Parameter Object does not define this field");
  } else if (walk.version == OpenAPIVersion::OPENAPI_3_2 &&
             openapi_parameter_admits_reserved(parameter_location, value)) {
    openapi_reject_unknown_fields(
        value, OPENAPI_PARAMETER_RESERVED_SCHEMA_FIELDS_3_2, base,
        "The Parameter Object does not define this field",
        OPENAPI_PARAMETER_INAPPLICABLE_COMMON_FIELDS,
        OPENAPI_PARAMETER_INAPPLICABLE_MESSAGE);
  } else {
    openapi_reject_unknown_fields(
        value, OPENAPI_PARAMETER_SCHEMA_FIELDS, base,
        "The Parameter Object does not define this field",
        OPENAPI_PARAMETER_INAPPLICABLE_SCHEMA_FIELDS,
        OPENAPI_PARAMETER_INAPPLICABLE_MESSAGE);
  }

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Parameter Object description must be a string");

  const auto *deprecated{value.try_at("deprecated", OPENAPI_HASH_DEPRECATED)};
  if (deprecated != nullptr) {
    openapi_expect_boolean(*deprecated, base, "deprecated"sv,
                           "The Parameter Object deprecated must be a boolean");
  }

  const auto *required{value.try_at("required", OPENAPI_HASH_REQUIRED)};
  if (required != nullptr) {
    openapi_expect_boolean(*required, base, "required"sv,
                           "The Parameter Object required must be a boolean");
  }

  // OpenAPI Specification 3.1.1, Section 4.8.12: "If the parameter location is
  // `"path"`, this field is REQUIRED and its value MUST be `true`"
  if (parameter_location == "path"sv) {
    if (required == nullptr) {
      throw OpenAPIError{
          base, "A path Parameter Object must declare that it is required"};
    }

    if (!required->to_boolean()) {
      throw OpenAPIError{openapi_child(base, "required"sv),
                         "A path Parameter Object must be required"};
    }

    // OpenAPI Specification 3.2.1, Section 4.12.2.1 has such a name "correspond
    // to a single template expression occurring within the path field in the
    // Paths Object", and 3.2.1 Section 4.8.2 writes out what one may hold:
    // "template-expression-param-name = 1*( %x00-7A / %x7C / %x7E-10FFFF ) ;
    // every Unicode character except { and }". No revision of 3.1 carries that
    // grammar, and all 3.1.1 Section 3.5 says of the shape is "template
    // expressions, delimited by curly braces", which leaves a brace within a
    // name to whatever reads the path. Holding a 3.1 document to this would
    // also leave it nothing to declare, as the expression of `/a/{x{y}` reads
    // as `x{y` there and the correspondence below would then ask for the very
    // name this turns down
    if (walk.version == OpenAPIVersion::OPENAPI_3_2 &&
        (parameter_name.find('{') != JSON::StringView::npos ||
         parameter_name.find('}') != JSON::StringView::npos)) {
      throw OpenAPIError{openapi_child(base, "name"sv),
                         "A path Parameter Object name must not hold braces"};
    }
  }

  const auto *allow_empty{
      value.try_at("allowEmptyValue", OPENAPI_HASH_ALLOW_EMPTY_VALUE)};
  if (allow_empty != nullptr) {
    openapi_expect_boolean(
        *allow_empty, base, "allowEmptyValue"sv,
        "The Parameter Object allowEmptyValue must be a boolean");
  }

  // Both forms carry this pair from 3.2 onwards, and in 3.1 the field table
  // for a `content` form has already turned both of them down, so where this
  // sits is what both revisions ask for
  openapi_check_examples(
      value, base,
      "The Parameter Object example and examples are mutually exclusive",
      "The Parameter Object examples must be an object", walk);

  if (content != nullptr) {
    const auto content_location{openapi_child(base, "content"sv)};
    openapi_check_content(*content, content_location,
                          "The Parameter Object content must be an object",
                          walk);
    // OpenAPI Specification 3.1.1, Section 4.8.12: "The map MUST only contain
    // one entry"
    if (content->size() != 1) {
      throw OpenAPIError{
          content_location,
          "The Parameter Object content must hold exactly one entry"};
    }

    walk.parameters.insert_or_assign(
        location, std::pair{JSON::String{parameter_name},
                            JSON::String{parameter_location}});
    return {parameter_name, parameter_location};
  }

  openapi_expect_schema(*schema, openapi_child(base, "schema"sv),
                        "A Schema Object must be an object or a boolean", walk);

  // Section 4.8.12.3 gives the styles an `in` column, and both revisions fill
  // it with the same four locations. 3.2.1 Section 4.12.3 goes on to close the
  // table, "Combinations not represented in this table are not permitted",
  // which 3.1 does not say. It does not need to: the column is the table, and
  // Section 4.8.21 reads it as binding where it derives a Header Object's own
  // restriction from it, "All traits that are affected by the location MUST be
  // applicable to a location of `header` (for example, `style`) ... and
  // `style`, if used, MUST be limited to `"simple"`". So each location admits
  // what its column says in both revisions, and only the `cookie` style 3.2
  // adds is held back
  const auto *style{value.try_at("style", OPENAPI_HASH_STYLE)};
  if (style != nullptr) {
    if (parameter_location == "path"sv) {
      openapi_expect_enumeration(
          *style, base, "style"sv, {"matrix"sv, "label"sv, "simple"sv},
          "The Parameter Object style must be a string",
          "The Parameter Object style is not one a path parameter admits");
    } else if (parameter_location == "header"sv) {
      openapi_expect_enumeration(
          *style, base, "style"sv, {"simple"sv},
          "The Parameter Object style must be a string",
          "The Parameter Object style is not one a header parameter admits");
    } else if (in_query) {
      openapi_expect_enumeration(
          *style, base, "style"sv,
          {"form"sv, "spaceDelimited"sv, "pipeDelimited"sv, "deepObject"sv},
          "The Parameter Object style must be a string",
          "The Parameter Object style is not one a query parameter admits");
    } else if (walk.version == OpenAPIVersion::OPENAPI_3_2) {
      // 3.2.1 Section 4.12.3 adds a `cookie` style, "Analogous to `form`, but
      // following [RFC6265] `Cookie` syntax rules", which no revision of 3.1
      // carries
      openapi_expect_enumeration(
          *style, base, "style"sv, {"form"sv, "cookie"sv},
          "The Parameter Object style must be a string",
          "The Parameter Object style is not one a cookie parameter admits");
    } else {
      openapi_expect_enumeration(
          *style, base, "style"sv, {"form"sv},
          "The Parameter Object style must be a string",
          "The Parameter Object style is not one a cookie parameter admits");
    }
  }

  const auto *explode{value.try_at("explode", OPENAPI_HASH_EXPLODE)};
  if (explode != nullptr) {
    openapi_expect_boolean(*explode, base, "explode"sv,
                           "The Parameter Object explode must be a boolean");
  }

  const auto *allow_reserved{
      value.try_at("allowReserved", OPENAPI_HASH_ALLOW_RESERVED)};
  if (allow_reserved != nullptr) {
    openapi_expect_boolean(
        *allow_reserved, base, "allowReserved"sv,
        "The Parameter Object allowReserved must be a boolean");
  }

  walk.parameters.insert_or_assign(location,
                                   std::pair{JSON::String{parameter_name},
                                             JSON::String{parameter_location}});
  return {parameter_name, parameter_location};
}

inline auto openapi_check_parameters_entry(const JSON &value,
                                           const Pointer &base,
                                           OpenAPIWalk &walk) -> void {
  if (openapi_is_reference(value)) {
    openapi_check_reference(value, base, OpenAPIObjectKind::Parameter, walk);
    return;
  }

  [[maybe_unused]] const auto identity{
      openapi_check_parameter(value, base, walk)};
}

// OpenAPI Specification 3.1.1, Sections 4.8.9 and 4.8.10: "The list MUST NOT
// include duplicated parameters. A unique parameter is defined by a
// combination of a name and location". A Reference Object names neither until
// it is followed, and following is eager, so what it leads to is compared
// alongside the parameters written out in place. One that could not be
// followed names nothing and is left out of the comparison
inline auto openapi_check_parameters(const JSON &value, const Pointer &base,
                                     const char *type_message,
                                     const char *duplicate_message,
                                     OpenAPIWalk &walk)
    -> std::vector<JSON::String> {
  openapi_expect_array(value, base, type_message);

  std::vector<JSON::String> result;
  result.reserve(value.size());
  // These own their strings, as an identity read back through a reference
  // borrows from a map the walk keeps writing to
  std::set<std::pair<JSON::String, JSON::String>> seen;
  // 3.2.1 Section 4.12, of `querystring`: it "MUST NOT appear more than once,
  // and MUST NOT appear in the same operation (or in the operation's path-item)
  // as any `in: "query"` parameters". Both halves hold of a single list on its
  // own, and what the two levels come to between them is settled where they
  // meet
  std::size_t querystrings{0};
  std::size_t index{0};
  for (const auto &parameter : value.as_array()) {
    const auto location{openapi_child(base, index)};
    if (openapi_is_reference(parameter)) {
      openapi_check_reference(parameter, location, OpenAPIObjectKind::Parameter,
                              walk);
      const auto *identity{openapi_parameter_identity(
          walk, openapi_location_uri(walk.base, location))};
      if (identity != nullptr && !seen.insert(*identity).second) {
        throw OpenAPIError{location, duplicate_message};
      }
    } else {
      const auto identity{openapi_check_parameter(parameter, location, walk)};
      if (!seen.insert({JSON::String{identity.first},
                        JSON::String{identity.second}})
               .second) {
        throw OpenAPIError{location, duplicate_message};
      }
    }

    result.push_back(openapi_location_uri(walk.base, location));
    index += 1;
  }

  bool query{false};
  for (const auto &position : result) {
    const auto *identity{openapi_parameter_identity(walk, position)};
    if (identity == nullptr) {
      continue;
    }

    if (identity->second == "querystring") {
      querystrings += 1;
    } else if (identity->second == "query") {
      query = true;
    }
  }

  if (querystrings > 1) {
    throw OpenAPIError{base,
                       "A querystring Parameter Object must not appear more "
                       "than once"};
  }

  // One list is enough to settle this when it holds both, as the list is
  // either an operation's own parameters or the ones every operation under a
  // Path Item starts from, and a list that is neither reaches no request. What
  // the two levels come to between them is settled where they meet, since
  // neither list alone shows it
  if (querystrings > 0 && query) {
    throw OpenAPIError{base, "A querystring Parameter Object must not appear "
                             "alongside a query Parameter Object"};
  }

  return result;
}

} // namespace sourcemeta::core

#endif
