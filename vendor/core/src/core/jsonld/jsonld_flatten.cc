#include <sourcemeta/core/json.h>

#include "jsonld_algorithms.h"
#include "jsonld_keywords.h"

#include <algorithm> // std::ranges::sort
#include <optional>  // std::nullopt
#include <utility>   // std::move
#include <vector>    // std::vector

namespace sourcemeta::core {

namespace {

// The node with the only entry @id carries no statements, so it is dropped.
auto is_reference_only(const JSON &node) -> bool {
  return node.object_size() == 1 && node.defines(KEYWORD_ID, KEYWORD_ID_HASH);
}

// The identifiers of a graph's nodes in lexicographical order. The views are
// backed by the graph keys, so the graph must outlive the result.
auto sorted_ids(const JSON &graph) -> std::vector<JSON::StringView> {
  std::vector<JSON::StringView> ids;
  ids.reserve(graph.object_size());
  for (const auto &entry : graph.as_object()) {
    ids.push_back(entry.first);
  }
  std::ranges::sort(ids);
  return ids;
}

} // namespace

auto flatten(BlankNodeState &state, const JSON &element) -> JSON {
  auto node_map{JSON::make_object()};
  node_map.assign_assume_new(JSON::String{KEYWORD_DEFAULT}, JSON::make_object(),
                             KEYWORD_DEFAULT_HASH);
  generate_node_map(state, node_map, element, KEYWORD_DEFAULT, nullptr,
                    std::nullopt, nullptr);

  // Fold every named graph into a node of the default graph as an @graph entry.
  std::vector<JSON::StringView> graph_names;
  for (const auto &entry : node_map.as_object()) {
    if (entry.first != KEYWORD_DEFAULT) {
      graph_names.push_back(entry.first);
    }
  }
  std::ranges::sort(graph_names);

  auto &default_graph{node_map.at(KEYWORD_DEFAULT, KEYWORD_DEFAULT_HASH)};
  for (const auto graph_name : graph_names) {
    if (!default_graph.defines(graph_name)) {
      auto node{JSON::make_object()};
      node.assign_assume_new(JSON::String{KEYWORD_ID}, JSON{graph_name},
                             KEYWORD_ID_HASH);
      default_graph.assign_assume_new(JSON::String{graph_name},
                                      std::move(node));
    }

    auto graph_array{JSON::make_array()};
    const auto &graph{node_map.at(graph_name)};
    for (const auto &id : sorted_ids(graph)) {
      const auto &node{graph.at(id)};
      if (!is_reference_only(node)) {
        graph_array.push_back(node);
      }
    }
    default_graph.at(graph_name)
        .assign(JSON::String{KEYWORD_GRAPH}, std::move(graph_array));
  }

  auto flattened{JSON::make_array()};
  for (const auto &id : sorted_ids(default_graph)) {
    const auto &node{default_graph.at(id)};
    if (!is_reference_only(node)) {
      flattened.push_back(node);
    }
  }
  return flattened;
}

} // namespace sourcemeta::core
