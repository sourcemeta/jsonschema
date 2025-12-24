#ifndef SOURCEMETA_CORE_JSONSCHEMA_TYPES_H_
#define SOURCEMETA_CORE_JSONSCHEMA_TYPES_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema_vocabularies.h>

#include <cstdint>       // std::uint8_t
#include <functional>    // std::function, std::reference_wrapper
#include <optional>      // std::optional
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_set> // std::unordered_set

namespace sourcemeta::core {

// Take a URI and get back a schema
/// @ingroup jsonschema
///
/// Some functions need to reference other schemas by their URIs. To accomplish
/// this in a generic and flexible way, these functions take resolver functions
/// as arguments, of the type sourcemeta::core::SchemaResolver.
///
/// For convenience, we provide the following default resolvers:
///
/// - sourcemeta::core::schema_resolver
///
/// You can implement resolvers to read from a local storage, to send HTTP
/// requests, or anything your application might require. Unless your resolver
/// is trivial, it is recommended to create a callable object that implements
/// the function interface.
using SchemaResolver = std::function<std::optional<JSON>(std::string_view)>;

/// @ingroup jsonschema
/// The reference type
enum class SchemaReferenceType : std::uint8_t { Static, Dynamic };

#if defined(__GNUC__)
#pragma GCC diagnostic push
// For some strange reason, GCC on Debian 11 believes that a member of
// an enum class (which is namespaced by definition), can shadow an
// alias defined even on a different namespace.
#pragma GCC diagnostic ignored "-Wshadow"
#endif
/// @ingroup jsonschema
/// Determines the type of a JSON Schema keyword
enum class SchemaKeywordType : std::uint8_t {
  /// The JSON Schema keyword is unknown
  Unknown,
  /// The JSON Schema keyword is a non-applicator assertion
  Assertion,
  /// The JSON Schema keyword is a non-applicator annotation
  Annotation,
  /// The JSON Schema keyword is a reference
  Reference,
  /// The JSON Schema keyword is known but doesn't match any other type
  Other,
  /// The JSON Schema keyword is considered to be a comment without any
  /// additional meaning
  Comment,
  /// The JSON Schema keyword is a reserved location that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions
  LocationMembers,

  /// The JSON Schema keyword is an applicator that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions.
  /// The instance traverses based on the members as property names
  ApplicatorMembersTraversePropertyStatic,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions.
  /// The instance traverses based on the members as property regular
  /// expressions
  ApplicatorMembersTraversePropertyRegex,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument
  /// The instance traverses to some of the properties
  ApplicatorValueTraverseSomeProperty,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument
  /// The instance traverses to any property key
  ApplicatorValueTraverseAnyPropertyKey,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument
  /// The instance traverses to any item
  ApplicatorValueTraverseAnyItem,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument
  /// The instance traverses to some of the items
  ApplicatorValueTraverseSomeItem,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument
  /// The instance traverses back to the parent
  ApplicatorValueTraverseParent,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument
  /// The instance traverses based on the element indexes
  ApplicatorElementsTraverseItem,
  /// The JSON Schema keyword is an applicator that may take a JSON Schema
  /// definition or an array of potentially JSON Schema definitions
  /// as an argument
  /// The instance traverses to any item or based on the element indexes
  ApplicatorValueOrElementsTraverseAnyItemOrItem,
  /// The JSON Schema keyword is an applicator that may take a JSON Schema
  /// definition or an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location.
  /// The instance does not traverse
  ApplicatorValueOrElementsInPlace,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an object as argument, whose values are potentially
  /// JSON Schema definitions without affecting the instance location.
  /// The instance does not traverse
  ApplicatorMembersInPlaceSome,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location.
  /// The instance does not traverse
  ApplicatorElementsInPlace,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location
  /// The instance does not traverse, and only some of the
  /// elements apply.
  ApplicatorElementsInPlaceSome,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes an array of potentially JSON Schema definitions
  /// as an argument without affecting the instance location
  /// The instance does not traverse, and only some of the
  /// elements apply in negated form.
  ApplicatorElementsInPlaceSomeNegate,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument without affecting the
  /// instance location.
  /// The instance does not traverse, and only applies some of the times.
  ApplicatorValueInPlaceMaybe,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument but its evaluation follows
  /// special rules.
  /// The instance does not traverse
  ApplicatorValueInPlaceOther,
  /// The JSON Schema keyword is an applicator that potentially
  /// takes a JSON Schema definition as an argument but the instance is expected
  /// to not validate against it.
  /// The instance does not traverse
  ApplicatorValueInPlaceNegate,
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

/// @ingroup jsonschema
/// A structure that encapsulates the result of walker over a specific keyword
struct SchemaWalkerResult {
  /// The walker strategy to continue traversing across the schema
  SchemaKeywordType type;
  /// The vocabulary associated with the keyword, if any
  std::optional<Vocabularies::URI> vocabulary;
  /// The keywords a given keyword depends on (if any) during the evaluation
  /// process
  std::unordered_set<std::string_view> dependencies;
  /// The keywords a given keyword depends on for evaluation ordering purposes
  /// only (not semantic dependencies)
  std::unordered_set<std::string_view> order_dependencies;
  /// The JSON instance types that this keyword applies to (empty means all)
  JSON::TypeSet instances;

  // Prevent accidental copies, as walker results are always returned by
  // reference
  SchemaWalkerResult(const SchemaWalkerResult &) = delete;
  auto operator=(const SchemaWalkerResult &) -> SchemaWalkerResult & = delete;
  SchemaWalkerResult(SchemaWalkerResult &&) = default;
  auto operator=(SchemaWalkerResult &&) -> SchemaWalkerResult & = default;
  ~SchemaWalkerResult() = default;

  SchemaWalkerResult(SchemaKeywordType type_,
                     std::optional<Vocabularies::URI> vocabulary_,
                     std::unordered_set<std::string_view> dependencies_,
                     std::unordered_set<std::string_view> order_dependencies_,
                     JSON::TypeSet instances_)
      : type{type_}, vocabulary{std::move(vocabulary_)},
        dependencies{std::move(dependencies_)},
        order_dependencies{std::move(order_dependencies_)},
        instances{instances_} {}
};

/// @ingroup jsonschema
///
/// For walking purposes, some functions need to understand which JSON Schema
/// keywords declare other JSON Schema definitions. To accomplish this in a
/// generic and flexible way that does not assume the use any vocabulary other
/// than `core`, these functions take a walker function as argument, of the type
/// sourcemeta::core::SchemaWalker.
///
/// For convenience, we provide the following default walkers:
///
/// - sourcemeta::core::schema_walker
/// - sourcemeta::core::schema_walker_none
using SchemaWalker = std::function<const SchemaWalkerResult &(
    std::string_view, const Vocabularies &)>;

/// @ingroup jsonschema
/// An entry of a schema iterator.
struct SchemaIteratorEntry {
  // TODO: Turn this into a weak pointer
  std::optional<Pointer> parent;
  // TODO: Turn this into a weak pointer
  Pointer pointer;
  // TODO: Use "known" enum classes + strings for dialects
  std::optional<JSON::String> dialect;
  Vocabularies vocabularies;
  // TODO: Use "known" enum classes for base dialects
  std::optional<JSON::String> base_dialect;
  std::reference_wrapper<const JSON> subschema;

  // TODO: These two pointer templates contain some overlap.
  // Instead, have a `base_instance_location` and a `relative_instance_location`
  // that when concatenated, represent the full `instance_location`
  // TODO: Make these WeakPointerTemplate
  PointerTemplate instance_location;
  PointerTemplate relative_instance_location;

  bool orphan;
};

} // namespace sourcemeta::core

#endif
