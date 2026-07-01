#ifndef SOURCEMETA_CORE_JSONLD_MATERIALIZE_H_
#define SOURCEMETA_CORE_JSONLD_MATERIALIZE_H_

#ifndef SOURCEMETA_CORE_JSONLD_EXPORT
#include <sourcemeta/core/jsonld_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cstdint>       // std::uint8_t
#include <optional>      // std::optional
#include <unordered_map> // std::unordered_map
#include <variant>       // std::variant
#include <vector>        // std::vector

namespace sourcemeta::core {

/// @ingroup jsonld
/// How a position connects to its parent. The position is the object of the
/// edge, asserted in reverse when the flag is set.
struct JSONLDEdge {
  JSON::String predicate{};
  bool reverse{false};
};

/// @ingroup jsonld
/// The base direction of a language-tagged string
enum class JSONLDDirection : std::uint8_t { LTR, RTL };

/// @ingroup jsonld
/// A position that is an object. An absent identifier means a fresh blank node,
/// and the graph flag asserts the node's descendants in the named graph the
/// identifier denotes.
struct JSONLDNode {
  std::optional<JSON::String> id{};
  std::vector<JSON::String> types{};
  bool graph{false};
};

/// @ingroup jsonld
/// A position that is a value. The datatype defaults to the native type of the
/// JSON value, a language may carry a direction, and the JSON flag preserves an
/// opaque JSON literal verbatim.
struct JSONLDLiteral {
  std::optional<JSON::String> datatype{};
  std::optional<JSON::String> language{};
  std::optional<JSONLDDirection> direction{};
  bool json{false};
};

/// @ingroup jsonld
/// A scalar promoted to an identified node
struct JSONLDReference {
  JSON::String id{};
  std::vector<JSON::String> types{};
};

/// @ingroup jsonld
/// A position that is an array, asserted as an RDF list when ordered and as a
/// set otherwise
struct JSONLDCollection {
  bool ordered{false};
};

/// @ingroup jsonld
/// The semantics of a single instance position: how it connects to its parent
/// and what it is
struct JSONLDDescriptor {
  std::vector<JSONLDEdge> edges{};
  std::variant<JSONLDNode, JSONLDLiteral, JSONLDReference, JSONLDCollection>
      value{};
};

/// @ingroup jsonld
/// A resolved mapping of instance positions to their JSON-LD semantics, keyed
/// by a JSON Pointer of the given kind
template <typename PointerT>
using JSONLDBasicAnnotationMap =
    std::unordered_map<PointerT, JSONLDDescriptor, typename PointerT::Hasher>;

/// @ingroup jsonld
/// A resolved annotation map keyed by an owning JSON Pointer
using JSONLDAnnotationMap = JSONLDBasicAnnotationMap<Pointer>;

/// @ingroup jsonld
/// A resolved annotation map keyed by a non-owning weak JSON Pointer. The keys
/// reference strings owned elsewhere that must outlive any materialization
/// call.
using JSONLDWeakAnnotationMap = JSONLDBasicAnnotationMap<WeakPointer>;

/// @ingroup jsonld
///
/// Materialize an instance into expanded JSON-LD using an annotation map that
/// assigns JSON-LD semantics to instance positions. The result is always a JSON
/// array. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonld.h>
/// #include <iostream>
///
/// const auto instance{sourcemeta::core::parse_json(
///     R"({ "name": "Sourcemeta" })")};
///
/// sourcemeta::core::JSONLDAnnotationMap map;
/// map.emplace(sourcemeta::core::Pointer{},
///             sourcemeta::core::JSONLDDescriptor{
///                 {}, sourcemeta::core::JSONLDNode{
///                         "https://example.com/org", {}, false }});
/// map.emplace(sourcemeta::core::Pointer{"name"},
///             sourcemeta::core::JSONLDDescriptor{
///                 { { "https://schema.org/name", false } },
///                 sourcemeta::core::JSONLDLiteral{}});
///
/// const auto expanded{sourcemeta::core::jsonld_materialize(instance, map)};
/// sourcemeta::core::prettify(expanded, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_materialize(const JSON &instance, const JSONLDAnnotationMap &map)
    -> JSON;

/// @ingroup jsonld
///
/// Materialize an instance into expanded JSON-LD using a weak annotation map
/// whose keys are non-owning views into strings owned elsewhere. The backing
/// strings must outlive the call. The result is always a JSON array. For
/// example:
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
/// sourcemeta::core::JSONLDWeakAnnotationMap map;
/// map.emplace(sourcemeta::core::WeakPointer{},
///             sourcemeta::core::JSONLDDescriptor{
///                 {}, sourcemeta::core::JSONLDNode{
///                         "https://example.com/org", {}, false }});
/// map.emplace(sourcemeta::core::WeakPointer{std::cref(name_key)},
///             sourcemeta::core::JSONLDDescriptor{
///                 { { "https://schema.org/name", false } },
///                 sourcemeta::core::JSONLDLiteral{}});
///
/// const auto expanded{sourcemeta::core::jsonld_materialize(instance, map)};
/// sourcemeta::core::prettify(expanded, std::cout);
/// std::cout << std::endl;
/// ```
SOURCEMETA_CORE_JSONLD_EXPORT
auto jsonld_materialize(const JSON &instance,
                        const JSONLDWeakAnnotationMap &map) -> JSON;

} // namespace sourcemeta::core

#endif
