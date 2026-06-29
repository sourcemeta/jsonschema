#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <algorithm> // std::ranges::sort
#include <optional>  // std::optional, std::nullopt
#include <string>    // std::to_string
#include <utility>   // std::move
#include <vector>    // std::vector

namespace sourcemeta::core {

namespace {

auto is_blank_node(const JSON::StringView value) -> bool {
  return value.starts_with("_:");
}

// Append a value to the property array of node, creating the array as needed.
auto append_value(JSON &node, const JSON::StringView property, JSON value)
    -> void {
  if (!node.defines(property)) {
    node.assign_assume_new(JSON::String{property}, JSON::make_array());
  }
  node.at(property).push_back(std::move(value));
}

// Append a value to the property array of node unless an equivalent value is
// already present.
auto add_unique(JSON &node, const JSON::StringView property, JSON value)
    -> void {
  if (!node.defines(property)) {
    node.assign_assume_new(JSON::String{property}, JSON::make_array());
  }
  auto &array{node.at(property)};
  for (const auto &item : array.as_array()) {
    if (item == value) {
      return;
    }
  }
  array.push_back(std::move(value));
}

} // namespace

auto generate_blank_node_identifier(
    BlankNodeState &state, const std::optional<JSON::StringView> &identifier)
    -> JSON::String {
  if (identifier.has_value()) {
    const auto iterator{state.identifiers.find(identifier.value())};
    if (iterator != state.identifiers.cend()) {
      return iterator->second;
    }
  }

  JSON::String result{"_:b"};
  result += std::to_string(state.counter);
  state.counter += 1;
  if (identifier.has_value()) {
    state.identifiers.emplace(identifier.value(), result);
  }
  return result;
}

auto generate_node_map(BlankNodeState &state, JSON &node_map,
                       const JSON &element, const JSON::StringView active_graph,
                       const JSON *active_subject,
                       const std::optional<JSON::StringView> &active_property,
                       JSON *list) -> void {
  if (element.is_array()) {
    for (const auto &item : element.as_array()) {
      generate_node_map(state, node_map, item, active_graph, active_subject,
                        active_property, list);
    }
    return;
  }

  if (!node_map.defines(active_graph)) {
    node_map.assign_assume_new(JSON::String{active_graph}, JSON::make_object());
  }

  // The algorithm consumes entries of element, so work on a mutable copy.
  JSON working{element};

  // Relabel blank node identifiers used as @type values.
  if (working.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
    auto &type_value{working.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
    if (type_value.is_array()) {
      for (auto &item : type_value.as_array()) {
        if (item.is_string() && is_blank_node(item.to_string())) {
          item = JSON{generate_blank_node_identifier(state, item.to_string())};
        }
      }
    } else if (type_value.is_string() &&
               is_blank_node(type_value.to_string())) {
      type_value =
          JSON{generate_blank_node_identifier(state, type_value.to_string())};
    }
  }

  // A value object is appended to the active property or the list accumulator.
  if (working.defines(KEYWORD_VALUE, KEYWORD_VALUE_HASH)) {
    if (list == nullptr) {
      auto &subject_node{
          node_map.at(active_graph).at(active_subject->to_string())};
      add_unique(subject_node, active_property.value(), std::move(working));
    } else {
      list->at(KEYWORD_LIST, KEYWORD_LIST_HASH).push_back(std::move(working));
    }
    return;
  }

  // A set object is a transparent wrapper around its items, which are processed
  // against the same subject and property. Expansion never emits one, but the
  // expanded form predicate accepts it.
  if (working.defines(KEYWORD_SET, KEYWORD_SET_HASH)) {
    generate_node_map(state, node_map,
                      working.at(KEYWORD_SET, KEYWORD_SET_HASH), active_graph,
                      active_subject, active_property, list);
    return;
  }

  // A list object recurses into a fresh list accumulator.
  if (working.defines(KEYWORD_LIST, KEYWORD_LIST_HASH)) {
    auto result{JSON::make_object()};
    result.assign_assume_new(JSON::String{KEYWORD_LIST}, JSON::make_array(),
                             KEYWORD_LIST_HASH);
    generate_node_map(state, node_map,
                      working.at(KEYWORD_LIST, KEYWORD_LIST_HASH), active_graph,
                      active_subject, active_property, &result);
    if (list == nullptr) {
      auto &subject_node{
          node_map.at(active_graph).at(active_subject->to_string())};
      append_value(subject_node, active_property.value(), std::move(result));
    } else {
      list->at(KEYWORD_LIST, KEYWORD_LIST_HASH).push_back(std::move(result));
    }
    return;
  }

  // A node object.
  std::optional<JSON::String> id;
  if (working.defines(KEYWORD_ID, KEYWORD_ID_HASH)) {
    const auto &id_value{working.at(KEYWORD_ID, KEYWORD_ID_HASH)};
    if (id_value.is_string()) {
      id = is_blank_node(id_value.to_string())
               ? generate_blank_node_identifier(state, id_value.to_string())
               : id_value.to_string();
    }
    working.erase(KEYWORD_ID);
  }
  if (!id.has_value()) {
    id = generate_blank_node_identifier(state, std::nullopt);
  }

  if (!node_map.at(active_graph).defines(id.value())) {
    auto node{JSON::make_object()};
    node.assign_assume_new(JSON::String{KEYWORD_ID}, JSON{id.value()},
                           KEYWORD_ID_HASH);
    node_map.at(active_graph)
        .assign_assume_new(JSON::String{id.value()}, std::move(node));
  }

  if (active_subject != nullptr && active_subject->is_object()) {
    // A reverse relationship wires the referenced subject into this node.
    add_unique(node_map.at(active_graph).at(id.value()),
               active_property.value(), *active_subject);
  } else if (active_property.has_value()) {
    auto reference{JSON::make_object()};
    reference.assign_assume_new(JSON::String{KEYWORD_ID}, JSON{id.value()},
                                KEYWORD_ID_HASH);
    if (list == nullptr) {
      add_unique(node_map.at(active_graph).at(active_subject->to_string()),
                 active_property.value(), std::move(reference));
    } else {
      list->at(KEYWORD_LIST, KEYWORD_LIST_HASH).push_back(std::move(reference));
    }
  }

  if (working.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
    auto &node{node_map.at(active_graph).at(id.value())};
    if (!node.defines(KEYWORD_TYPE, KEYWORD_TYPE_HASH)) {
      node.assign_assume_new(JSON::String{KEYWORD_TYPE}, JSON::make_array(),
                             KEYWORD_TYPE_HASH);
    }
    auto &node_types{node.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH)};
    for (const auto &item :
         working.at(KEYWORD_TYPE, KEYWORD_TYPE_HASH).as_array()) {
      bool present{false};
      for (const auto &existing : node_types.as_array()) {
        if (existing == item) {
          present = true;
          break;
        }
      }
      if (!present) {
        node_types.push_back(item);
      }
    }
    working.erase(KEYWORD_TYPE);
  }

  if (working.defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH)) {
    auto &node{node_map.at(active_graph).at(id.value())};
    const auto &index_value{working.at(KEYWORD_INDEX, KEYWORD_INDEX_HASH)};
    if (node.defines(KEYWORD_INDEX, KEYWORD_INDEX_HASH) &&
        node.at(KEYWORD_INDEX, KEYWORD_INDEX_HASH) != index_value) {
      throw JSONLDError("Conflicting indexes", empty_weak_pointer);
    }
    node.assign(JSON::String{KEYWORD_INDEX}, JSON{index_value});
    working.erase(KEYWORD_INDEX);
  }

  if (working.defines(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH)) {
    auto referenced_node{JSON::make_object()};
    referenced_node.assign_assume_new(JSON::String{KEYWORD_ID},
                                      JSON{id.value()}, KEYWORD_ID_HASH);
    for (const auto &entry :
         working.at(KEYWORD_REVERSE, KEYWORD_REVERSE_HASH).as_object()) {
      for (const auto &value : entry.second.as_array()) {
        generate_node_map(state, node_map, value, active_graph,
                          &referenced_node, entry.first, nullptr);
      }
    }
    working.erase(KEYWORD_REVERSE);
  }

  if (working.defines(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH)) {
    generate_node_map(state, node_map,
                      working.at(KEYWORD_GRAPH, KEYWORD_GRAPH_HASH), id.value(),
                      nullptr, std::nullopt, nullptr);
    working.erase(KEYWORD_GRAPH);
  }

  if (working.defines(KEYWORD_INCLUDED, KEYWORD_INCLUDED_HASH)) {
    generate_node_map(state, node_map,
                      working.at(KEYWORD_INCLUDED, KEYWORD_INCLUDED_HASH),
                      active_graph, nullptr, std::nullopt, nullptr);
    working.erase(KEYWORD_INCLUDED);
  }

  std::vector<JSON::StringView> properties;
  properties.reserve(working.object_size());
  for (const auto &entry : working.as_object()) {
    properties.push_back(entry.first);
  }
  std::ranges::sort(properties);
  const JSON subject{id.value()};
  for (const auto property : properties) {
    const JSON::String key{is_blank_node(property)
                               ? generate_blank_node_identifier(state, property)
                               : JSON::String{property}};
    auto &node{node_map.at(active_graph).at(id.value())};
    if (!node.defines(key)) {
      node.assign_assume_new(JSON::String{key}, JSON::make_array());
    }
    generate_node_map(state, node_map, working.at(property), active_graph,
                      &subject, key, nullptr);
  }
}

} // namespace sourcemeta::core
