#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include "jsonld_keywords.h"

#include <algorithm> // std::ranges::sort, std::ranges::stable_sort, std::ranges::unique, std::ranges::none_of
#include <cassert>   // assert
#include <cstddef>   // std::size_t
#include <functional>  // std::reference_wrapper, std::cref
#include <optional>    // std::optional, std::nullopt
#include <type_traits> // std::is_same_v
#include <utility>     // std::move, std::unreachable
#include <variant>     // std::holds_alternative, std::get
#include <vector>      // std::vector

namespace sourcemeta::core {

namespace {

template <typename PointerT>
using AnnotationIndex = std::vector<const JSONLDBasicAnnotation<PointerT> *>;

// A contiguous run of sorted annotations whose positions are all extensions
// of, or equal to, the position currently being visited
template <typename PointerT> struct AnnotationRange {
  typename AnnotationIndex<PointerT>::const_iterator begin;
  typename AnnotationIndex<PointerT>::const_iterator end;
};

// The sub-run of annotations that belong to the child position the pointer
// currently names, advancing the scan iterator past it. Annotations sorting
// before the child correspond to positions the walk does not visit and are
// skipped for good.
template <typename PointerT>
auto child_range(typename AnnotationIndex<PointerT>::const_iterator &iterator,
                 const typename AnnotationIndex<PointerT>::const_iterator end,
                 const PointerT &pointer) -> AnnotationRange<PointerT> {
  const auto depth{pointer.size() - 1};
  const auto &token{pointer.at(depth)};
  while (iterator != end && (*iterator)->pointer.at(depth) < token) {
    iterator += 1;
  }
  const auto begin{iterator};
  while (iterator != end && (*iterator)->pointer.at(depth) == token) {
    iterator += 1;
  }
  return {begin, iterator};
}

template <typename PointerT>
auto materialize_value(const JSON &value, PointerT &pointer,
                       AnnotationRange<PointerT> range,
                       std::vector<JSON> &standalone,
                       const std::vector<JSONLDEdge> **matched_edges = nullptr)
    -> std::optional<JSON>;

template <typename PointerT>
auto fill_node(JSON &node, const JSON &instance_object, PointerT &pointer,
               const AnnotationRange<PointerT> &range,
               std::vector<JSON> &standalone) -> void;

template <typename PointerT>
auto materialize_member(const JSON &value, PointerT &pointer,
                        const AnnotationRange<PointerT> &range,
                        std::vector<JSON> &standalone) -> std::optional<JSON>;

// Append an object key to the pointer, copying it for an owning pointer and
// taking a non-owning view for a weak pointer.
template <typename PointerT>
auto push_property(PointerT &pointer, const JSON::String &key) -> void {
  if constexpr (std::is_same_v<typename PointerT::Token::Property,
                               JSON::String>) {
    pointer.push_back(key);
  } else {
    pointer.push_back(std::cref(key));
  }
}

// A node object is any map that is neither a value object nor a list object.
auto is_node_object(const JSON &value) -> bool {
  return value.is_object() &&
         !value.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH) &&
         !value.defines(KEYWORD_LIST, KEYWORD_LIST_HASH);
}

auto direction_to_string(const JSONLDDirection direction) -> JSON::StringView {
  switch (direction) {
    case JSONLDDirection::LTR:
      return "ltr";
    case JSONLDDirection::RTL:
      return "rtl";
  }

  std::unreachable();
}

auto types_to_array(const std::vector<JSON::String> &types) -> JSON {
  auto result{JSON::make_array()};
  for (const auto &type : types) {
    result.push_back(JSON{type});
  }
  return result;
}

// The property array of node under the given predicate, creating it as needed.
auto property_target(JSON &node, const JSON::StringView predicate) -> JSON & {
  const auto hash{node.as_object().hash(predicate)};
  const auto existing{node.try_at(predicate, hash)};
  if (existing != nullptr) {
    return *existing;
  }
  return node.assign_assume_new(JSON::String{predicate}, JSON::make_array(),
                                hash);
}

// The property array nested under @reverse and the given predicate.
auto reverse_target(JSON &node, const JSON::StringView predicate) -> JSON & {
  const auto existing{node.try_at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)};
  auto &reverse{existing != nullptr
                    ? *existing
                    : node.assign_assume_new(JSON::String{KEYWORD_REVERSE},
                                             JSON::make_object(),
                                             KEYWORD_REVERSE_HASH)};
  return property_target(reverse, predicate);
}

// Attach a value under a single edge. A set, represented as a bare array,
// spreads its members into the property array.
auto attach_one(JSON &node, const JSONLDEdge &edge, JSON value) -> void {
  // An empty set carries no values, so it asserts no property.
  if (value.is_array() && value.empty()) {
    return;
  }

  auto &target{edge.reverse ? reverse_target(node, edge.predicate)
                            : property_target(node, edge.predicate)};
  if (value.is_array()) {
    for (auto &element : value.as_array()) {
      target.push_back(std::move(element));
    }
  } else {
    target.push_back(std::move(value));
  }
}

auto attach(JSON &node, const std::vector<JSONLDEdge> &edges, JSON value)
    -> void {
  for (std::size_t index = 0; index + 1 < edges.size(); index += 1) {
    attach_one(node, edges[index], value);
  }
  attach_one(node, edges.back(), std::move(value));
}

auto materialize_literal(const JSONLDLiteral &descriptor, const JSON &value)
    -> JSON {
  auto result{JSON::make_object()};
  result.assign_assume_new(JSON::String{KEYWORD_VALUE}, JSON{value},
                           KEYWORD_VALUE_HASH);
  if (descriptor.json) {
    result.assign_assume_new(JSON::String{KEYWORD_TYPE}, JSON{KEYWORD_JSON},
                             KEYWORD_TYPE_HASH);
    return result;
  }

  if (descriptor.datatype.has_value()) {
    result.assign_assume_new(JSON::String{KEYWORD_TYPE},
                             JSON{descriptor.datatype.value()},
                             KEYWORD_TYPE_HASH);
  }
  if (descriptor.language.has_value()) {
    result.assign_assume_new(JSON::String{KEYWORD_LANGUAGE},
                             JSON{descriptor.language.value()},
                             KEYWORD_LANGUAGE_HASH);
  }
  if (descriptor.direction.has_value()) {
    result.assign_assume_new(
        JSON::String{KEYWORD_DIRECTION},
        JSON{direction_to_string(descriptor.direction.value())},
        KEYWORD_DIRECTION_HASH);
  }
  return result;
}

auto materialize_reference(const JSONLDReference &descriptor) -> JSON {
  auto result{JSON::make_object()};
  result.assign_assume_new(JSON::String{KEYWORD_ID}, JSON{descriptor.id},
                           KEYWORD_ID_HASH);
  if (!descriptor.types.empty()) {
    result.assign_assume_new(JSON::String{KEYWORD_TYPE},
                             types_to_array(descriptor.types),
                             KEYWORD_TYPE_HASH);
  }
  return result;
}

template <typename PointerT>
auto build_collection(const JSON &value, PointerT &pointer,
                      const AnnotationRange<PointerT> &range,
                      std::vector<JSON> &standalone, const bool ordered)
    -> JSON {
  auto elements{JSON::make_array()};
  auto iterator{range.begin};
  for (std::size_t index = 0; index < value.size(); index += 1) {
    pointer.push_back(index);
    auto element{materialize_member(value.at(index), pointer,
                                    child_range(iterator, range.end, pointer),
                                    standalone)};
    pointer.pop_back();
    if (!element.has_value()) {
      continue;
    }

    // A nested set flattens into the enclosing collection.
    if (element->is_array()) {
      for (auto &nested : element->as_array()) {
        elements.push_back(std::move(nested));
      }
    } else {
      elements.push_back(std::move(element.value()));
    }
  }

  if (!ordered) {
    return elements;
  }

  auto result{JSON::make_object()};
  result.assign_assume_new(JSON::String{KEYWORD_LIST}, std::move(elements),
                           KEYWORD_LIST_HASH);
  return result;
}

// The keys of an object in sorted order, so the object walk is canonical
// regardless of the instance key order.
auto sorted_keys(const JSON &value)
    -> std::vector<std::reference_wrapper<const JSON::String>> {
  std::vector<std::reference_wrapper<const JSON::String>> keys;
  keys.reserve(value.object_size());
  for (const auto &entry : value.as_object()) {
    keys.push_back(std::cref(entry.first));
  }
  std::ranges::sort(keys, [](const auto &left, const auto &right) -> bool {
    return left.get() < right.get();
  });
  return keys;
}

// The reserved @none key carries no language.
auto language_literal(const JSON &value, const JSON::String &language,
                      const bool none) -> JSON {
  auto result{JSON::make_object()};
  result.assign_assume_new(JSON::String{KEYWORD_VALUE}, JSON{value},
                           KEYWORD_VALUE_HASH);
  if (!none) {
    result.assign_assume_new(JSON::String{KEYWORD_LANGUAGE}, JSON{language},
                             KEYWORD_LANGUAGE_HASH);
  }
  return result;
}

auto build_language_collection(const JSON &value) -> JSON {
  auto elements{JSON::make_array()};
  for (const auto key : sorted_keys(value)) {
    const auto &member{value.at(key.get())};
    const bool none{key.get() == KEYWORD_NONE};
    // A null value or array item in a language map is treated as absent
    if (member.is_array()) {
      for (const auto &element : member.as_array()) {
        if (element.is_null()) {
          continue;
        }

        assert(element.is_string());
        elements.push_back(language_literal(element, key.get(), none));
      }
    } else if (!member.is_null()) {
      assert(member.is_string());
      elements.push_back(language_literal(member, key.get(), none));
    }
  }
  return elements;
}

// The index keys carry no RDF and are dropped.
template <typename PointerT>
auto build_index_collection(const JSON &value, PointerT &pointer,
                            const AnnotationRange<PointerT> &range,
                            std::vector<JSON> &standalone) -> JSON {
  auto elements{JSON::make_array()};
  auto iterator{range.begin};
  for (const auto key : sorted_keys(value)) {
    push_property(pointer, key.get());
    auto element{materialize_member(value.at(key.get()), pointer,
                                    child_range(iterator, range.end, pointer),
                                    standalone)};
    pointer.pop_back();
    if (!element.has_value()) {
      continue;
    }

    // A nested set flattens into the enclosing collection.
    if (element->is_array()) {
      for (auto &nested : element->as_array()) {
        elements.push_back(std::move(nested));
      }
    } else {
      elements.push_back(std::move(element.value()));
    }
  }
  return elements;
}

// An undescribed collection member still materializes with a default kind, a
// scalar as a plain literal and a nested array as an unordered collection.
// An undescribed object member keeps the anonymous node treatment of any
// other position.
template <typename PointerT>
auto materialize_member(const JSON &value, PointerT &pointer,
                        const AnnotationRange<PointerT> &range,
                        std::vector<JSON> &standalone) -> std::optional<JSON> {
  const auto described{range.begin != range.end &&
                       (*range.begin)->pointer.size() == pointer.size()};
  if (described || value.is_object()) {
    return materialize_value(value, pointer, range, standalone);
  }

  if (value.is_null()) {
    return std::nullopt;
  }

  if (value.is_array()) {
    return build_collection(value, pointer, range, standalone, false);
  }

  return materialize_literal(JSONLDLiteral{}, value);
}

template <typename PointerT>
auto materialize_node(const JSONLDNode &descriptor, const JSON &value,
                      PointerT &pointer, const AnnotationRange<PointerT> &range,
                      std::vector<JSON> &standalone) -> JSON {
  auto node{JSON::make_object()};
  if (descriptor.id.has_value()) {
    node.assign_assume_new(JSON::String{KEYWORD_ID},
                           JSON{descriptor.id.value()}, KEYWORD_ID_HASH);
  }
  if (!descriptor.types.empty()) {
    node.assign_assume_new(JSON::String{KEYWORD_TYPE},
                           types_to_array(descriptor.types), KEYWORD_TYPE_HASH);
  }

  if (!value.is_object()) {
    return node;
  }

  // A graph node asserts its members in the named graph it identifies, with the
  // members carried by a subject node that shares its identifier.
  if (descriptor.graph) {
    auto inner{JSON::make_object()};
    if (descriptor.id.has_value()) {
      inner.assign_assume_new(JSON::String{KEYWORD_ID},
                              JSON{descriptor.id.value()}, KEYWORD_ID_HASH);
    }
    std::vector<JSON> graph_nodes;
    fill_node(inner, value, pointer, range, graph_nodes);
    auto graph{JSON::make_array()};
    if (inner.object_size() > (descriptor.id.has_value() ? 1 : 0)) {
      graph.push_back(std::move(inner));
    }
    for (auto &extra : graph_nodes) {
      graph.push_back(std::move(extra));
    }
    node.assign_assume_new(JSON::String{KEYWORD_GRAPH}, std::move(graph),
                           KEYWORD_GRAPH_HASH);
    return node;
  }

  fill_node(node, value, pointer, range, standalone);
  return node;
}

template <typename PointerT>
auto materialize_value(const JSON &value, PointerT &pointer,
                       AnnotationRange<PointerT> range,
                       std::vector<JSON> &standalone,
                       const std::vector<JSONLDEdge> **matched_edges)
    -> std::optional<JSON> {
  if (matched_edges != nullptr) {
    *matched_edges = nullptr;
  }

  if (value.is_null()) {
    return std::nullopt;
  }

  // Every annotation in the range extends the current position, so one of
  // equal length is the annotation of the position itself and sorts first
  if (range.begin == range.end ||
      (*range.begin)->pointer.size() != pointer.size()) {
    // An undescribed object with described descendants becomes an anonymous
    // blank node so its children have a subject. Anything else is not
    // annotated.
    if (value.is_object() && range.begin != range.end) {
      auto node{JSON::make_object()};
      fill_node(node, value, pointer, range, standalone);
      if (node.empty()) {
        return std::nullopt;
      }
      return node;
    }
    return std::nullopt;
  }

  const auto &descriptor{(*range.begin)->descriptor};
  range.begin += 1;
  if (matched_edges != nullptr) {
    *matched_edges = &descriptor.edges;
  }
  if (std::holds_alternative<JSONLDNode>(descriptor.value)) {
    return materialize_node(std::get<JSONLDNode>(descriptor.value), value,
                            pointer, range, standalone);
  }
  if (std::holds_alternative<JSONLDLiteral>(descriptor.value)) {
    return materialize_literal(std::get<JSONLDLiteral>(descriptor.value),
                               value);
  }
  if (std::holds_alternative<JSONLDReference>(descriptor.value)) {
    return materialize_reference(std::get<JSONLDReference>(descriptor.value));
  }

  const auto &collection{std::get<JSONLDCollection>(descriptor.value)};
  switch (collection.container) {
    case JSONLDContainer::List:
    case JSONLDContainer::Set:
      if (!value.is_array()) {
        return std::nullopt;
      }
      return build_collection(value, pointer, range, standalone,
                              collection.container == JSONLDContainer::List);
    case JSONLDContainer::Language:
      assert(value.is_object());
      // A language-tagged literal cannot be the object of a reverse property.
      assert(std::ranges::none_of(
          descriptor.edges,
          [](const JSONLDEdge &edge) -> bool { return edge.reverse; }));
      return build_language_collection(value);
    case JSONLDContainer::Index:
      assert(value.is_object());
      return build_index_collection(value, pointer, range, standalone);
  }

  std::unreachable();
}

template <typename PointerT>
auto fill_node(JSON &node, const JSON &instance_object, PointerT &pointer,
               const AnnotationRange<PointerT> &range,
               std::vector<JSON> &standalone) -> void {
  std::vector<std::reference_wrapper<const JSON::String>> keys;
  keys.reserve(instance_object.object_size());
  for (const auto &entry : instance_object.as_object()) {
    keys.push_back(std::cref(entry.first));
  }
  std::ranges::sort(keys, [](const auto &left, const auto &right) -> bool {
    return left.get() < right.get();
  });

  auto iterator{range.begin};
  for (const auto key : keys) {
    push_property(pointer, key.get());
    const std::vector<JSONLDEdge> *edges{nullptr};
    auto child_value{materialize_value(
        instance_object.at(key.get()), pointer,
        child_range(iterator, range.end, pointer), standalone, &edges)};
    pointer.pop_back();
    if (!child_value.has_value()) {
      continue;
    }

    if (edges == nullptr || edges->empty()) {
      // Without an edge a node cannot attach to its parent, so it is asserted
      // as a standalone node in the current graph. A non-node cannot be
      // asserted.
      if (is_node_object(child_value.value())) {
        standalone.push_back(std::move(child_value.value()));
      }
      continue;
    }

    attach(node, *edges, std::move(child_value.value()));
  }
}

template <typename PointerT>
auto materialize_root(const JSON &instance,
                      const JSONLDBasicAnnotationList<PointerT> &annotations)
    -> JSON {
  AnnotationIndex<PointerT> index;
  index.reserve(annotations.size());
  for (const auto &annotation : annotations) {
    index.push_back(&annotation);
  }

  std::ranges::stable_sort(index,
                           [](const auto *left, const auto *right) -> bool {
                             return left->pointer < right->pointer;
                           });
  const auto duplicates{std::ranges::unique(
      index, [](const auto *left, const auto *right) -> bool {
        return left->pointer == right->pointer;
      })};
  index.erase(duplicates.begin(), duplicates.end());

  std::vector<JSON> standalone;
  PointerT pointer;
  auto root{materialize_value(instance, pointer, {index.cbegin(), index.cend()},
                              standalone)};

  auto result{JSON::make_array()};
  if (root.has_value()) {
    // The default graph may only hold node objects. A top-level value or list
    // object, whether the root itself or an element of a root set, carries no
    // triples and is dropped.
    if (root->is_array()) {
      for (auto &element : root->as_array()) {
        if (is_node_object(element)) {
          result.push_back(std::move(element));
        }
      }
    } else if (is_node_object(root.value())) {
      result.push_back(std::move(root.value()));
    }
  }

  for (auto &node : standalone) {
    result.push_back(std::move(node));
  }

  return result;
}

} // namespace

auto jsonld_materialize(const JSON &instance,
                        const JSONLDAnnotationList &annotations) -> JSON {
  return materialize_root<Pointer>(instance, annotations);
}

auto jsonld_materialize(const JSON &instance,
                        const JSONLDWeakAnnotationList &annotations) -> JSON {
  return materialize_root<WeakPointer>(instance, annotations);
}

} // namespace sourcemeta::core
