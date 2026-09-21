#ifndef SOURCEMETA_CORE_MARKDOWN_DOCUMENT_H_
#define SOURCEMETA_CORE_MARKDOWN_DOCUMENT_H_

#include <algorithm> // std::max
#include <cstddef>   // std::size_t
#include <cstdint>   // std::int32_t, std::uint8_t, std::uint16_t, std::uint32_t
#include <cstring>   // std::memcpy
#include <string>    // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace sourcemeta::core::markdown {

enum class NodeType : std::uint8_t {
  Document,
  BlockQuote,
  List,
  Item,
  CodeBlock,
  HTMLBlock,
  Paragraph,
  Heading,
  ThematicBreak,
  FootnoteDefinition,
  Table,
  TableRow,
  TableCell,
  Text,
  SoftBreak,
  LineBreak,
  Code,
  HTMLInline,
  Emphasis,
  Strong,
  Link,
  Image,
  FootnoteReference,
  Strikethrough
};

constexpr std::uint32_t NO_NODE{0xFFFFFFFF};
constexpr std::uint32_t ROOT_NODE{0};

constexpr std::uint16_t FLAG_OPEN{1U << 0U};
constexpr std::uint16_t FLAG_LAST_LINE_BLANK{1U << 1U};
constexpr std::uint16_t FLAG_LAST_LINE_CHECKED{1U << 2U};
constexpr std::uint16_t FLAG_TABLE_VISITED{1U << 3U};
constexpr std::uint16_t FLAG_TASK{1U << 4U};
constexpr std::uint16_t FLAG_CHECKED{1U << 5U};
constexpr std::uint16_t FLAG_HEADER{1U << 6U};
constexpr std::uint16_t FLAG_FENCED{1U << 7U};
constexpr std::uint16_t FLAG_SETEXT{1U << 8U};
constexpr std::uint16_t FLAG_TIGHT{1U << 9U};
constexpr std::uint16_t FLAG_REFERENCED{1U << 10U};
constexpr std::uint16_t FLAG_ATTACHED{1U << 11U};
constexpr std::uint16_t FLAG_DETACHED{1U << 12U};
constexpr std::uint16_t FLAG_REFERENCE_LINK{1U << 13U};

// A block or inline element, whose generic fields mean different things for
// different types of nodes, such as the fence of a code block or the numbering
// of a footnote
struct Node {
  std::string_view literal;
  std::string_view title;
  std::uint32_t parent{NO_NODE};
  std::uint32_t first_child{NO_NODE};
  std::uint32_t last_child{NO_NODE};
  std::uint32_t previous{NO_NODE};
  std::uint32_t next{NO_NODE};
  std::uint32_t content_offset{0};
  std::uint32_t content_length{0};
  std::uint32_t data{0};
  std::uint32_t extra{0};
  std::uint16_t flags{0};
  NodeType type{NodeType::Document};
  std::uint8_t level{0};
};

inline auto has_flag(const Node &node, const std::uint16_t flag) noexcept
    -> bool {
  return (node.flags & flag) != 0;
}

inline auto set_flag(Node &node, const std::uint16_t flag,
                     const bool value) noexcept -> void {
  node.flags = value ? static_cast<std::uint16_t>(node.flags | flag)
                     : static_cast<std::uint16_t>(node.flags & ~flag);
}

struct ListData {
  std::int32_t start{0};
  std::int32_t marker_offset{0};
  std::int32_t padding{0};
  std::int32_t start_line{0};
  char bullet_character{0};
  char delimiter{0};
  bool ordered{false};
};

struct TableData {
  std::uint32_t alignments_offset{0};
  std::uint32_t columns{0};
  std::int64_t rows{0};
  std::int64_t nonempty_cells{0};
};

struct Reference {
  std::string_view url;
  std::string_view title;
};

// Strings that stay at the same address until the arena is cleared
class StringArena {
public:
  auto clear() -> void {
    if (this->blocks_.empty()) {
      return;
    }

    if (this->blocks_.size() > 1) {
      this->blocks_.erase(this->blocks_.begin() + 1, this->blocks_.end());
    }

    this->cursor_ = this->blocks_.front().data();
    this->remaining_ = this->blocks_.front().size();
  }

  auto allocate(const std::size_t size) -> char * {
    if (this->remaining_ < size) [[unlikely]] {
      const auto capacity{std::max(size, BLOCK_SIZE)};
      this->blocks_.emplace_back(capacity, '\0');
      this->cursor_ = this->blocks_.back().data();
      this->remaining_ = capacity;
    }

    auto *const result{this->cursor_};
    this->cursor_ += size;
    this->remaining_ -= size;
    return result;
  }

  auto store(const std::string_view value) -> std::string_view {
    if (value.empty()) {
      return {};
    }

    auto *const target{this->allocate(value.size())};
    std::memcpy(target, value.data(), value.size());
    return {target, value.size()};
  }

private:
  static constexpr std::size_t BLOCK_SIZE{16384};
  std::vector<std::string> blocks_;
  char *cursor_{nullptr};
  std::size_t remaining_{0};
};

struct Document {
  auto clear() -> void {
    this->nodes.clear();
    this->content.clear();
    this->strings.clear();
    this->lists.clear();
    this->tables.clear();
    this->alignments.clear();
    this->references.clear();
    this->reference_size_limit = 0;
    this->footnote_definition_nodes.clear();
    this->footnote_reference_nodes.clear();
    this->nested_footnote_definitions = false;
    this->nodes.emplace_back().flags = FLAG_OPEN;
  }

  auto create(const NodeType type) -> std::uint32_t {
    const auto index{static_cast<std::uint32_t>(this->nodes.size())};
    this->nodes.emplace_back().type = type;
    return index;
  }

  [[nodiscard]] auto content_of(const Node &node) const -> std::string_view {
    return std::string_view{this->content}.substr(node.content_offset,
                                                  node.content_length);
  }

  auto append_child(const std::uint32_t parent, const std::uint32_t child)
      -> void {
    auto &parent_node{this->nodes[parent]};
    auto &child_node{this->nodes[child]};
    child_node.parent = parent;
    child_node.next = NO_NODE;
    child_node.previous = parent_node.last_child;
    if (parent_node.last_child == NO_NODE) {
      parent_node.first_child = child;
    } else {
      this->nodes[parent_node.last_child].next = child;
    }

    parent_node.last_child = child;
  }

  auto unlink(const std::uint32_t index) -> void {
    auto &node{this->nodes[index]};
    if (node.previous != NO_NODE) {
      this->nodes[node.previous].next = node.next;
    }

    if (node.next != NO_NODE) {
      this->nodes[node.next].previous = node.previous;
    }

    if (node.parent != NO_NODE) {
      auto &parent{this->nodes[node.parent]};
      if (parent.first_child == index) {
        parent.first_child = node.next;
      }

      if (parent.last_child == index) {
        parent.last_child = node.previous;
      }
    }

    node.parent = NO_NODE;
    node.previous = NO_NODE;
    node.next = NO_NODE;
  }

  auto insert_before(const std::uint32_t index, const std::uint32_t sibling)
      -> void {
    this->unlink(sibling);
    auto &node{this->nodes[index]};
    auto &sibling_node{this->nodes[sibling]};
    sibling_node.parent = node.parent;
    sibling_node.next = index;
    sibling_node.previous = node.previous;
    if (node.previous != NO_NODE) {
      this->nodes[node.previous].next = sibling;
    } else if (node.parent != NO_NODE) {
      this->nodes[node.parent].first_child = sibling;
    }

    node.previous = sibling;
  }

  auto insert_after(const std::uint32_t index, const std::uint32_t sibling)
      -> void {
    this->unlink(sibling);
    auto &node{this->nodes[index]};
    auto &sibling_node{this->nodes[sibling]};
    sibling_node.parent = node.parent;
    sibling_node.previous = index;
    sibling_node.next = node.next;
    if (node.next != NO_NODE) {
      this->nodes[node.next].previous = sibling;
    } else if (node.parent != NO_NODE) {
      this->nodes[node.parent].last_child = sibling;
    }

    node.next = sibling;
  }

  std::vector<Node> nodes;
  std::string content;
  StringArena strings;
  std::vector<ListData> lists;
  std::vector<TableData> tables;
  std::vector<std::uint8_t> alignments;
  std::unordered_map<std::string_view, Reference> references;
  std::size_t reference_size_limit{0};
  std::vector<std::uint32_t> footnote_definition_nodes;
  std::vector<std::uint32_t> footnote_reference_nodes;
  bool nested_footnote_definitions{false};
};

} // namespace sourcemeta::core::markdown

#endif
