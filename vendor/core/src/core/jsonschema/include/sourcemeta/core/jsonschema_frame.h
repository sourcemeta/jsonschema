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
#include <unordered_map> // std::unordered_map
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
/// ```
class SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaFrame {
public:
  /// The mode of framing. More extensive analysis can be compute and memory
  /// intensive
  enum class Mode : std::uint8_t { Locations, References };

  SchemaFrame(const Mode mode) : mode_{mode} {}

  // We rely on internal caches that would be dangling otherwise
  SchemaFrame(const SchemaFrame &) = delete;
  auto operator=(const SchemaFrame &) -> SchemaFrame & = delete;
  SchemaFrame(SchemaFrame &&) = delete;
  auto operator=(SchemaFrame &&) -> SchemaFrame & = delete;

  // Query the current mode that the schema frame was configured with
  [[nodiscard]] auto mode() const noexcept -> Mode { return this->mode_; }

  /// A single entry in a JSON Schema reference map
  struct ReferencesEntry {
    std::string_view original;
    // TODO: This one is tricky to turn into a view, as there is no
    // location entry to point to if it is an external unresolved reference
    JSON::String destination;
    // Empty means no base
    std::string_view base;
    std::optional<std::string_view> fragment;
  };

  /// A JSON Schema reference map is a mapping of a JSON Pointer
  /// of a subschema to a destination static reference URI.
  /// For convenience, the value consists of the URI on its entirety,
  /// but also broken down by its potential fragment component.
  /// The reference type is part of the key as it is possible to
  /// have a static and a dynamic reference to the same location
  /// on the same schema object.
  using References =
      std::map<std::pair<SchemaReferenceType, WeakPointer>, ReferencesEntry>;

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
    std::optional<WeakPointer> parent;
    LocationType type;
    std::string_view base;
    WeakPointer pointer;
    std::size_t relative_pointer;
    std::string_view dialect;
    SchemaBaseDialect base_dialect;
    bool property_name;
    bool orphan;
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

  /// A list of paths to frame within a schema wrapper
  using Paths = std::vector<WeakPointer>;

  /// Export the frame entries as JSON
  [[nodiscard]] auto to_json(
      const std::optional<PointerPositionTracker> &tracker = std::nullopt) const
      -> JSON;

  /// Analyse a schema or set of schemas from a given root. Passing
  /// multiple paths that have any overlap is undefined behaviour
  auto analyse(const JSON &root, const SchemaWalker &walker,
               const SchemaResolver &resolver,
               std::string_view default_dialect = "",
               std::string_view default_id = "",
               const Paths &paths = {empty_weak_pointer}) -> void;

  /// Access the analysed schema locations
  [[nodiscard]] auto locations() const noexcept -> const Locations &;

  /// Access the analysed schema references
  [[nodiscard]] auto references() const noexcept -> const References &;

  /// Get a specific reference entry by type and pointer
  [[nodiscard]] auto reference(const SchemaReferenceType type,
                               const WeakPointer &pointer) const
      -> std::optional<std::reference_wrapper<const ReferencesEntry>>;

  /// Check whether the analysed schema has no external references
  [[nodiscard]] auto standalone() const -> bool;

  /// Get the root schema identifier (empty if none)
  [[nodiscard]] auto root() const noexcept -> const JSON::String &;

  /// Get the vocabularies associated with a location entry
  [[nodiscard]] auto vocabularies(const Location &location,
                                  const SchemaResolver &resolver) const
      -> Vocabularies;

  /// Get the URI associated with a location entry
  [[nodiscard]] auto
  uri(const Location &location,
      const WeakPointer &relative_schema_location = empty_weak_pointer) const
      -> JSON::String;

  /// Get the location associated by traversing a pointer from another location
  [[nodiscard]] auto traverse(const Location &location,
                              const WeakPointer &relative_schema_location) const
      -> const Location &;

  /// Get the location associated with a given URI
  [[nodiscard]] auto traverse(const std::string_view uri) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Get the location associated with a given pointer
  [[nodiscard]] auto traverse(const WeakPointer &pointer) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Get the location of a specific type associated with a given pointer
  [[nodiscard]] auto traverse(const WeakPointer &pointer,
                              const LocationType type) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Turn an absolute pointer into a location URI
  [[nodiscard]] auto uri(const WeakPointer &pointer) const
      -> std::optional<std::reference_wrapper<const JSON::String>>;

  /// Try to dereference a reference location into its destination location
  [[nodiscard]] auto dereference(
      const Location &location,
      const WeakPointer &relative_schema_location = empty_weak_pointer) const
      -> std::pair<SchemaReferenceType,
                   std::optional<std::reference_wrapper<const Location>>>;

  /// Iterate over all resource URIs in the frame
  auto for_each_resource_uri(
      const std::function<void(std::string_view)> &callback) const -> void;

  /// Iterate over all unresolved references (where destination cannot be
  /// traversed)
  auto for_each_unresolved_reference(
      const std::function<void(const WeakPointer &, const ReferencesEntry &)>
          &callback) const -> void;

  /// Check if there are any references to a given location pointer
  [[nodiscard]] auto has_references_to(const WeakPointer &pointer) const
      -> bool;

  /// Check if there are any references that go through a given location pointer
  [[nodiscard]] auto has_references_through(const WeakPointer &pointer) const
      -> bool;
  /// Check if there are any references that go through a given location pointer
  /// with a tail token
  [[nodiscard]] auto
  has_references_through(const WeakPointer &pointer,
                         const WeakPointer::Token &tail) const -> bool;

  /// Get the relative instance location pointer for a given location entry
  [[nodiscard]] auto relative_instance_location(const Location &location) const
      -> WeakPointer;

  /// Check if the frame has no analysed data
  [[nodiscard]] auto empty() const noexcept -> bool;

  /// Reset the frame, clearing all analysed data
  auto reset() -> void;

  /// Determines if a location could be evaluated during validation
  [[nodiscard]] auto is_reachable(const Location &location,
                                  const SchemaWalker &walker,
                                  const SchemaResolver &resolver) const -> bool;

private:
  auto populate_pointer_to_location() const -> void;
  auto populate_reachability(const SchemaWalker &walker,
                             const SchemaResolver &resolver) const -> void;

  Mode mode_;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  JSON::String root_;
  Locations locations_;
  References references_;
  mutable std::unordered_map<std::reference_wrapper<const WeakPointer>,
                             std::vector<const Location *>, WeakPointer::Hasher,
                             WeakPointer::Comparator>
      pointer_to_location_;
  mutable std::unordered_map<std::reference_wrapper<const WeakPointer>, bool,
                             WeakPointer::Hasher, WeakPointer::Comparator>
      reachability_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::core

#endif
