#ifndef SOURCEMETA_BLAZE_FOUNDATION_FRAME_H_
#define SOURCEMETA_BLAZE_FOUNDATION_FRAME_H_

#ifndef SOURCEMETA_BLAZE_FOUNDATION_EXPORT
#include <sourcemeta/blaze/foundation_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/blaze/foundation_error.h>
#include <sourcemeta/blaze/foundation_types.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <concepts>   // std::invocable
#include <cstdint>    // std::uint8_t
#include <deque>      // std::deque
#include <functional> // std::reference_wrapper
#include <map>        // std::map
#include <memory>     // std::unique_ptr
#include <optional>   // std::optional
#include <utility>    // std::pair
#include <vector>     // std::vector

namespace sourcemeta::blaze {

/// @ingroup foundation
///
/// This class performs a static analysis pass on the input schema, computing
/// things such as the static identifiers and references of a schema.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/blaze/foundation.h>
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
/// const sourcemeta::blaze::SchemaFrame frame{
///   sourcemeta::blaze::SchemaFrame::Mode::References, document,
///   sourcemeta::blaze::schema_walker,
///   sourcemeta::blaze::schema_resolver};
/// ```
class SOURCEMETA_BLAZE_FOUNDATION_EXPORT SchemaFrame {
public:
  /// The mode of framing. More extensive analysis can be compute and memory
  /// intensive. Each mode is a superset of the previous one. Note that
  /// sourcemeta::blaze::SchemaFrame::Mode::Root reports on a single schema,
  /// so framing a wrapper that holds more than one yields no locations
  ///
  /// sourcemeta::blaze::SchemaFrame::Mode::Locations and
  /// sourcemeta::blaze::SchemaFrame::Mode::References locate the schemas of
  /// the document rather than each of its JSON Pointers, and the latter also
  /// locates whatever place a reference names. Reach for
  /// sourcemeta::blaze::SchemaFrame::Mode::Pointers only to address a keyword
  /// or a value of the document by URI, as computing those locations tends to
  /// dominate the cost of framing
  enum class Mode : std::uint8_t { Root, Locations, References, Pointers };

  /// How a caller-provided default identifier relates to the one that the
  /// schema declares, if any
  enum class IdentifierMode : std::uint8_t {
    /// Register the default identifier in addition to the schema's own
    Additional,
    /// Register the default identifier only if the schema declares none
    Fallback
  };

  ~SchemaFrame();

  // We rely on internal caches that would be dangling otherwise
  SchemaFrame(const SchemaFrame &) = delete;
  auto operator=(const SchemaFrame &) -> SchemaFrame & = delete;
  SchemaFrame(SchemaFrame &&) = delete;
  auto operator=(SchemaFrame &&) -> SchemaFrame & = delete;

  // Query the current mode that the schema frame was configured with
  [[nodiscard]] auto mode() const noexcept -> Mode { return this->mode_; }

  /// A reference that the schema declares, as found by framing
  struct Reference {
    std::string_view original;
    // TODO: This one is tricky to turn into a view, as there is no
    // location entry to point to if it is an external unresolved reference
    sourcemeta::core::JSON::String destination;
    // Empty means no base
    std::string_view base;
    std::optional<std::string_view> fragment;
  };

#if defined(__GNUC__)
#pragma GCC diagnostic push
// GCC believes that a member of an enum class (which is namespaced by
// definition), can shadow an alias defined even on a different namespace.
#pragma GCC diagnostic ignored "-Wshadow"
#endif
  /// @ingroup foundation
  /// The type of a location frame
  enum class LocationType : std::uint8_t {
    Resource,
    Anchor,
    Pointer,
    Subschema
  };
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  /// A location entry
  struct Location {
    std::optional<sourcemeta::core::WeakPointer> parent;
    LocationType type;
    std::string_view base;
    sourcemeta::core::WeakPointer pointer;
    std::size_t relative_pointer;
    std::string_view dialect;
    SchemaBaseDialect base_dialect;
    bool property_name;
    bool orphan;
  };

  /// A list of paths to frame within a schema wrapper
  using Paths = std::vector<sourcemeta::core::WeakPointer>;

  /// Export the frame as JSON
  [[nodiscard]] auto to_json(
      const SchemaResolver &resolver,
      const std::optional<sourcemeta::core::PointerPositionTracker> &tracker =
          std::nullopt) const -> sourcemeta::core::JSON;

  /// Frame a schema or set of schemas from a given root. Passing multiple
  /// paths that have any overlap is undefined behaviour
  ///
  /// A frame is analysed once, on construction, and is immutable afterwards
  ///
  /// The resulting locations point into the schema rather than copying from
  /// it, so the schema must outlive the frame. The same goes for
  /// `default_dialect`, as a location that has no dialect of its own reports
  /// the default back as a view into what the caller passed. In contrast,
  /// `default_id` is copied, so it does not need to outlive this call
  SchemaFrame(const Mode mode, const sourcemeta::core::JSON &root,
              const SchemaWalker &walker, const SchemaResolver &resolver,
              std::string_view default_dialect = "",
              std::string_view default_id = "",
              IdentifierMode identifier_mode = IdentifierMode::Additional,
              const Paths &paths = {sourcemeta::core::EMPTY_WEAK_POINTER});

  /// Get a specific reference entry by type and pointer
  [[nodiscard]] auto
  reference(const SchemaReferenceType type,
            const sourcemeta::core::WeakPointer &pointer) const
      -> std::optional<std::reference_wrapper<const Reference>>;

  /// Get a specific location entry by reference type and URI
  [[nodiscard]] auto location(const SchemaReferenceType type,
                              const std::string_view uri) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// The number of locations in the frame
  [[nodiscard]] auto location_count() const noexcept -> std::size_t;

  /// The number of references in the frame
  [[nodiscard]] auto reference_count() const noexcept -> std::size_t;

  /// Check whether the analysed schema makes use of dynamic referencing at all
  [[nodiscard]] auto has_dynamic_references() const noexcept -> bool;

  /// Check whether the analysed schema has no external references
  [[nodiscard]] auto standalone() const noexcept -> bool;

  /// Get the root schema identifier (empty if none)
  [[nodiscard]] auto root() const noexcept
      -> const sourcemeta::core::JSON::String &;

  /// Get the location entry of the schema that was analysed. Unlike
  /// sourcemeta::blaze::SchemaFrame::root, this works for anonymous schemas
  [[nodiscard]] auto root_location() const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Get the vocabularies associated with a location entry. The frame owns
  /// the result, computing it at most once per dialect that it came across.
  /// Note that as with the meta-schemas that framing found embedded in the
  /// document, what the first resolver reported for a given dialect is what
  /// every later call reports, whichever resolver they pass
  /// Get the meta-schema of the analysed schema, preferring one embedded in
  /// the document itself over what the resolver knows about
  [[nodiscard]] auto metaschema(const SchemaResolver &resolver) const
      -> const sourcemeta::core::JSON &;

  [[nodiscard]] auto vocabularies(const Location &location,
                                  const SchemaResolver &resolver) const
      -> const SchemaVocabularies &;

  /// Get the URI associated with a location entry
  [[nodiscard]] auto
  uri(const Location &location,
      const sourcemeta::core::WeakPointer &relative_schema_location =
          sourcemeta::core::EMPTY_WEAK_POINTER) const
      -> sourcemeta::core::JSON::String;

  /// Get the location associated by traversing a pointer from another location
  [[nodiscard]] auto
  traverse(const Location &location,
           const sourcemeta::core::WeakPointer &relative_schema_location) const
      -> const Location &;

  /// Get the location associated with a given URI
  [[nodiscard]] auto traverse(const std::string_view uri) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Get the location associated with a given pointer
  [[nodiscard]] auto
  traverse(const sourcemeta::core::WeakPointer &pointer) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Get the location of a specific type associated with a given pointer
  [[nodiscard]] auto traverse(const sourcemeta::core::WeakPointer &pointer,
                              const LocationType type) const
      -> std::optional<std::reference_wrapper<const Location>>;

  /// Turn an absolute pointer into a location URI
  [[nodiscard]] auto uri(const sourcemeta::core::WeakPointer &pointer) const
      -> std::optional<
          std::reference_wrapper<const sourcemeta::core::JSON::String>>;

  /// Try to dereference a reference location into its destination location
  [[nodiscard]] auto
  dereference(const Location &location,
              const sourcemeta::core::WeakPointer &relative_schema_location =
                  sourcemeta::core::EMPTY_WEAK_POINTER) const
      -> std::pair<SchemaReferenceType,
                   std::optional<std::reference_wrapper<const Location>>>;

  /// Iterate over every schema resource and subschema, skipping the pointer
  /// and anchor entries that do not stand for a schema of their own
  template <std::invocable<const Location &> F>
  auto for_each_subschema(const F &callback) const -> void {
    for (const auto &entry : this->locations_) {
      if (entry.second.type == LocationType::Resource ||
          entry.second.type == LocationType::Subschema) {
        callback(entry.second);
      }
    }
  }

  /// Iterate over every schema resource and subschema strictly below the given
  /// pointer
  template <std::invocable<const Location &> F>
  auto for_each_subschema_under(const sourcemeta::core::WeakPointer &pointer,
                                const F &callback) const -> void {
    for (const auto &entry : this->locations_) {
      if ((entry.second.type == LocationType::Resource ||
           entry.second.type == LocationType::Subschema) &&
          entry.second.pointer.size() > pointer.size() &&
          entry.second.pointer.starts_with(pointer)) {
        callback(entry.second);
      }
    }
  }

  /// Check whether any schema resource or subschema satisfies the predicate
  template <std::predicate<const Location &> F>
  [[nodiscard]] auto any_subschema(const F &predicate) const -> bool {
    for (const auto &entry : this->locations_) {
      if ((entry.second.type == LocationType::Resource ||
           entry.second.type == LocationType::Subschema) &&
          predicate(entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Check whether any schema resource or subschema strictly below the given
  /// pointer satisfies the predicate
  template <std::predicate<const Location &> F>
  [[nodiscard]] auto
  any_subschema_under(const sourcemeta::core::WeakPointer &pointer,
                      const F &predicate) const -> bool {
    for (const auto &entry : this->locations_) {
      if ((entry.second.type == LocationType::Resource ||
           entry.second.type == LocationType::Subschema) &&
          entry.second.pointer.size() > pointer.size() &&
          entry.second.pointer.starts_with(pointer) &&
          predicate(entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over every anchor of the given kind, along with its URI
  template <std::invocable<std::string_view, const Location &> F>
  auto for_each_anchor(const SchemaReferenceType type, const F &callback) const
      -> void {
    for (const auto &entry : this->locations_) {
      if (entry.first.first == type &&
          entry.second.type == LocationType::Anchor) {
        callback(entry.first.second, entry.second);
      }
    }
  }

  /// Check whether any anchor of the given kind satisfies the predicate
  template <std::predicate<std::string_view, const Location &> F>
  [[nodiscard]] auto any_anchor(const SchemaReferenceType type,
                                const F &predicate) const -> bool {
    for (const auto &entry : this->locations_) {
      if (entry.first.first == type &&
          entry.second.type == LocationType::Anchor &&
          predicate(entry.first.second, entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over every schema resource, along with its URI
  template <std::invocable<std::string_view, const Location &> F>
  auto for_each_resource(const F &callback) const -> void {
    for (const auto &entry : this->locations_) {
      if (entry.second.type == LocationType::Resource) {
        callback(entry.first.second, entry.second);
      }
    }
  }

  /// Iterate over every location, whatever kind it is
  template <
      std::invocable<SchemaReferenceType, std::string_view, const Location &> F>
  auto for_each_location(const F &callback) const -> void {
    for (const auto &entry : this->locations_) {
      callback(entry.first.first, entry.first.second, entry.second);
    }
  }

  /// Check whether any location satisfies the predicate
  template <
      std::predicate<SchemaReferenceType, std::string_view, const Location &> F>
  [[nodiscard]] auto any_location(const F &predicate) const -> bool {
    for (const auto &entry : this->locations_) {
      if (predicate(entry.first.first, entry.first.second, entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over every reference, along with the pointer it originates from
  template <
      std::invocable<SchemaReferenceType, const sourcemeta::core::WeakPointer &,
                     const Reference &>
          F>
  auto for_each_reference(const F &callback) const -> void {
    for (const auto &entry : this->references_) {
      callback(entry.first.first, entry.first.second, entry.second);
    }
  }

  /// Check whether any reference satisfies the predicate
  template <
      std::predicate<SchemaReferenceType, const sourcemeta::core::WeakPointer &,
                     const Reference &>
          F>
  [[nodiscard]] auto any_reference(const F &predicate) const -> bool {
    for (const auto &entry : this->references_) {
      if (predicate(entry.first.first, entry.first.second, entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over every reference that originates at or below the given pointer
  template <
      std::invocable<SchemaReferenceType, const sourcemeta::core::WeakPointer &,
                     const Reference &>
          F>
  auto for_each_reference_from(const sourcemeta::core::WeakPointer &pointer,
                               const F &callback) const -> void {
    for (const auto &entry : this->references_) {
      if (entry.first.second.starts_with(pointer)) {
        callback(entry.first.first, entry.first.second, entry.second);
      }
    }
  }

  /// Check whether any reference originating at or below the given pointer
  /// satisfies the predicate
  template <
      std::predicate<SchemaReferenceType, const sourcemeta::core::WeakPointer &,
                     const Reference &>
          F>
  [[nodiscard]] auto
  any_reference_from(const sourcemeta::core::WeakPointer &pointer,
                     const F &predicate) const -> bool {
    for (const auto &entry : this->references_) {
      if (entry.first.second.starts_with(pointer) &&
          predicate(entry.first.first, entry.first.second, entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over every reference whose destination resolves at or below the
  /// given pointer. Note that unlike
  /// sourcemeta::blaze::SchemaFrame::has_references_to, which matches a
  /// destination exactly, this covers the entire subtree
  template <
      std::invocable<SchemaReferenceType, const sourcemeta::core::WeakPointer &,
                     const Reference &>
          F>
  auto for_each_reference_into(const sourcemeta::core::WeakPointer &pointer,
                               const F &callback) const -> void {
    for (const auto &entry : this->references_) {
      const auto destination{this->traverse(entry.second.destination)};
      if (destination.has_value() &&
          destination.value().get().pointer.starts_with(pointer)) {
        callback(entry.first.first, entry.first.second, entry.second);
      }
    }
  }

  /// Check whether any reference whose destination resolves at or below the
  /// given pointer satisfies the predicate
  template <
      std::predicate<SchemaReferenceType, const sourcemeta::core::WeakPointer &,
                     const Reference &>
          F>
  [[nodiscard]] auto
  any_reference_into(const sourcemeta::core::WeakPointer &pointer,
                     const F &predicate) const -> bool {
    for (const auto &entry : this->references_) {
      const auto destination{this->traverse(entry.second.destination)};
      if (destination.has_value() &&
          destination.value().get().pointer.starts_with(pointer) &&
          predicate(entry.first.first, entry.first.second, entry.second)) {
        return true;
      }
    }

    return false;
  }

  /// Iterate over all resource URIs in the frame
  template <std::invocable<std::string_view> F>
  auto for_each_resource_uri(const F &callback) const -> void {
    for (const auto &[key, location] : this->locations_) {
      if (location.type == LocationType::Resource) {
        callback(key.second);
      }
    }
  }

  /// Iterate over all unresolved references (where destination cannot be
  /// traversed)
  template <
      std::invocable<const sourcemeta::core::WeakPointer &, const Reference &>
          F>
  auto for_each_unresolved_reference(const F &callback) const -> void {
    for (const auto &[key, reference] : this->references_) {
      if (!this->traverse(reference.destination).has_value()) {
        callback(key.second, reference);
      }
    }
  }

  /// Check if there are any references to a given location pointer
  [[nodiscard]] auto
  has_references_to(const sourcemeta::core::WeakPointer &pointer) const -> bool;

  /// Check if there are any references that go through a given location pointer
  [[nodiscard]] auto
  has_references_through(const sourcemeta::core::WeakPointer &pointer) const
      -> bool;
  /// Check if there are any references that go through a given location pointer
  /// with a tail token
  [[nodiscard]] auto
  has_references_through(const sourcemeta::core::WeakPointer &pointer,
                         const sourcemeta::core::WeakPointer::Token &tail) const
      -> bool;

  /// Get the relative instance location pointer for a given location entry
  [[nodiscard]] auto relative_instance_location(const Location &location) const
      -> sourcemeta::core::WeakPointer;

  /// Determines if a location could be evaluated during validation
  [[nodiscard]] auto is_reachable(const Location &base,
                                  const Location &location,
                                  const SchemaWalker &walker,
                                  const SchemaResolver &resolver) const -> bool;

private:
  /// A JSON Schema reference map is a mapping of a JSON Pointer
  /// of a subschema to a destination static reference URI.
  /// For convenience, the value consists of the URI on its entirety,
  /// but also broken down by its potential fragment component.
  /// The reference type is part of the key as it is possible to
  /// have a static and a dynamic reference to the same location
  /// on the same schema object.
  using References =
      std::map<std::pair<SchemaReferenceType, sourcemeta::core::WeakPointer>,
               Reference>;

  using Locations =
      // While it might seem weird that we namespace the location URIs with a
      // reference type, it is essential for distinguishing schema resource URIs
      // from `$recursiveRef: true` on another place of the schema schema
      // resource, as otherwise they would both have the exact same URI, but
      // point to different places.
      std::map<std::pair<SchemaReferenceType, sourcemeta::core::JSON::String>,
               Location>;

  Mode mode_;
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  sourcemeta::core::JSON::String root_;
  // A reference may target a place that no schema location covers, in which
  // case framing materialises one for it. Unlike every other location, the
  // tokens of those pointers are not borrowed from the analysed document, so
  // they have to be declared here to outlive the locations that borrow them
  std::deque<sourcemeta::core::Pointer> reference_pointers_;
  Locations locations_;
  References references_;
  // What the frame derives rather than is, kept out of line so that this
  // declaration stays down to the schema it framed
  struct Cache;
  std::unique_ptr<Cache> cache_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
};

} // namespace sourcemeta::blaze

#endif
