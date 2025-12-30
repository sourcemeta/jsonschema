#ifndef SOURCEMETA_CORE_JSONSCHEMA_FRAME_H_
#define SOURCEMETA_CORE_JSONSCHEMA_FRAME_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <sourcemeta/core/jsonschema_types.h>
#include <sourcemeta/core/jsonschema_walker.h>

#include <cstdint>       // std::uint8_t
#include <functional>    // std::reference_wrapper
#include <map>           // std::map
#include <optional>      // std::optional
#include <set>           // std::set
#include <tuple>         // std::tuple
#include <unordered_set> // std::unordered_set
#include <utility>       // std::pair
#include <vector>        // std::vector

namespace sourcemeta::core {

/// @ingroup jsonschema
///
/// This class performs a static analysis pass on the input schema, computing
/// things such as the static identifiers and references of a schema.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonschema.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document =
///     sourcemeta::core::parse_json(R"JSON({
///   "$id": "https://www.example.com/schema",
///   "$schema": "https://json-schema.org/draft/2020-12/schema",
///   "items": { "$id": "foo", "type": "string" },
///   "properties": {
///     "foo": { "$anchor": "test", "type": "number" },
///     "bar": { "$ref": "#/properties/foo" }
///   }
/// })JSON");
///
/// sourcemeta::core::SchemaFrame
///   frame{sourcemeta::core::SchemaFrame::Mode::References};
///
/// frame.analyse(document,
///   sourcemeta::core::schema_walker,
///   sourcemeta::core::schema_resolver);
///
/// // IDs
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/foo"}));
///
/// // Anchors
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#test"}));
///
/// // Root Pointers
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/$id"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/$schema"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/items"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/items/$id"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/items/type"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/properties"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/properties/foo"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/properties/foo/$anchor"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/properties/foo/type"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/properties/bar"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/schema#/properties/bar/$ref"}));
///
/// // Subpointers
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/foo#/$id"}));
/// assert(frame.locations().contains({sourcemeta::core::SchemaReferenceType::Static,
///   "https://www.example.com/foo#/type"}));
///
/// // References
/// assert(frame.references().contains({sourcemeta::core::SchemaReferenceType::Static,
///   { "properties", "bar", "$ref" }}));
/// assert(frame.references().at({sourcemeta::core::SchemaReferenceType::Static,
///   { "properties", "bar", "$ref" }}).destination ==
///     "https://www.example.com/schema#/properties/foo");
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaFrame {
public:
  /// The mode of framing. More extensive analysis can be compute and memory
  /// intensive
  enum class Mode : std::uint8_t { Locations, References };

  SchemaFrame(const Mode mode) : mode_{mode} {}

  // Query the current mode that the schema frame was configured with
  [[nodiscard]] auto mode() const noexcept -> Mode { return this->mode_; }

  /// A single entry in a JSON Schema reference map
  struct ReferencesEntry {
    JSON::String original;
    JSON::String destination;
    // TODO: This string can be a `string_view` over the `destination`
    std::optional<JSON::String> base;
    // TODO: This string can be a `string_view` over the `destination`
    std::optional<JSON::String> fragment;
  };

  /// A JSON Schema reference map is a mapping of a JSON Pointer
  /// of a subschema to a destination static reference URI.
  /// For convenience, the value consists of the URI on its entirety,
  /// but also broken down by its potential fragment component.
  /// The reference type is part of the key as it is possible to
  /// have a static and a dynamic reference to the same location
  /// on the same schema object.
  using References =
      std::map<std::pair<SchemaReferenceType, Pointer>, ReferencesEntry>;

#if defined(__GNUC__)
#pragma GCC diagnostic push
// GCC believes that a member of an enum class (which is namespaced by
// definition), can shadow an alias defined even on a different namespace.
#pragma GCC diagnostic ignored "-Wshadow"
#endif
  /// @ingroup jsonschema
  /// The type of a location frame
  enum class LocationType : std::uint8_t {
    Resource,
    Anchor,
    // TODO: Distinguish between a Pointer and a Keyword
    Pointer,
    Subschema
  };
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  /// A location entry
  struct Location {
    // TODO: Turn this into a weak pointer
    std::optional<Pointer> parent;
    LocationType type;
    std::optional<JSON::String> root;
    JSON::String base;
    // TODO: Turn this into a weak pointer
    Pointer pointer;
    // TODO: Turn this into a weak pointer
    Pointer relative_pointer;
    JSON::String dialect;
    JSON::String base_dialect;
  };

  /// A JSON Schema reference frame is a mapping of URIs to schema identifiers,
  /// JSON Pointers within the schema, and subschemas dialects. We call it
  /// reference frame as this mapping is essential for resolving references.
  using Locations =
      // While it might seem weird that we namespace the location URIs with a
      // reference type, it is essential for distinguishing schema resource URIs
      // from `$recursiveRef: true` on another place of the schema schema
      // resource, as otherwise they would both have the exact same URI, but
      // point to different places.
      std::map<std::pair<SchemaReferenceType, JSON::String>, Location>;

  /// A set of paths to frame within a schema wrapper
  using Paths = std::set<Pointer>;

  /// Export the frame entries as JSON
  [[nodiscard]] auto to_json(
      const std::optional<PointerPositionTracker> &tracker = std::nullopt) const
      -> JSON;

  /// Analyse a schema or set of schemas from a given root. Passing
  /// multiple paths that have any overlap is undefined behaviour
  auto
  analyse(const JSON &root, const SchemaWalker &walker,
          const SchemaResolver &resolver,
          const std::optional<JSON::String> &default_dialect = std::nullopt,
          const std::optional<JSON::String> &default_id = std::nullopt,
          const Paths &paths = {empty_pointer}) -> void;

  /// Access the analysed schema locations
  [[nodiscard]] auto locations() const noexcept -> const Locations &;

  /// Access the analysed schema references
  [[nodiscard]] auto references() const noexcept -> const References &;

  /// Check whether the analysed schema has no external references
  [[nodiscard]] auto standalone() const -> bool;

  /// Get the vocabularies associated with a location entry
  [[nodiscard]] auto vocabularies(const Location &location,
                                  const SchemaResolver &resolver) const
      -> Vocabularies;

  /// Get the URI associated with a location entry
  [[nodiscard]] auto
  uri(const Location &location,
      const Pointer &relative_schema_location = empty_pointer) const
      -> JSON::String;

  /// Get the location associated by traversing a pointer from another location
  [[nodiscard]] auto traverse(const Location &location,
                              const Pointer &relative_schema_location) const
      -> const Location &;

  /// Get the location associated with a given URI
  [[nodiscard]] auto traverse(const JSON::String &uri) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Turn an absolute pointer into a location URI
  [[nodiscard]] auto uri(const Pointer &pointer) const
      -> std::optional<std::reference_wrapper<const JSON::String>>;

  /// Try to dereference a reference location into its destination location
  [[nodiscard]] auto
  dereference(const Location &location,
              const Pointer &relative_schema_location = empty_pointer) const
      -> std::pair<SchemaReferenceType,
                   std::optional<std::reference_wrapper<const Location>>>;

  /// Find all references to a given location pointer
  [[nodiscard]] auto references_to(const Pointer &pointer) const -> std::vector<
      std::reference_wrapper<const typename References::value_type>>;

private:
  Mode mode_;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  Locations locations_;
  References references_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::core

#endif
