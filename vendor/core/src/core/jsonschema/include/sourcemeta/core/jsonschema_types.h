#ifndef SOURCEMETA_CORE_JSONSCHEMA_TYPES_H_
#define SOURCEMETA_CORE_JSONSCHEMA_TYPES_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema_vocabularies.h>
#include <sourcemeta/core/memory.h>

#include <cstdint>     // std::uint8_t
#include <format>      // std::formatter, std::format_to
#include <functional>  // std::function, std::reference_wrapper
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <span>        // std::span
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jsonschema
/// What a sourcemeta::core::SchemaResolver hands back: either a schema it
/// owns, or a reference to one that outlives the call
using SchemaResolverResult =
    sourcemeta::core::OwnedOrReference<sourcemeta::core::JSON>;

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
using SchemaResolver = std::function<SchemaResolverResult(std::string_view)>;

/// @ingroup jsonschema
/// The reference type
enum class SchemaReferenceType : std::uint8_t {
  /// A reference that resolves at framing time
  Static,
  /// A reference that resolves at evaluation time
  Dynamic
};

/// @ingroup jsonschema
/// All the known JSON Schema base dialects
enum class SchemaBaseDialect : std::uint8_t {
  /// The 2020-12 validation base dialect
  JSON_SCHEMA_2020_12,
  /// The 2020-12 hyper-schema base dialect
  JSON_SCHEMA_2020_12_HYPER,
  /// The 2019-09 validation base dialect
  JSON_SCHEMA_2019_09,
  /// The 2019-09 hyper-schema base dialect
  JSON_SCHEMA_2019_09_HYPER,
  /// The Draft 7 validation base dialect
  JSON_SCHEMA_DRAFT_7,
  /// The Draft 7 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_7_HYPER,
  /// The Draft 6 validation base dialect
  JSON_SCHEMA_DRAFT_6,
  /// The Draft 6 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_6_HYPER,
  /// The Draft 4 validation base dialect
  JSON_SCHEMA_DRAFT_4,
  /// The Draft 4 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_4_HYPER,
  /// The Draft 3 validation base dialect
  JSON_SCHEMA_DRAFT_3,
  /// The Draft 3 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_3_HYPER,
  /// The Draft 2 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_2_HYPER,
  /// The Draft 1 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_1_HYPER,
  /// The Draft 0 hyper-schema base dialect
  JSON_SCHEMA_DRAFT_0_HYPER
};

/// @ingroup jsonschema
/// Write a base dialect to a stream as its URI
SOURCEMETA_CORE_JSONSCHEMA_EXPORT
auto operator<<(std::ostream &stream, const SchemaBaseDialect base_dialect)
    -> std::ostream &;

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
  std::optional<SchemaVocabularies::URIView> vocabulary;
  /// The keywords a given keyword depends on (if any) during the evaluation
  /// process
  std::span<const std::string_view> dependencies;
  /// The keywords a given keyword depends on for evaluation ordering purposes
  /// only (not semantic dependencies)
  std::span<const std::string_view> order_dependencies;
  /// The JSON instance types that this keyword applies to (empty means all)
  sourcemeta::core::JSON::TypeSet instances;

  // Prevent accidental copies, as walker results are always returned by
  // reference
  SchemaWalkerResult(const SchemaWalkerResult &) = delete;
  auto operator=(const SchemaWalkerResult &) -> SchemaWalkerResult & = delete;
  /// Move a walker result
  SchemaWalkerResult(SchemaWalkerResult &&) = default;
  auto operator=(SchemaWalkerResult &&) -> SchemaWalkerResult & = default;
  ~SchemaWalkerResult() = default;

  /// Describe a keyword from its type, vocabulary, dependencies, and the
  /// instance types it applies to
  constexpr SchemaWalkerResult(
      SchemaKeywordType keyword_type,
      std::optional<SchemaVocabularies::URIView> keyword_vocabulary,
      std::span<const std::string_view> keyword_dependencies,
      std::span<const std::string_view> keyword_order_dependencies,
      sourcemeta::core::JSON::TypeSet keyword_instances)
      : type{keyword_type}, vocabulary{keyword_vocabulary},
        dependencies{keyword_dependencies},
        order_dependencies{keyword_order_dependencies},
        instances{keyword_instances} {}
};

/// @ingroup jsonschema
///
/// For walking purposes, some functions need to understand which JSON Schema
/// keywords declare other JSON Schema definitions. To accomplish this in a
/// generic and flexible way that does not assume the use any vocabulary other
/// than `core`, these functions take a walker function as argument.
using SchemaWalker = std::function<const SchemaWalkerResult &(
    std::string_view, const SchemaVocabularies &)>;

} // namespace sourcemeta::core

/// @cond
template <> struct std::formatter<sourcemeta::core::SchemaBaseDialect> {
  constexpr auto parse(std::format_parse_context &context)
      -> decltype(context.begin()) {
    return context.begin();
  }

  auto format(const sourcemeta::core::SchemaBaseDialect value,
              std::format_context &context) const -> decltype(context.out()) {
    std::ostringstream stream;
    stream << value;
    return std::format_to(context.out(), "{}", stream.str());
  }
};
/// @endcond

#endif
