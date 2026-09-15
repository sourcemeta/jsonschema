#ifndef SOURCEMETA_CORE_OPENAPI_COMPONENTS_H_
#define SOURCEMETA_CORE_OPENAPI_COMPONENTS_H_

#include <sourcemeta/core/openapi.h>

#include "content.h"
#include "example.h"
#include "helpers.h"
#include "link.h"
#include "parameter.h"
#include "path_item.h"
#include "request_body.h"
#include "response.h"
#include "security.h"

#include <array>       // std::array
#include <set>         // std::set
#include <string_view> // std::string_view

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_COMPONENTS{JSON::Object::hash("components"sv)};
constexpr auto OPENAPI_HASH_SECURITY_SCHEMES{
    JSON::Object::hash("securitySchemes"sv)};

constexpr std::array<JSON::StringView, 10> OPENAPI_COMPONENTS_FIELDS_3_1{
    {"schemas"sv, "responses"sv, "parameters"sv, "examples"sv,
     "requestBodies"sv, "headers"sv, "securitySchemes"sv, "links"sv,
     "callbacks"sv, "pathItems"sv}};

// OpenAPI Specification 3.2.1, Section 4.7 adds `mediaTypes`
constexpr std::array<JSON::StringView, 11> OPENAPI_COMPONENTS_FIELDS_3_2{
    {"schemas"sv, "responses"sv, "parameters"sv, "examples"sv,
     "requestBodies"sv, "headers"sv, "securitySchemes"sv, "links"sv,
     "callbacks"sv, "pathItems"sv, "mediaTypes"sv}};

// The names the document declares as security schemes, read before the walk
// goes anywhere because the Components Object may hold a Path Item Object
// whose operations declare a requirement, and the order that Object writes its
// own members must not decide what a requirement may name. This runs before
// the Components Object has been checked, so it takes what is there and leaves
// being strict about the shape to that check
inline auto openapi_collect_security_schemes(const JSON &document,
                                             OpenAPIWalk &walk) -> void {
  const auto *components{
      document.try_at("components", OPENAPI_HASH_COMPONENTS)};
  if (components == nullptr || !components->is_object()) {
    return;
  }

  const auto *schemes{
      components->try_at("securitySchemes", OPENAPI_HASH_SECURITY_SCHEMES)};
  if (schemes == nullptr || !schemes->is_object()) {
    return;
  }

  for (const auto &entry : schemes->as_object()) {
    walk.security_schemes.insert(entry.first);
  }
}

// OpenAPI Specification 3.1.1, Section 4.8.30: "Lists the required security
// schemes to execute this operation". Every field is patterned, so unlike
// almost every other Object this one has no extension carve-out and a member
// named `x-` is a scheme name
// OpenAPI Specification 3.1.1, Section 4.8.7: "All the fixed fields declared
// above are objects that MUST use keys that match the regular expression:
// `^[a-zA-Z0-9\.\-_]+$`". Every member of that character class is ASCII, so
// reading the key one byte at a time turns down any other code point too
inline auto openapi_is_component_key(const JSON::StringView key) noexcept
    -> bool {
  for (const auto character : key) {
    if ((character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9') || character == '.' ||
        character == '-' || character == '_') {
      continue;
    }

    return false;
  }

  return !key.empty();
}

// OpenAPI Specification 3.1.1, Section 4.8.7: "Holds a set of reusable objects
// for different aspects of the OAS". What each entry of those maps holds is
// not read here, and the Schema Objects under `schemas` are never read at all,
// as their semantics belong to a JSON Schema implementation
inline auto openapi_check_components(const JSON &document, OpenAPIWalk &walk)
    -> void {
  const auto *components{
      document.try_at("components", OPENAPI_HASH_COMPONENTS)};
  if (components == nullptr) {
    return;
  }

  const Pointer base{"components"};
  openapi_record(walk, base, OpenAPIObjectKind::Components);
  if (!components->is_object()) {
    throw OpenAPIError{base, "The Components Object must be an object"};
  }

  openapi_reject_unknown_fields(
      *components, OPENAPI_COMPONENTS_FIELDS_3_1, OPENAPI_COMPONENTS_FIELDS_3_2,
      base, "The Components Object does not define this field", walk);

  for (const auto &entry : components->as_object()) {
    if (entry.first.starts_with(OPENAPI_EXTENSION_PREFIX)) {
      continue;
    }

    const auto location{openapi_child(base, entry.first)};
    if (!entry.second.is_object()) {
      throw OpenAPIError{location,
                         "The Components Object fields must each be an object"};
    }

    // Every entry of every map but one is an Object or a Reference Object,
    // and the specification types both as objects. What sits under `schemas`
    // is a Schema Object, and Section 4.8.24 states that "The empty schema
    // [...] MAY be represented by the boolean value `true` and a schema which
    // allows no instance to validate MAY be represented by the boolean value
    // `false`", which is also all that the meta-schema asserts of a Schema
    // Object position when it leaves those unvalidated
    const auto holds_schemas{entry.first == "schemas"sv};

    for (const auto &component : entry.second.as_object()) {
      if (component.first.empty()) {
        throw OpenAPIError{openapi_child(location, component.first),
                           "The Components Object keys must not be empty"};
      }

      if (!openapi_is_component_key(component.first)) {
        throw OpenAPIError{openapi_child(location, component.first),
                           "The Components Object keys may only hold letters, "
                           "digits, dots, hyphens and underscores"};
      }

      const auto entry_location{openapi_child(location, component.first)};
      if (holds_schemas) {
        openapi_expect_schema(component.second, entry_location,
                              "A Schema Object must be an object or a boolean",
                              walk);
        continue;
      }

      openapi_expect_object(component.second, entry_location,
                            "The Components Object entries must be objects");

      if (entry.first == "responses"sv) {
        openapi_check_response_or_reference(component.second, entry_location,
                                            walk);
      } else if (entry.first == "parameters"sv) {
        if (openapi_is_reference(component.second)) {
          openapi_check_reference(component.second, entry_location,
                                  OpenAPIObjectKind::Parameter, walk);
        } else {
          [[maybe_unused]] const auto identity{
              openapi_check_parameter(component.second, entry_location, walk)};
        }
      } else if (entry.first == "examples"sv) {
        openapi_check_example_or_reference(component.second, entry_location,
                                           walk);
      } else if (entry.first == "requestBodies"sv) {
        openapi_check_request_body_or_reference(component.second,
                                                entry_location, walk);
      } else if (entry.first == "headers"sv) {
        openapi_check_header_or_reference(component.second, entry_location,
                                          walk);
      } else if (entry.first == "securitySchemes"sv) {
        openapi_check_security_scheme_or_reference(component.second,
                                                   entry_location, walk);
      } else if (entry.first == "links"sv) {
        openapi_check_link_or_reference(component.second, entry_location, walk);
      } else if (entry.first == "callbacks"sv) {
        openapi_check_callbacks_or_reference(component.second, entry_location,
                                             walk);
      } else if (entry.first == "mediaTypes"sv) {
        openapi_check_media_type_or_reference(component.second, entry_location,
                                              walk);
      } else {
        openapi_check_path_item(component.second, entry_location, walk);
      }
    }
  }
}

} // namespace sourcemeta::core

#endif
