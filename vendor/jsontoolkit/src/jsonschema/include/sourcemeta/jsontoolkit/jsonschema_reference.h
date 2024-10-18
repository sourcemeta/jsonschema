#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_REFERENCE_H_
#define SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_REFERENCE_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
#include <sourcemeta/jsontoolkit/jsonschema_export.h>
#endif

#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/jsonschema_resolver.h>
#include <sourcemeta/jsontoolkit/jsonschema_walker.h>

#include <cstdint>  // std::uint8_t
#include <map>      // std::map
#include <optional> // std::optional
#include <string>   // std::string
#include <tuple>    // std::tuple
#include <utility>  // std::pair

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonschema
/// The reference type
enum class ReferenceType : std::uint8_t { Static, Dynamic };

#if defined(__GNUC__)
#pragma GCC diagnostic push
// GCC believes that a member of an enum class (which is namespaced by
// definition), can shadow an alias defined even on a different namespace.
#pragma GCC diagnostic ignored "-Wshadow"
#endif
/// @ingroup jsonschema
/// The reference entry type
enum class ReferenceEntryType : std::uint8_t { Resource, Anchor, Pointer };
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/// @ingroup jsonschema
/// A single frame in a JSON Schema reference frame
struct ReferenceFrameEntry {
  const ReferenceEntryType type;
  const std::optional<std::string> root;
  const std::string base;
  const Pointer pointer;
  const Pointer relative_pointer;
  const std::string dialect;
};

/// @ingroup jsonschema
/// A JSON Schema reference frame is a mapping of URIs to schema identifiers,
/// JSON Pointers within the schema, and subschemas dialects. We call it
/// reference frame as this mapping is essential for resolving references.
using ReferenceFrame =
    std::map<std::pair<ReferenceType, std::string>, ReferenceFrameEntry>;

/// @ingroup jsonschema
/// A single entry in a JSON Schema reference map
struct ReferenceMapEntry {
  const std::string destination;
  const std::optional<std::string> base;
  const std::optional<std::string> fragment;
};

/// @ingroup jsonschema
/// A JSON Schema reference map is a mapping of a JSON Pointer
/// of a subschema to a destination static reference URI.
/// For convenience, the value consists of the URI on its entirety,
/// but also broken down by its potential fragment component.
/// The reference type is part of the key as it is possible to
/// have a static and a dynamic reference to the same location
/// on the same schema object.
using ReferenceMap =
    std::map<std::pair<ReferenceType, Pointer>, ReferenceMapEntry>;

/// @ingroup jsonschema
///
/// This function computes the static identifiers and references of a schema.
/// Each entry consists in the URI of the resulting schema, the JSON Pointer
/// within that resulting schema, and the dialect URI of the resulting
/// subschema.
///
/// Note that dynamic references cannot be resolved without the presence of an
/// instance. Therefore, framing does what it can with dynamic references to let
/// a higher level evaluate function perform its work.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/json.h>
/// #include <sourcemeta/jsontoolkit/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::jsontoolkit::JSON document =
///     sourcemeta::jsontoolkit::parse(R"JSON({
///   "$id": "https://www.example.com/schema",
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$id": "foo", "type": "string" },
///   "properties": {
///     "foo": { "$anchor": "test", "type": "number" },
///     "bar": { "$ref": "#/properties/foo" }
///   }
/// })JSON");
///
/// sourcemeta::jsontoolkit::ReferenceFrame frame;
/// sourcemeta::jsontoolkit::ReferenceMap references;
/// sourcemeta::jsontoolkit::frame(document, frame, references,
///                                sourcemeta::jsontoolkit::default_schema_walker,
///                                sourcemeta::jsontoolkit::official_resolver);
///
/// // IDs
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/foo"}));
///
/// // Anchors
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#test"}));
///
/// // Root Pointers
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/$id"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/$schema"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/items"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/items/$id"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/items/type"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/properties"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/properties/foo"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/properties/foo/$anchor"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/properties/foo/type"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/properties/bar"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/schema#/properties/bar/$ref"}));
///
/// // Subpointers
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/foo#/$id"}));
/// assert(frame.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   "https://www.example.com/foo#/type"}));
///
/// // References
/// assert(references.contains({sourcemeta::jsontoolkit::ReferenceType::Static,
///   { "properties", "bar", "$ref" }}));
/// assert(references.at({sourcemeta::jsontoolkit::ReferenceType::Static,
///   { "properties", "bar", "$ref" }}).destination ==
///     "https://www.example.com/schema#/properties/foo");
/// ```
SOURCEMETA_JSONTOOLKIT_JSONSCHEMA_EXPORT
auto frame(const JSON &schema, ReferenceFrame &frame, ReferenceMap &references,
           const SchemaWalker &walker, const SchemaResolver &resolver,
           const std::optional<std::string> &default_dialect = std::nullopt,
           const std::optional<std::string> &default_id = std::nullopt) -> void;

} // namespace sourcemeta::jsontoolkit

#endif
