#ifndef SOURCEMETA_CORE_OPENAPI_REFERENCE_H_
#define SOURCEMETA_CORE_OPENAPI_REFERENCE_H_

#include <sourcemeta/core/openapi.h>

#include "helpers.h"

#include <cassert>     // assert
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_REF{JSON::Object::hash("$ref"sv)};

// A Reference Object stands in for most of the Objects the Components Object
// holds, and the meta-schema tells the two apart by whether `$ref` is present
inline auto openapi_is_reference(const JSON &value) -> bool {
  return value.is_object() && value.try_at("$ref", OPENAPI_HASH_REF) != nullptr;
}

// OpenAPI Specification 3.1.1, Section 4.8.23: "A simple object to allow
// referencing other components in the OpenAPI Description". Only a value the
// predicate above accepts may be read as one, which is what makes the
// identifier below something rather than nothing
inline auto openapi_check_reference(const JSON &value, const Pointer &base,
                                    const OpenAPIObjectKind expected,
                                    OpenAPIWalk &walk) -> void {
  openapi_record(walk, base, OpenAPIObjectKind::Reference);
  // OpenAPI Specification 3.1.1, Section 4.8.23: "$ref | string | REQUIRED.
  // The reference identifier. This MUST be in the form of a URI"
  const auto *target{value.try_at("$ref", OPENAPI_HASH_REF)};
  assert(target != nullptr);
  const auto reference{openapi_expect_uri_reference(
      *target, base, "$ref"sv,
      "The Reference Object identifier must be a "
      "string",
      "The Reference Object identifier must be a URI reference")};

  openapi_check_optional_string(
      value, base, "summary"sv, OPENAPI_HASH_SUMMARY,
      "The Reference Object summary must be a string");

  openapi_check_optional_string(
      value, base, "description"sv, OPENAPI_HASH_DESCRIPTION,
      "The Reference Object description must be a string");

  // OpenAPI Specification 3.1.1, Section 4.8.23: "This object cannot be
  // extended with additional properties, and any properties added SHALL be
  // ignored". Ignoring is what the specification asks for, so no member beyond
  // the three above is read and none is turned down either

  // Following comes last, so that what is wrong with this Object is reported
  // before whatever may be wrong at the far end of it
  openapi_follow_reference(reference, openapi_child(base, "$ref"sv), expected,
                           walk);
}

// Most of the Objects this specification defines may stand in for themselves
// or be written as a Reference Object, and which one a value is comes down to
// the predicate above. The kind is what the reference is held to, and the
// check is what the Object itself goes through
template <OpenAPIObjectKind Kind, auto Check>
inline auto openapi_check_or_reference(const JSON &value, const Pointer &base,
                                       OpenAPIWalk &walk) -> void {
  if (openapi_is_reference(value)) {
    openapi_check_reference(value, base, Kind, walk);
    return;
  }

  Check(value, base, walk);
}

} // namespace sourcemeta::core

#endif
