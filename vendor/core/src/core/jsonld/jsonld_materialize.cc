#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>

#include "jsonld_keywords.h"

#include <algorithm>   // std::ranges::sort
#include <cstddef>     // std::size_t
#include <functional>  // std::reference_wrapper, std::cref
#include <optional>    // std::optional, std::nullopt
#include <type_traits> // std::is_same_v
#include <utility>     // std::move, std::unreachable
#include <variant>     // std::holds_alternative, std::get
#include <vector>      // std::vector

namespace sourcemeta::core {

namespace {

template <typename PointerT>
auto materialize_value(const JSON &value, PointerT &pointer,
                       const JSONLDBasicAnnotationMap<PointerT> &map,
                       std::vector<JSON> &standalone) -> std::optional<JSON>;

template <typename PointerT>
auto fill_node(JSON &node, const JSON &instance_object, PointerT &pointer,
               const JSONLDBasicAnnotationMap<PointerT> &map,
               std::vector<JSON> &standalone) -> void;

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
  if (!node.defines(predicate)) {
    node.assign_assume_new(JSON::String{predicate}, JSON::make_array());
  }
  return node.at(predicate);
}

// The property array nested under @reverse and the given predicate.
auto reverse_target(JSON &node, const JSON::StringView predicate) -> JSON & {
  if (!node.defines(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)) {
    node.assign_assume_new(JSON::String{KEYWORD_REVERSE}, JSON::make_object(),
                           KEYWORD_REVERSE_HASH);
  }
  auto &reverse{node.at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)};
  if (!reverse.defines(predicate)) {
    reverse.assign_assume_new(JSON::String{predicate}, JSON::make_array());
  }
  return reverse.at(predicate);
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
                      const JSONLDBasicAnnotationMap<PointerT> &map,
                      std::vector<JSON> &standalone, const bool ordered)
    -> JSON {
  auto elements{JSON::make_array()};
  for (std::size_t index = 0; index < value.size(); index += 1) {
    pointer.push_back(index);
    auto element{materialize_value(value.at(index), pointer, map, standalone)};
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

template <typename PointerT>
auto materialize_node(const JSONLDNode &descriptor, const JSON &value,
                      PointerT &pointer,
                      const JSONLDBasicAnnotationMap<PointerT> &map,
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
    fill_node(inner, value, pointer, map, graph_nodes);
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

  fill_node(node, value, pointer, map, standalone);
  return node;
}

template <typename PointerT>
auto materialize_value(const JSON &value, PointerT &pointer,
                       const JSONLDBasicAnnotationMap<PointerT> &map,
                       std::vector<JSON> &standalone) -> std::optional<JSON> {
  if (value.is_null()) {
    return std::nullopt;
  }

  const auto iterator{map.find(pointer)};
  if (iterator == map.cend()) {
    // An undescribed object with described descendants becomes an anonymous
    // blank node so its children have a subject. Anything else is not
    // annotated.
    if (value.is_object()) {
      auto node{JSON::make_object()};
      fill_node(node, value, pointer, map, standalone);
      if (node.empty()) {
        return std::nullopt;
      }
      return node;
    }
    return std::nullopt;
  }

  const auto &descriptor{iterator->second};
  if (std::holds_alternative<JSONLDNode>(descriptor.value)) {
    return materialize_node(std::get<JSONLDNode>(descriptor.value), value,
                            pointer, map, standalone);
  }
  if (std::holds_alternative<JSONLDLiteral>(descriptor.value)) {
    return materialize_literal(std::get<JSONLDLiteral>(descriptor.value),
                               value);
  }
  if (std::holds_alternative<JSONLDReference>(descriptor.value)) {
    return materialize_reference(std::get<JSONLDReference>(descriptor.value));
  }

  const auto &collection{std::get<JSONLDCollection>(descriptor.value)};
  if (!value.is_array()) {
    return std::nullopt;
  }
  return build_collection(value, pointer, map, standalone, collection.ordered);
}

template <typename PointerT>
auto fill_node(JSON &node, const JSON &instance_object, PointerT &pointer,
               const JSONLDBasicAnnotationMap<PointerT> &map,
               std::vector<JSON> &standalone) -> void {
  std::vector<std::reference_wrapper<const JSON::String>> keys;
  keys.reserve(instance_object.object_size());
  for (const auto &entry : instance_object.as_object()) {
    keys.push_back(std::cref(entry.first));
  }
  std::ranges::sort(keys, [](const auto &left, const auto &right) -> bool {
    return left.get() < right.get();
  });

  for (const auto key : keys) {
    push_property(pointer, key.get());
    const auto child_iterator{map.find(pointer)};
    auto child_value{materialize_value(instance_object.at(key.get()), pointer,
                                       map, standalone)};
    pointer.pop_back();
    if (!child_value.has_value()) {
      continue;
    }

    const std::vector<JSONLDEdge> *edges{
        child_iterator == map.cend() ? nullptr : &child_iterator->second.edges};
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
                      const JSONLDBasicAnnotationMap<PointerT> &map) -> JSON {
  std::vector<JSON> standalone;
  PointerT pointer;
  auto root{materialize_value(instance, pointer, map, standalone)};

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

auto jsonld_materialize(const JSON &instance, const JSONLDAnnotationMap &map)
    -> JSON {
  return materialize_root<Pointer>(instance, map);
}

auto jsonld_materialize(const JSON &instance,
                        const JSONLDWeakAnnotationMap &map) -> JSON {
  return materialize_root<WeakPointer>(instance, map);
}

} // namespace sourcemeta::core
