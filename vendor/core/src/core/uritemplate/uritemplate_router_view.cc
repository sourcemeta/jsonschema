#include <sourcemeta/core/uritemplate.h>

#include <cassert>       // assert
#include <cstring>       // std::memcmp, std::memcpy
#include <fstream>       // std::ofstream, std::ifstream
#include <limits>        // std::numeric_limits
#include <queue>         // std::queue
#include <string>        // std::string
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace sourcemeta::core {

namespace {

constexpr std::uint32_t ROUTER_MAGIC = 0x52544552; // "RTER"
constexpr std::uint32_t ROUTER_VERSION = 2;
constexpr std::uint32_t NO_CHILD = std::numeric_limits<std::uint32_t>::max();

// Type tags for argument value serialization
constexpr std::uint8_t ARGUMENT_TYPE_STRING = 0x00;
constexpr std::uint8_t ARGUMENT_TYPE_INTEGER = 0x01;
constexpr std::uint8_t ARGUMENT_TYPE_BOOLEAN = 0x02;

struct RouterHeader {
  std::uint32_t magic;
  std::uint32_t version;
  std::uint32_t node_count;
  std::uint32_t string_table_offset;
  std::uint32_t arguments_offset;
};

struct ArgumentEntryHeader {
  std::uint16_t identifier;
  std::uint32_t blob_offset;
  std::uint32_t blob_length;
};

// Binary search for a literal child matching the given segment
inline auto binary_search_literal_children(
    const URITemplateRouterView::Node *nodes, const char *string_table,
    const std::size_t string_table_size, const std::uint32_t first_child,
    const std::uint32_t child_count, const char *segment,
    const std::uint32_t segment_length) noexcept -> std::uint32_t {
  std::uint32_t low = 0;
  std::uint32_t high = child_count;

  while (low < high) {
    const auto middle = low + (high - low) / 2;
    const auto child_index = first_child + middle;
    const auto &child = nodes[child_index];

    if (child.string_offset > string_table_size ||
        child.string_length > string_table_size - child.string_offset) {
      return NO_CHILD;
    }

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

  // Build the arguments section
  const auto &route_arguments = router.arguments();
  const auto entry_count = static_cast<std::uint16_t>(route_arguments.size());

  // Build per-entry blobs
  std::vector<ArgumentEntryHeader> argument_entries;
  std::vector<std::uint8_t> argument_blob;
  argument_entries.reserve(entry_count);

  for (const auto &[identifier, arguments] : route_arguments) {
    ArgumentEntryHeader entry{};
    entry.identifier = identifier;
    entry.blob_offset = static_cast<std::uint32_t>(argument_blob.size());

    assert(arguments.size() <= std::numeric_limits<std::uint16_t>::max());
    const auto arg_count = static_cast<std::uint16_t>(arguments.size());
    argument_blob.push_back(static_cast<std::uint8_t>(arg_count & 0xFF));
    argument_blob.push_back(static_cast<std::uint8_t>(arg_count >> 8));

    for (const auto &[name, value] : arguments) {
      // Write key_length + key_bytes
      assert(name.size() <= std::numeric_limits<std::uint16_t>::max());
      const auto key_length = static_cast<std::uint16_t>(name.size());
      argument_blob.push_back(static_cast<std::uint8_t>(key_length & 0xFF));
      argument_blob.push_back(static_cast<std::uint8_t>(key_length >> 8));
      argument_blob.insert(argument_blob.end(), name.begin(), name.end());

      // Write type_tag + value_length + value_bytes
      if (std::holds_alternative<std::string_view>(value)) {
        const auto string_value = std::get<std::string_view>(value);
        assert(string_value.size() <=
               std::numeric_limits<std::uint16_t>::max());
        const auto value_length =
            static_cast<std::uint16_t>(string_value.size());
        argument_blob.push_back(ARGUMENT_TYPE_STRING);
        argument_blob.push_back(static_cast<std::uint8_t>(value_length & 0xFF));
        argument_blob.push_back(static_cast<std::uint8_t>(value_length >> 8));
        argument_blob.insert(argument_blob.end(), string_value.begin(),
                             string_value.end());
      } else if (std::holds_alternative<std::int64_t>(value)) {
        const auto integer_value = std::get<std::int64_t>(value);
        argument_blob.push_back(ARGUMENT_TYPE_INTEGER);
        argument_blob.push_back(8);
        argument_blob.push_back(0);
        const auto old_size = argument_blob.size();
        argument_blob.resize(old_size + 8);
        std::memcpy(argument_blob.data() + old_size, &integer_value, 8);
      } else {
        const auto boolean_value = std::get<bool>(value);
        argument_blob.push_back(ARGUMENT_TYPE_BOOLEAN);
        argument_blob.push_back(1);
        argument_blob.push_back(0);
        argument_blob.push_back(boolean_value ? 1 : 0);
      }
    }

    entry.blob_length =
        static_cast<std::uint32_t>(argument_blob.size()) - entry.blob_offset;
    argument_entries.push_back(entry);
  }

  RouterHeader header{};
  header.magic = ROUTER_MAGIC;
  header.version = ROUTER_VERSION;
  header.node_count = static_cast<std::uint32_t>(nodes.size());
  header.string_table_offset = static_cast<std::uint32_t>(
      sizeof(RouterHeader) + nodes.size() * sizeof(Node));
  header.arguments_offset = static_cast<std::uint32_t>(
      header.string_table_offset + string_table.size());

  std::ofstream file(path, std::ios::binary);
  if (!file) {
    throw URITemplateRouterSaveError{path, "Failed to open file for writing"};
  }

  file.write(reinterpret_cast<const char *>(&header), sizeof(header));
  file.write(reinterpret_cast<const char *>(nodes.data()),
             static_cast<std::streamsize>(nodes.size() * sizeof(Node)));
  file.write(string_table.data(),
             static_cast<std::streamsize>(string_table.size()));

  // Write arguments section
  file.write(reinterpret_cast<const char *>(&entry_count), sizeof(entry_count));
  for (const auto &entry : argument_entries) {
    file.write(reinterpret_cast<const char *>(&entry.identifier),
               sizeof(entry.identifier));
    file.write(reinterpret_cast<const char *>(&entry.blob_offset),
               sizeof(entry.blob_offset));
    file.write(reinterpret_cast<const char *>(&entry.blob_length),
               sizeof(entry.blob_length));
  }

  if (!argument_blob.empty()) {
    file.write(reinterpret_cast<const char *>(argument_blob.data()),
               static_cast<std::streamsize>(argument_blob.size()));
  }

  if (!file) {
    throw URITemplateRouterSaveError{path,
                                     "Failed to write router data to file"};
  }
}

URITemplateRouterView::URITemplateRouterView(
    const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    throw URITemplateRouterReadError{path};
  }

  const auto position = file.tellg();
  if (position < 0) {
    throw URITemplateRouterReadError{path};
  }

  const auto size = static_cast<std::size_t>(position);
  file.seekg(0, std::ios::beg);
  this->data_.resize(size);
  file.read(reinterpret_cast<char *>(this->data_.data()),
            static_cast<std::streamsize>(size));
  if (!file) {
    throw URITemplateRouterReadError{path};
  }
}

URITemplateRouterView::URITemplateRouterView(const std::uint8_t *data,
                                             const std::size_t size)
    : data_{data, data + size} {}

auto URITemplateRouterView::match(const std::string_view path,
                                  const URITemplateRouter::Callback &callback)
    const -> URITemplateRouter::Identifier {
  if (this->data_.size() < sizeof(RouterHeader)) {
    return 0;
  }

  const auto *header =
      reinterpret_cast<const RouterHeader *>(this->data_.data());
  if (header->magic != ROUTER_MAGIC || header->version != ROUTER_VERSION) {
    return 0;
  }

  if (header->node_count == 0 ||
      header->node_count >
          (this->data_.size() - sizeof(RouterHeader)) / sizeof(Node)) {
    return 0;
  }

  const auto *nodes =
      reinterpret_cast<const Node *>(this->data_.data() + sizeof(RouterHeader));
  const auto nodes_size =
      static_cast<std::size_t>(header->node_count) * sizeof(Node);
  const auto expected_string_table_offset = sizeof(RouterHeader) + nodes_size;
  if (header->string_table_offset < expected_string_table_offset ||
      header->string_table_offset > this->data_.size()) {
    return 0;
  }

  if (header->arguments_offset < header->string_table_offset ||
      header->arguments_offset > this->data_.size()) {
    return 0;
  }

  const auto *string_table = reinterpret_cast<const char *>(
      this->data_.data() + header->string_table_offset);
  const auto string_table_size =
      header->arguments_offset - header->string_table_offset;

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

    if (root.first_literal_child >= header->node_count ||
        root.literal_child_count >
            header->node_count - root.first_literal_child) {
      return 0;
    }

    const auto match = binary_search_literal_children(
        nodes, string_table, string_table_size, root.first_literal_child,
        root.literal_child_count, "", 0);
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
    const auto node_count = header->node_count;

    // Try literal children first
    if (node.first_literal_child != NO_CHILD) {
      if (node.first_literal_child >= node_count ||
          node.literal_child_count > node_count - node.first_literal_child) {
        return 0;
      }

      const auto literal_match = binary_search_literal_children(
          nodes, string_table, string_table_size, node.first_literal_child,
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
      if (node.variable_child >= node_count ||
          variable_index >
              std::numeric_limits<URITemplateRouter::Index>::max()) {
        return 0;
      }

      const auto &variable_node = nodes[node.variable_child];

      if (variable_node.string_offset > string_table_size ||
          variable_node.string_length >
              string_table_size - variable_node.string_offset) {
        return 0;
      }

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

auto URITemplateRouterView::arguments(
    const URITemplateRouter::Identifier identifier,
    const URITemplateRouter::ArgumentCallback &callback) const -> void {
  if (this->data_.size() < sizeof(RouterHeader)) {
    return;
  }

  const auto *header =
      reinterpret_cast<const RouterHeader *>(this->data_.data());
  if (header->magic != ROUTER_MAGIC || header->version != ROUTER_VERSION) {
    return;
  }

  const auto arguments_start =
      static_cast<std::size_t>(header->arguments_offset);
  const auto data_size = this->data_.size();
  if (arguments_start >= data_size) {
    return;
  }

  const auto section_size = data_size - arguments_start;
  if (section_size < sizeof(std::uint16_t)) {
    return;
  }

  std::uint16_t entry_count = 0;
  std::memcpy(&entry_count, this->data_.data() + arguments_start,
              sizeof(entry_count));
  if (entry_count == 0) {
    return;
  }

  constexpr std::size_t ENTRY_SIZE =
      sizeof(std::uint16_t) + sizeof(std::uint32_t) + sizeof(std::uint32_t);
  const auto entries_offset = arguments_start + sizeof(entry_count);
  const auto entries_size = static_cast<std::size_t>(entry_count) * ENTRY_SIZE;
  if (entries_size > data_size - entries_offset) {
    return;
  }

  const auto blob_area_offset = entries_offset + entries_size;

  for (std::uint16_t index = 0; index < entry_count; ++index) {
    const auto entry_offset = entries_offset + index * ENTRY_SIZE;

    std::uint16_t entry_identifier = 0;
    std::memcpy(&entry_identifier, this->data_.data() + entry_offset,
                sizeof(entry_identifier));
    if (entry_identifier != identifier) {
      continue;
    }

    std::uint32_t blob_offset = 0;
    std::uint32_t blob_length = 0;
    std::memcpy(&blob_offset,
                this->data_.data() + entry_offset + sizeof(std::uint16_t),
                sizeof(blob_offset));
    std::memcpy(&blob_length,
                this->data_.data() + entry_offset + sizeof(std::uint16_t) +
                    sizeof(std::uint32_t),
                sizeof(blob_length));

    const auto blob_abs_offset = blob_area_offset + blob_offset;
    if (blob_offset > data_size - blob_area_offset ||
        blob_length > data_size - blob_abs_offset ||
        blob_length < sizeof(std::uint16_t)) {
      return;
    }

    std::uint16_t arg_count = 0;
    std::memcpy(&arg_count, this->data_.data() + blob_abs_offset,
                sizeof(arg_count));
    auto cursor = blob_abs_offset + sizeof(arg_count);
    const auto blob_end = blob_abs_offset + blob_length;

    for (std::uint16_t arg_index = 0; arg_index < arg_count; ++arg_index) {
      if (cursor + sizeof(std::uint16_t) > blob_end) {
        return;
      }

      std::uint16_t key_length = 0;
      std::memcpy(&key_length, this->data_.data() + cursor, sizeof(key_length));
      cursor += sizeof(key_length);
      if (key_length > blob_end - cursor) {
        return;
      }

      const std::string_view name{
          reinterpret_cast<const char *>(this->data_.data() + cursor),
          key_length};
      cursor += key_length;

      if (cursor + sizeof(std::uint8_t) + sizeof(std::uint16_t) > blob_end) {
        return;
      }

      const auto type_tag = this->data_[cursor];
      cursor += sizeof(std::uint8_t);
      std::uint16_t value_length = 0;
      std::memcpy(&value_length, this->data_.data() + cursor,
                  sizeof(value_length));
      cursor += sizeof(value_length);
      if (value_length > blob_end - cursor) {
        return;
      }

      switch (type_tag) {
        case ARGUMENT_TYPE_STRING: {
          const std::string_view string_value{
              reinterpret_cast<const char *>(this->data_.data() + cursor),
              value_length};
          callback(name, string_value);
          break;
        }

        case ARGUMENT_TYPE_INTEGER: {
          if (value_length != 8) {
            return;
          }

          std::int64_t integer_value = 0;
          std::memcpy(&integer_value, this->data_.data() + cursor, 8);
          callback(name, integer_value);
          break;
        }

        case ARGUMENT_TYPE_BOOLEAN: {
          if (value_length != 1) {
            return;
          }

          callback(name, this->data_[cursor] != 0);
          break;
        }

        default:
          return;
      }

      cursor += value_length;
    }

    return;
  }
}

} // namespace sourcemeta::core
