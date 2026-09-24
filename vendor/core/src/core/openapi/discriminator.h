#ifndef SOURCEMETA_CORE_OPENAPI_DISCRIMINATOR_H_
#define SOURCEMETA_CORE_OPENAPI_DISCRIMINATOR_H_

#include <sourcemeta/core/openapi.h>

#include "components.h"
#include "helpers.h"

#include <optional>    // std::optional, std::nullopt
#include <set>         // std::set
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace sourcemeta::core {

constexpr auto OPENAPI_HASH_DISCRIMINATOR{
    JSON::Object::hash("discriminator"sv)};
constexpr auto OPENAPI_HASH_MAPPING{JSON::Object::hash("mapping"sv)};
constexpr auto OPENAPI_HASH_DEFAULT_MAPPING{
    JSON::Object::hash("defaultMapping"sv)};
constexpr auto OPENAPI_HASH_PROPERTY_NAME{JSON::Object::hash("propertyName"sv)};

/// Where a Discriminator Object names a schema, by the name of a component or
/// by URI. OpenAPI Specification 3.1.1, Section 4.3 lists the URI form of a
/// `mapping` among the fields that connect the documents of a description, and
/// Section 4.3.3 lists the name form among the connections it makes by name,
/// so either way one of these is a place the description reaches for
struct OpenAPIDiscriminator {
  /// Where the mapping value sits, as a pointer from the root of the document
  Pointer origin;
  /// Where it points, resolved and canonicalised
  JSON::String destination;
  /// What it resolved against, which is the nearest identifier an enclosing
  /// schema declares for the URI form, and the description itself for the
  /// name form
  JSON::String scope;
};

// OpenAPI Specification 3.1.1, Section 4.8.25: a `mapping` entry "maps a
// specific property value to either a different schema component name, or to a
// schema identified by a URI". Only the latter is a reference, as Section
// 4.3.3 lists the name form among the connections a description makes by name,
// and Section 4.8.25 settles which of the two a value is:
//
//   The behavior of a `mapping` value that is both a valid schema name and a
//   valid relative URI reference is implementation-defined, but it is
//   RECOMMENDED that it be treated as a schema name. To ensure that an
//   ambiguous value (e.g. `"foo"`) is treated as a relative URI reference by
//   all implementations, authors MUST prefix it with the `"."` path segment
//
// OpenAPI Specification 3.2.1, Section 4.25.3 says the same of the default a
// Discriminator Object may declare beside the map, which is what holds the two
// of them to the one rule: "The behavior of a `mapping` value or
// `defaultMapping` value that is both a valid schema name and a valid relative
// URI reference is implementation-defined"
//
// A schema name is what the Components Object takes as a key, and the path
// segment an author writes to force the other reading is no such key. Whether
// a value holds up as one is asked of the value alone rather than of what the
// description declares, as Section 4.8.25 calls `"foo"` ambiguous without
// regard to whether a component goes by that name
// Keep what one of those names, where it names a schema by URI rather than by
// the name a component goes by. Section 4.8.25 types a mapping as holding
// "schema names or URI references", so a value that holds up as neither is one
// nothing can read, which is what every other field this specification types
// as a URI is held to as well
inline auto openapi_record_discriminator(
    std::vector<OpenAPIDiscriminator> &result, const JSON::String &base,
    Pointer origin, const JSON &value, const JSON::String &scope) -> void {
  if (!value.is_string()) {
    throw OpenAPIError{
        base, std::move(origin),
        "A Discriminator Object mapping must name a schema with a string"};
  }

  // Section 4.3.3 lists the name form among the connections a description
  // makes by name rather than by URI, and has one resolve "from the entry
  // document, rather than the current document". So a name stands for the
  // schema that the Components Object of the description holds under it,
  // wherever the Discriminator Object naming it happens to sit
  if (openapi_is_component_key(value.to_string())) {
    Pointer named;
    named.push_back(JSON::String{"components"});
    named.push_back(JSON::String{"schemas"});
    named.push_back(JSON::String{value.to_string()});
    result.push_back({.origin = std::move(origin),
                      .destination = openapi_location_uri(base, named),
                      .scope = base});
    return;
  }

  if (!URI::is_uri_reference(value.to_string())) {
    throw OpenAPIError{base, std::move(origin),
                       "A Discriminator Object mapping must name a schema by "
                       "the name of a component or by a URI reference"};
  }

  const auto destination{openapi_resolve_uri(value.to_string(), scope)};
  if (!destination.has_value()) {
    return;
  }

  result.push_back({.origin = std::move(origin),
                    .destination = destination.value().recompose(),
                    .scope = scope});
}

// Every schema that a Discriminator Object of the document names.
// Section 4.6 has a relative reference of a Schema Object resolve against
// "the nearest parent `$id`", which is the base that framing the schemas
// settles for wherever the Discriminator Object sits
inline auto
openapi_discriminators(const JSON &document, const SchemaFrame &schemas,
                       const JSON::String &base, const SchemaWalker &walker,
                       const SchemaResolver &resolver)
    -> std::vector<OpenAPIDiscriminator> {
  std::vector<OpenAPIDiscriminator> result;
  // A schema that declares an identifier of its own is registered under every
  // base it can be reached by, and what it holds is the one thing whichever
  // way it is reached
  std::set<JSON::String> seen;
  schemas.for_each_subschema([&document, &schemas, &base, &result, &seen,
                              &walker,
                              &resolver](const auto &location) -> void {
    const auto *schema{try_get(document, location.pointer)};
    if (schema == nullptr || !schema->is_object()) {
      return;
    }

    const auto *discriminator{
        schema->try_at("discriminator", OPENAPI_HASH_DISCRIMINATOR)};
    if (discriminator == nullptr || !discriminator->is_object()) {
      return;
    }

    auto origin{to_pointer(location.pointer)};
    if (!seen.insert(to_string(origin)).second) {
      return;
    }

    // Section 4.8.24.2 lists `discriminator` among the keywords that the
    // dialect this specification publishes is made of, and Section 4.8.24.5
    // has a Schema Object read under whichever dialect it declares. So a
    // schema written against one that leaves the keyword out holds no
    // Discriminator Object at all, however the member happens to be spelled,
    // and what a keyword amounts to where it sits is the walker's to say
    const auto &vocabularies{schemas.vocabularies(location, resolver)};
    if (walker("discriminator", vocabularies).type ==
        SchemaKeywordType::Unknown) {
      return;
    }

    const JSON::String scope{location.base};
    origin.push_back(JSON::String{"discriminator"});

    // Section 4.8.25 marks this one "**REQUIRED**. The name of the property in
    // the payload that will hold the discriminating value", and 3.2.1 Section
    // 4.25 says as much, so a Discriminator Object the dialect does define
    // carries one or it is no such Object. The dialect 3.2 publishes leaves it
    // out of its own list of required fields where the one 3.1 publishes keeps
    // it, and Section 4.8 leaves the text authoritative where the two differ
    const auto *property_name{
        discriminator->try_at("propertyName", OPENAPI_HASH_PROPERTY_NAME)};
    if (property_name == nullptr) {
      throw OpenAPIError{
          base, std::move(origin),
          "The Discriminator Object must declare a property name"};
    }

    if (!property_name->is_string()) {
      throw OpenAPIError{
          base, origin.concat(JSON::String{"propertyName"}),
          "The Discriminator Object property name must be a string"};
    }

    const auto *mapping{discriminator->try_at("mapping", OPENAPI_HASH_MAPPING)};
    if (mapping != nullptr && mapping->is_object()) {
      const auto mapped{origin.concat(JSON::String{"mapping"})};
      for (const auto &entry : mapping->as_object()) {
        openapi_record_discriminator(result, base, mapped.concat(entry.first),
                                     entry.second, scope);
      }
    }

    // OpenAPI Specification 3.2.1, Section 4.25 gives a Discriminator Object a
    // default of its own, which is "the schema name or URI reference to a
    // schema" just as every entry of the map beside it is. Only the dialect of
    // that revision defines the field, which is what settles whether there is
    // one to read rather than what the document says of itself.
    //
    // 3.2.1 Section 4.25.1 goes on to require one wherever the discriminating
    // property is optional, which is a demand on what the schema holding it
    // says of its own properties. Reading that far into a Schema Object is
    // the business of whatever understands JSON Schema, so it is left there
    if (!vocabularies.contains(SchemaVocabularies::Known::OPENAPI_3_2_BASE)) {
      return;
    }

    const auto *fallback{
        discriminator->try_at("defaultMapping", OPENAPI_HASH_DEFAULT_MAPPING)};
    if (fallback != nullptr) {
      openapi_record_discriminator(
          result, base, origin.concat(JSON::String{"defaultMapping"}),
          *fallback, scope);
    }
  });

  return result;
}

// Whether the schemas of the document hold what a mapping names. Section
// 4.8.25 has one name "a schema identified by a URI", and framing a document
// records the places of it that are no schema of their own as well, so landing
// on one of those is landing on nothing this was after
inline auto
openapi_discriminator_lands(const SchemaFrame &schemas,
                            const OpenAPIDiscriminator &discriminator) -> bool {
  const auto location{schemas.traverse(discriminator.destination)};
  return location.has_value() &&
         location.value().get().type != SchemaFrame::LocationType::Pointer;
}

} // namespace sourcemeta::core

#endif
