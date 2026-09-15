#ifndef SOURCEMETA_CORE_OPENAPI_CONTENT_H_
#define SOURCEMETA_CORE_OPENAPI_CONTENT_H_

#include <sourcemeta/core/openapi.h>

#include "example.h"
#include "helpers.h"
#include "reference.h"

#include <array>       // std::array
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_SCHEMA{JSON::Object::hash("schema"sv)};
constexpr auto OPENAPI_HASH_CONTENT{JSON::Object::hash("content"sv)};
constexpr auto OPENAPI_HASH_ENCODING{JSON::Object::hash("encoding"sv)};
constexpr auto OPENAPI_HASH_CONTENT_TYPE{JSON::Object::hash("contentType"sv)};
constexpr auto OPENAPI_HASH_HEADERS{JSON::Object::hash("headers"sv)};
constexpr auto OPENAPI_HASH_STYLE{JSON::Object::hash("style"sv)};
constexpr auto OPENAPI_HASH_EXPLODE{JSON::Object::hash("explode"sv)};
constexpr auto OPENAPI_HASH_ALLOW_RESERVED{
    JSON::Object::hash("allowReserved"sv)};
constexpr auto OPENAPI_HASH_REQUIRED{JSON::Object::hash("required"sv)};
constexpr auto OPENAPI_HASH_ITEM_SCHEMA{JSON::Object::hash("itemSchema"sv)};
constexpr auto OPENAPI_HASH_ITEM_ENCODING{JSON::Object::hash("itemEncoding"sv)};
constexpr auto OPENAPI_HASH_PREFIX_ENCODING{
    JSON::Object::hash("prefixEncoding"sv)};

constexpr std::array<JSON::StringView, 5> OPENAPI_ENCODING_FIELDS_3_1{
    {"contentType"sv, "headers"sv, "style"sv, "explode"sv, "allowReserved"sv}};

// OpenAPI Specification 3.2.1, Section 4.15 adds the nested encoding fields
constexpr std::array<JSON::StringView, 8> OPENAPI_ENCODING_FIELDS_3_2{
    {"contentType"sv, "headers"sv, "style"sv, "explode"sv, "allowReserved"sv,
     "encoding"sv, "prefixEncoding"sv, "itemEncoding"sv}};

constexpr std::array<JSON::StringView, 4> OPENAPI_MEDIA_TYPE_FIELDS_3_1{
    {"schema"sv, "example"sv, "examples"sv, "encoding"sv}};

// OpenAPI Specification 3.2.1, Section 4.14 adds `itemSchema`, for a
// sequential media type, alongside the nested encoding fields
constexpr std::array<JSON::StringView, 7> OPENAPI_MEDIA_TYPE_FIELDS_3_2{
    {"schema"sv, "example"sv, "examples"sv, "encoding"sv, "itemSchema"sv,
     "prefixEncoding"sv, "itemEncoding"sv}};

// A Header Object only admits the serialisation fields alongside a `schema`,
// which is how the meta-schema reads it, holding them behind a dependent
// schema that a `content` form never reaches
constexpr std::array<JSON::StringView, 8> OPENAPI_HEADER_SCHEMA_FIELDS{
    {"description"sv, "required"sv, "deprecated"sv, "schema"sv, "style"sv,
     "explode"sv, "example"sv, "examples"sv}};

constexpr std::array<JSON::StringView, 4> OPENAPI_HEADER_CONTENT_FIELDS_3_1{
    {"description"sv, "required"sv, "deprecated"sv, "content"sv}};

// OpenAPI Specification 3.2.1, Section 4.21.1.1 moves `example` and
// `examples` into the Header Object's common fields, of which it says "these
// fields MAY be used with either `content` or `schema`", the same move it
// makes in the Parameter Object
constexpr std::array<JSON::StringView, 6> OPENAPI_HEADER_CONTENT_FIELDS_3_2{
    {"description"sv, "required"sv, "deprecated"sv, "content"sv, "example"sv,
     "examples"sv}};

inline auto openapi_check_header_or_reference(const JSON &value,
                                              const Pointer &base,
                                              OpenAPIWalk &walk) -> void;

// The Header Objects an Object may map names to, which the Response Object
// and the Encoding Object each declare the same way
inline auto openapi_check_headers(const JSON &value, const Pointer &base,
                                  const char *message, OpenAPIWalk &walk)
    -> void {
  const auto *headers{value.try_at("headers", OPENAPI_HASH_HEADERS)};
  if (headers == nullptr) {
    return;
  }

  const auto location{openapi_child(base, "headers"sv)};
  openapi_expect_object(*headers, location, message);
  for (const auto &entry : headers->as_object()) {
    openapi_check_header_or_reference(
        entry.second, openapi_child(location, entry.first), walk);
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.15: "A single encoding definition
// applied to a single schema property"
inline auto openapi_check_encoding(const JSON &value, const Pointer &base,
                                   OpenAPIWalk &walk) -> void;

// OpenAPI Specification 3.2.1, Sections 4.14 and 4.15 give a Media Type
// Object and an Encoding Object the same three ways of encoding what is inside
// them: by name, by position, and one for every item
inline auto openapi_check_nested_encoding(const JSON &value,
                                          const Pointer &base,
                                          const char *object,
                                          const char *positional,
                                          OpenAPIWalk &walk) -> void {
  const auto *named{value.try_at("encoding", OPENAPI_HASH_ENCODING)};

  // Section 4.14, of `encoding`: "This field MUST NOT be present if
  // `prefixEncoding` or `itemEncoding` are present", and each of those two
  // says the same of `encoding` in turn. Section 4.15 defines all three by
  // reference to that Object, and the published meta-schema holds the pair to
  // the same rule in both places
  if (named != nullptr &&
      (value.try_at("prefixEncoding", OPENAPI_HASH_PREFIX_ENCODING) !=
           nullptr ||
       value.try_at("itemEncoding", OPENAPI_HASH_ITEM_ENCODING) != nullptr)) {
    throw OpenAPIError{base,
                       "Encoding by name and encoding by position are mutually "
                       "exclusive"};
  }

  if (named != nullptr) {
    const auto location{openapi_child(base, "encoding"sv)};
    openapi_expect_object(*named, location, object);
    for (const auto &entry : named->as_object()) {
      openapi_check_encoding(entry.second, openapi_child(location, entry.first),
                             walk);
    }
  }

  const auto *prefix{
      value.try_at("prefixEncoding", OPENAPI_HASH_PREFIX_ENCODING)};
  if (prefix != nullptr) {
    const auto location{openapi_child(base, "prefixEncoding"sv)};
    openapi_expect_array(*prefix, location, positional);
    std::size_t index{0};
    for (const auto &entry : prefix->as_array()) {
      openapi_check_encoding(entry, openapi_child(location, index), walk);
      index += 1;
    }
  }

  const auto *item{value.try_at("itemEncoding", OPENAPI_HASH_ITEM_ENCODING)};
  if (item != nullptr) {
    openapi_check_encoding(*item, openapi_child(base, "itemEncoding"sv), walk);
  }
}

inline auto openapi_check_encoding(const JSON &value, const Pointer &base,
                                   OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Encoding);
  openapi_expect_object(value, base, "The Encoding Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_ENCODING_FIELDS_3_1, OPENAPI_ENCODING_FIELDS_3_2, base,
      "The Encoding Object does not define this field", walk);

  // The specification says media type definitions "SHOULD be in compliance
  // with RFC6838", which is not a requirement, so only the type is checked
  openapi_check_optional_string(
      value, base, "contentType"sv, OPENAPI_HASH_CONTENT_TYPE,
      "The Encoding Object content type must be a string");

  openapi_check_headers(value, base,
                        "The Encoding Object headers must be an object", walk);

  const auto *style{value.try_at("style", OPENAPI_HASH_STYLE)};
  if (style != nullptr) {
    openapi_expect_enumeration(
        *style, base, "style"sv,
        {"form"sv, "spaceDelimited"sv, "pipeDelimited"sv, "deepObject"sv},
        "The Encoding Object style must be a string",
        "The Encoding Object style is not one this specification defines");
  }

  const auto *explode{value.try_at("explode", OPENAPI_HASH_EXPLODE)};
  if (explode != nullptr) {
    openapi_expect_boolean(*explode, base, "explode"sv,
                           "The Encoding Object explode must be a boolean");
  }

  const auto *allow_reserved{
      value.try_at("allowReserved", OPENAPI_HASH_ALLOW_RESERVED)};
  if (allow_reserved != nullptr) {
    openapi_expect_boolean(
        *allow_reserved, base, "allowReserved"sv,
        "The Encoding Object allowReserved must be a boolean");
  }

  openapi_check_nested_encoding(
      value, base, "The Encoding Object encoding must be an object",
      "The Encoding Object prefix encoding must be an array", walk);
}

// OpenAPI Specification 3.1.1, Section 4.8.14: "Each Media Type Object
// provides schema and examples for the media type identified by its key"
inline auto openapi_check_media_type(const JSON &value, const Pointer &base,
                                     OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::MediaType);
  openapi_expect_object(value, base, "The Media Type Object must be an object");
  openapi_reject_unknown_fields(
      value, OPENAPI_MEDIA_TYPE_FIELDS_3_1, OPENAPI_MEDIA_TYPE_FIELDS_3_2, base,
      "The Media Type Object does not define this field", walk);

  const auto *schema{value.try_at("schema", OPENAPI_HASH_SCHEMA)};
  if (schema != nullptr) {
    openapi_expect_schema(*schema, openapi_child(base, "schema"sv),
                          "A Schema Object must be an object or a boolean",
                          walk);
  }

  openapi_check_examples(
      value, base,
      "The Media Type Object example and examples are mutually exclusive",
      "The Media Type Object examples must be an object", walk);

  // Section 4.14: "itemSchema | Schema Object", for a sequential media type,
  // which is a fifth position where framing hands off to JSON Schema
  const auto *item_schema{value.try_at("itemSchema", OPENAPI_HASH_ITEM_SCHEMA)};
  if (item_schema != nullptr) {
    openapi_expect_schema(*item_schema, openapi_child(base, "itemSchema"sv),
                          "A Schema Object must be an object or a boolean",
                          walk);
  }

  openapi_check_nested_encoding(
      value, base, "The Media Type Object encoding must be an object",
      "The Media Type Object prefix encoding must be an array", walk);
}

// OpenAPI Specification 3.2.1, Section 4.7 holds `mediaTypes` under the
// Components Object as "Media Type Object | Reference Object", which is the
// first position where a Reference Object may stand in for one
inline auto openapi_check_media_type_or_reference(const JSON &value,
                                                  const Pointer &base,
                                                  OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::MediaType,
                             openapi_check_media_type>(value, base, walk);
}

// The map that the Request Body, Response, Parameter and Header Objects all
// key by media type. Section 4.5 says those definitions "SHOULD be in
// compliance with RFC6838", so the keys carry no requirement to enforce.
// OpenAPI Specification 3.2.1 widens what the values may be in all four of
// those Objects, from "Map[string, Media Type Object]" to "Map[string, Media
// Type Object | Reference Object]", which is what its Components Object entry
// for `mediaTypes` is there to be referenced from
inline auto openapi_check_content(const JSON &value, const Pointer &location,
                                  const char *type_message, OpenAPIWalk &walk)
    -> void {
  openapi_expect_object(value, location, type_message);
  for (const auto &entry : value.as_object()) {
    const auto entry_location{openapi_child(location, entry.first)};
    if (walk.version == OpenAPIVersion::OPENAPI_3_2) {
      openapi_check_media_type_or_reference(entry.second, entry_location, walk);
    } else {
      openapi_check_media_type(entry.second, entry_location, walk);
    }
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.21: "Describes a single header for
// HTTP responses and for individual parts in `multipart` representations"
inline auto openapi_check_header(const JSON &value, const Pointer &base,
                                 OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Header);
  openapi_expect_object(value, base, "The Header Object must be an object");

  const auto *schema{value.try_at("schema", OPENAPI_HASH_SCHEMA)};
  const auto *content{value.try_at("content", OPENAPI_HASH_CONTENT)};

  // OpenAPI Specification 3.1.1, Section 4.8.21: "The `schema` field and
  // `content` field are mutually exclusive", and one of them has to be there
  if (schema != nullptr && content != nullptr) {
    throw OpenAPIError{
        base, "The Header Object schema and content are mutually exclusive"};
  }

  if (schema == nullptr && content == nullptr) {
    throw OpenAPIError{base,
                       "The Header Object must declare a schema or a content"};
  }

  if (schema == nullptr) {
    openapi_reject_unknown_fields(
        value, OPENAPI_HEADER_CONTENT_FIELDS_3_1,
        OPENAPI_HEADER_CONTENT_FIELDS_3_2, base,
        "The Header Object does not define this field", walk);
  } else {
    openapi_reject_unknown_fields(
        value, OPENAPI_HEADER_SCHEMA_FIELDS, base,
        "The Header Object does not define this field");
  }

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Header Object description must be a string");

  const auto *required{value.try_at("required", OPENAPI_HASH_REQUIRED)};
  if (required != nullptr) {
    openapi_expect_boolean(*required, base, "required"sv,
                           "The Header Object required must be a boolean");
  }

  const auto *deprecated{value.try_at("deprecated", OPENAPI_HASH_DEPRECATED)};
  if (deprecated != nullptr) {
    openapi_expect_boolean(*deprecated, base, "deprecated"sv,
                           "The Header Object deprecated must be a boolean");
  }

  // Both forms carry this pair from 3.2 onwards, and in 3.1 the field table
  // for a `content` form has already turned both of them down, so where this
  // sits is what both revisions ask for
  openapi_check_examples(
      value, base,
      "The Header Object example and examples are mutually exclusive",
      "The Header Object examples must be an object", walk);

  if (content != nullptr) {
    const auto location{openapi_child(base, "content"sv)};
    openapi_check_content(*content, location,
                          "The Header Object content must be an object", walk);
    // The meta-schema bounds this map at one entry in both directions
    if (content->size() != 1) {
      throw OpenAPIError{
          location, "The Header Object content must hold exactly one entry"};
    }

    return;
  }

  openapi_expect_schema(*schema, openapi_child(base, "schema"sv),
                        "A Schema Object must be an object or a boolean", walk);

  // OpenAPI Specification 3.1.1, Section 4.8.21 fixes the only style a Header
  // Object may name
  const auto *style{value.try_at("style", OPENAPI_HASH_STYLE)};
  if (style != nullptr) {
    openapi_expect_enumeration(*style, base, "style"sv, {"simple"sv},
                               "The Header Object style must be a string",
                               "The Header Object style must be simple");
  }

  const auto *explode{value.try_at("explode", OPENAPI_HASH_EXPLODE)};
  if (explode != nullptr) {
    openapi_expect_boolean(*explode, base, "explode"sv,
                           "The Header Object explode must be a boolean");
  }
}

inline auto openapi_check_header_or_reference(const JSON &value,
                                              const Pointer &base,
                                              OpenAPIWalk &walk) -> void {
  openapi_check_or_reference<OpenAPIObjectKind::Header, openapi_check_header>(
      value, base, walk);
}

} // namespace sourcemeta::core

#endif
