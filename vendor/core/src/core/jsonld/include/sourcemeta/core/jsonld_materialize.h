#ifndef SOURCEMETA_CORE_JSONLD_MATERIALIZE_H_
#define SOURCEMETA_CORE_JSONLD_MATERIALIZE_H_

#ifndef SOURCEMETA_CORE_JSONLD_EXPORT
#include <sourcemeta/core/jsonld_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>  // std::uint8_t
#include <optional> // std::optional
#include <variant>  // std::variant
#include <vector>   // std::vector

namespace sourcemeta::core {

/// @ingroup jsonld
/// How a position connects to its parent. The position is the object of the
/// edge, asserted in reverse when the flag is set.
struct JSONLDEdge {
  /// The predicate that connects the position to its parent.
  JSON::String predicate{};
  /// Whether the edge is asserted in reverse.
  bool reverse{false};
};

/// @ingroup jsonld
/// The base direction of a language-tagged string
enum class JSONLDDirection : std::uint8_t {
  /// The left-to-right base direction
  LTR,
  /// The right-to-left base direction
  RTL
};

/// @ingroup jsonld
/// A position that is an object. An absent identifier means a fresh blank node,
/// and the graph flag asserts the node's descendants in the named graph the
/// identifier denotes.
struct JSONLDNode {
  /// The node identifier, absent for a fresh blank node.
  std::optional<JSON::String> id{};
  /// The types of the node.
  std::vector<JSON::String> types{};
  /// Whether the node's descendants are asserted in the named graph it denotes.
  bool graph{false};
  /// Constant properties merged into the node, as a canonical fragment of
  /// predicate to term array whose entries are all non-empty term arrays,
  /// never null removal entries. May be empty.
  JSON constants{JSON::make_object()};
};

/// @ingroup jsonld
/// A position that is a value. The datatype defaults to the native type of the
/// JSON value, a language may carry a direction, and the JSON flag preserves an
/// opaque JSON literal verbatim.
struct JSONLDLiteral {
  /// The literal datatype, defaulting to the native type of the value. An
  /// explicit datatype carries a native number or boolean as its canonical
  /// string lexical form.
  std::optional<JSON::String> datatype{};
  /// The language tag of the literal.
  std::optional<JSON::String> language{};
  /// The base direction of the literal.
  std::optional<JSONLDDirection> direction{};
  /// Whether the literal is preserved as an opaque JSON literal.
  bool json{false};
};

/// @ingroup jsonld
/// A scalar promoted to an identified node
struct JSONLDReference {
  /// The identifier of the promoted node.
  JSON::String id{};
  /// The types of the promoted node.
  std::vector<JSON::String> types{};
  /// Constant properties merged into the node, as a canonical fragment of
  /// predicate to term array whose entries are all non-empty term arrays,
  /// never null removal entries. May be empty.
  JSON constants{JSON::make_object()};
};

/// @ingroup jsonld
/// A scalar promoted to a node that carries the scalar as a literal under a
/// value predicate, plus constant properties. A null value at a promoted
/// position materializes nothing, as constant properties never appear
/// without the scalar that legitimizes them.
struct JSONLDPromotion {
  /// The node identifier, absent for a fresh blank node.
  std::optional<JSON::String> id{};
  /// The types of the promoted node.
  std::vector<JSON::String> types{};
  /// The predicate that carries the scalar.
  JSON::String value{};
  /// The literal facets of the carried scalar. The opaque JSON literal facet
  /// does not apply to a promoted scalar and must stay unset.
  JSONLDLiteral literal{};
  /// Constant properties merged into the node, as a canonical fragment of
  /// predicate to term array whose entries are all non-empty term arrays,
  /// never null removal entries. May be empty.
  JSON constants{JSON::make_object()};
};

/// @ingroup jsonld
/// The container form of a collection position. List and Set range over an
/// array; Language and Index range over an object.
enum class JSONLDContainer : std::uint8_t {
  /// An ordered RDF list ranging over an array
  List,
  /// An unordered set ranging over an array
  Set,
  /// A map of language tag to string ranging over an object
  Language,
  /// A map of index labels carrying no RDF ranging over an object
  Index
};

/// @ingroup jsonld
/// A position that is a collection. A List is asserted as an RDF list and a Set
/// as a set, both ranging over an array. A Language container ranges over an
/// object of language tag to string, and an Index container over an object
/// whose keys are index labels that carry no RDF.
struct JSONLDCollection {
  /// The container form of the collection.
  JSONLDContainer container{JSONLDContainer::Set};
};

/// @ingroup jsonld
/// The semantics of a single instance position: how it connects to its parent
/// and what it is
struct JSONLDDescriptor {
  /// How the position connects to its parent.
  std::vector<JSONLDEdge> edges{};
  /// What the position is.
  std::variant<JSONLDNode, JSONLDLiteral, JSONLDReference, JSONLDCollection,
               JSONLDPromotion>
      value{};
};

/// @ingroup jsonld
/// An instance position paired with its JSON-LD semantics
template <typename PointerT> struct JSONLDBasicAnnotation {
  /// The instance position the annotation describes.
  PointerT pointer{};
  /// The JSON-LD semantics of the position.
  JSONLDDescriptor descriptor{};
};

/// @ingroup jsonld
/// A flat collection of annotated instance positions, in any order. When more
/// than one entry describes the same position, the first one wins.
template <typename PointerT>
using JSONLDBasicAnnotationList = std::vector<JSONLDBasicAnnotation<PointerT>>;

/// @ingroup jsonld
/// An annotation list whose positions are owning JSON Pointers
using JSONLDAnnotationList = JSONLDBasicAnnotationList<Pointer>;

/// @ingroup jsonld
/// An annotation list whose positions are non-owning weak JSON Pointers. The
/// positions reference strings owned elsewhere that must outlive any
/// materialization call.
using JSONLDWeakAnnotationList = JSONLDBasicAnnotationList<WeakPointer>;

/// @ingroup jsonld
///
/// Materialize an instance into expanded JSON-LD using an annotation list that
/// assigns JSON-LD semantics to instance positions. An undescribed member of a
/// collection defaults to a plain literal, or to a collection of the enclosing
/// kind for a nested array. The result is always a JSON array. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto instance{sourcemeta::core::parse_json(
///     R"({ "name": "Sourcemeta" })")};
///
/// sourcemeta::core::JSONLDAnnotationList annotations;
/// annotations.push_back(
///     {sourcemeta::core::Pointer{},
///      sourcemeta::core::JSONLDDescriptor{
///          {}, sourcemeta::core::JSONLDNode{
///                  "https://example.com/org", {}, false }}});
/// annotations.push_back(
///     {sourcemeta::core::Pointer{"name"},
///      sourcemeta::core::JSONLDDescriptor{
///          { { "https://schema.org/name", false } },
///          sourcemeta::core::JSONLDLiteral{}}});
///
/// const auto expanded{
///     sourcemeta::core::jsonld_materialize(instance, annotations)};
/// sourcemeta::core::prettify(expanded, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_materialize(const JSON &instance,
                        const JSONLDAnnotationList &annotations) -> JSON;

/// @ingroup jsonld
///
/// Materialize an instance into expanded JSON-LD using a weak annotation list
/// whose positions are non-owning views into strings owned elsewhere. The
/// backing strings must outlive the call. An undescribed member of a
/// collection defaults to a plain literal, or to a collection of the enclosing
/// kind for a nested array. The result is always a JSON array. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <functional>
/// #include <iostream>
///
/// const auto instance{sourcemeta::core::parse_json(
///     R"({ "name": "Sourcemeta" })")};
///
/// const sourcemeta::core::JSON::String name_key{"name"};
///
/// sourcemeta::core::JSONLDWeakAnnotationList annotations;
/// annotations.push_back(
///     {sourcemeta::core::WeakPointer{},
///      sourcemeta::core::JSONLDDescriptor{
///          {}, sourcemeta::core::JSONLDNode{
///                  "https://example.com/org", {}, false }}});
/// annotations.push_back(
///     {sourcemeta::core::WeakPointer{std::cref(name_key)},
///      sourcemeta::core::JSONLDDescriptor{
///          { { "https://schema.org/name", false } },
///          sourcemeta::core::JSONLDLiteral{}}});
///
/// const auto expanded{
///     sourcemeta::core::jsonld_materialize(instance, annotations)};
/// sourcemeta::core::prettify(expanded, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_materialize(const JSON &instance,
                        const JSONLDWeakAnnotationList &annotations) -> JSON;

/// @ingroup jsonld
///
/// Validate a fragment of constant node properties written in a restricted
/// expanded form and return its canonical form: keys sorted, bare scalars
/// and single terms wrapped into term arrays, duplicate terms removed, and
/// native numbers or booleans paired with an explicit datatype rewritten to
/// their canonical string lexical form. A grammar violation throws. A null
/// entry passes through untouched as the caller's removal channel, so a
/// canonical fragment only becomes usable as descriptor constants once the
/// caller resolves removals and strips the null entries. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonld.h>
///
/// #include <cassert>
///
/// const auto fragment{sourcemeta::core::parse_json(R"({
///   "https://example.com/unit": { "@id": "https://example.com/metre" }
/// })")};
///
/// const auto canonical{
///     sourcemeta::core::jsonld_canonicalize_fragment(fragment)};
/// assert(canonical.at("https://example.com/unit").is_array());
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_canonicalize_fragment(const JSON &fragment) -> JSON;

} // namespace sourcemeta::core

#endif
