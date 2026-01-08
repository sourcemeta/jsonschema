#include <sourcemeta/core/uritemplate.h>

#include <cassert>       // assert
#include <cstring>       // std::memcmp
#include <fstream>       // std::ofstream
#include <limits>        // std::numeric_limits
#include <queue>         // std::queue
#include <string>        // std::string
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

constexpr std::uint32_t ROUTER_MAGIC = 0x52544552; // "RTER"
constexpr std::uint32_t ROUTER_VERSION = 1;
constexpr std::uint32_t NO_CHILD = std::numeric_limits<std::uint32_t>::max();

struct RouterHeader {
  std::uint32_t magic;
  std::uint32_t version;
  std::uint32_t node_count;
  std::uint32_t string_table_offset;
};

// Binary search for a literal child matching the given segment
inline auto binary_search_literal_children(
    const URITemplateRouterView::Node *nodes, const char *string_table,
    const std::uint32_t first_child, const std::uint32_t child_count,
    const char *segment, const std::uint32_t segment_length) noexcept
    -> std::uint32_t {
  std::uint32_t low = 0;
  std::uint32_t high = child_count;

  while (low < high) {
    const auto middle = low + (high - low) / 2;
    const auto child_index = first_child + middle;
    const auto &child = nodes[child_index];

    // Compare segments lexicographically (content first, then length)
    const auto min_length = segment_length < child.string_length
                                ? segment_length
                                : child.string_length;
    const auto content_comparison =
        std::memcmp(segment, string_table + child.string_offset, min_length);
    const auto comparison = content_comparison != 0
                                ? content_comparison
                                : static_cast<int>(segment_length) -
                                      static_cast<int>(child.string_length);

    if (comparison < 0) {
      high = middle;
    } else if (comparison > 0) {
      low = middle + 1;
    } else {
      return child_index;
    }
  }

  return NO_CHILD;
}

} // namespace

auto URITemplateRouterView::save(const URITemplateRouter &router,
                                 const std::filesystem::path &path) -> void {
  std::vector<Node> nodes;
  std::string string_table;
  std::queue<const URITemplateRouter::Node *> queue;
  std::unordered_map<const URITemplateRouter::Node *, std::uint32_t>
      node_indices;

  const auto &root = router.root();

  Node root_serialized{};
  root_serialized.string_offset = 0;
  root_serialized.string_length = 0;
  root_serialized.type = URITemplateRouter::NodeType::Root;
  root_serialized.padding = 0;
  root_serialized.identifier = root.identifier;

  if (root.literals.empty()) {
    root_serialized.first_literal_child = NO_CHILD;
    root_serialized.literal_child_count = 0;
  } else {
    root_serialized.first_literal_child = 1;
    root_serialized.literal_child_count =
        static_cast<std::uint32_t>(root.literals.size());
    for (const auto &child : root.literals) {
      node_indices[child.get()] = static_cast<std::uint32_t>(queue.size() + 1);
      queue.push(child.get());
    }
  }

  if (root.variable) {
    root_serialized.variable_child =
        static_cast<std::uint32_t>(queue.size() + 1);
    node_indices[root.variable.get()] = root_serialized.variable_child;
    queue.push(root.variable.get());
  } else {
    root_serialized.variable_child = NO_CHILD;
  }

  nodes.push_back(root_serialized);

  while (!queue.empty()) {
    const auto *node = queue.front();
    queue.pop();

    Node serialized{};
    serialized.string_offset = static_cast<std::uint32_t>(string_table.size());
    serialized.type = node->type;
    serialized.string_length = static_cast<std::uint32_t>(node->value.size());
    string_table += node->value;

    serialized.padding = 0;
    serialized.identifier = node->identifier;

    const auto first_child_index =
        static_cast<std::uint32_t>(nodes.size() + queue.size() + 1);

    if (!node->literals.empty()) {
      serialized.first_literal_child = first_child_index;
      serialized.literal_child_count =
          static_cast<std::uint32_t>(node->literals.size());
      for (const auto &child : node->literals) {
        node_indices[child.get()] =
            static_cast<std::uint32_t>(nodes.size() + queue.size() + 1);
        queue.push(child.get());
      }
    } else {
      serialized.first_literal_child = NO_CHILD;
      serialized.literal_child_count = 0;
    }

    if (node->variable) {
      serialized.variable_child =
          static_cast<std::uint32_t>(nodes.size() + queue.size() + 1);
      node_indices[node->variable.get()] = serialized.variable_child;
      queue.push(node->variable.get());
    } else {
      serialized.variable_child = NO_CHILD;
    }

    nodes.push_back(serialized);
  }

  RouterHeader header{};
  header.magic = ROUTER_MAGIC;
  header.version = ROUTER_VERSION;
  header.node_count = static_cast<std::uint32_t>(nodes.size());
  header.string_table_offset = static_cast<std::uint32_t>(
      sizeof(RouterHeader) + nodes.size() * sizeof(Node));

  std::ofstream file(path, std::ios::binary);
  if (!file) {
    throw URITemplateRouterSaveError{path, "Failed to open file for writing"};
  }

  file.write(reinterpret_cast<const char *>(&header), sizeof(header));
  file.write(reinterpret_cast<const char *>(nodes.data()),
             static_cast<std::streamsize>(nodes.size() * sizeof(Node)));
  file.write(string_table.data(),
             static_cast<std::streamsize>(string_table.size()));

  if (!file) {
    throw URITemplateRouterSaveError{path,
                                     "Failed to write router data to file"};
  }
}

URITemplateRouterView::URITemplateRouterView(const std::filesystem::path &path)
    : file_view_{path} {}

auto URITemplateRouterView::match(const std::string_view path,
                                  const URITemplateRouter::Callback &callback)
    const -> URITemplateRouter::Identifier {
  const auto *header = this->file_view_.as<RouterHeader>();
  assert(header->magic == ROUTER_MAGIC);
  assert(header->version == ROUTER_VERSION);

  const auto *nodes = this->file_view_.as<Node>(sizeof(RouterHeader));
  const auto *string_table =
      header->string_table_offset < this->file_view_.size()
          ? this->file_view_.as<char>(header->string_table_offset)
          : nullptr;

  // Empty path matches empty template
  if (path.empty()) {
    return nodes[0].identifier;
  }

  // Root path "/" is stored as an empty literal segment
  if (path.size() == 1 && path[0] == '/') {
    const auto &root = nodes[0];
    if (root.first_literal_child == NO_CHILD) {
      return 0;
    }

    const auto match = binary_search_literal_children(
        nodes, string_table, root.first_literal_child, root.literal_child_count,
        "", 0);
    return match != NO_CHILD ? nodes[match].identifier : 0;
  }

  // Walk the trie, matching each path segment
  std::uint32_t current_node = 0;
  const char *position = path.data();
  const char *const path_end = position + path.size();

  std::size_t variable_index = 0;

  // Skip leading slash
  if (position < path_end && *position == '/') {
    ++position;
  }

  while (true) {
    // Extract segment
    const char *segment_start = position;
    while (position < path_end && *position != '/') {
      ++position;
    }

    const auto segment_length =
        static_cast<std::uint32_t>(position - segment_start);

    // Empty segment (from double slash or trailing slash) doesn't match
    if (segment_length == 0) {
      return 0;
    }

    const auto &node = nodes[current_node];

    // Try literal children first
    if (node.first_literal_child != NO_CHILD) {
      const auto literal_match = binary_search_literal_children(
          nodes, string_table, node.first_literal_child,
          node.literal_child_count, segment_start, segment_length);
      if (literal_match != NO_CHILD) {
        current_node = literal_match;
        if (position >= path_end) {
          break;
        }
        ++position;
        continue;
      }
    }

    // Fall back to variable child
    if (node.variable_child != NO_CHILD) {
      assert(variable_index <=
             std::numeric_limits<URITemplateRouter::Index>::max());
      const auto &variable_node = nodes[node.variable_child];

      // Check if this is an expansion (catch-all)
      if (variable_node.type == URITemplateRouter::NodeType::Expansion) {
        const auto remaining_length =
            static_cast<std::uint32_t>(path_end - segment_start);
        callback(static_cast<URITemplateRouter::Index>(variable_index),
                 {string_table + variable_node.string_offset,
                  variable_node.string_length},
                 {segment_start, remaining_length});
        return variable_node.identifier;
      }

      // Regular variable - match single segment
      callback(static_cast<URITemplateRouter::Index>(variable_index),
               {string_table + variable_node.string_offset,
                variable_node.string_length},
               {segment_start, segment_length});
      ++variable_index;
      current_node = node.variable_child;
      if (position >= path_end) {
        break;
      }
      ++position;
      continue;
    }

    // No match
    return 0;
  }

  return nodes[current_node].identifier;
}

} // namespace sourcemeta::core
