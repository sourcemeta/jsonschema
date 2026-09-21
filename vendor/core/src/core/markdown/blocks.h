#ifndef SOURCEMETA_CORE_MARKDOWN_BLOCKS_H_
#define SOURCEMETA_CORE_MARKDOWN_BLOCKS_H_

#include <sourcemeta/core/markdown_error.h>
#include <sourcemeta/core/text.h>

#include "characters.h"
#include "document.h"
#include "references.h"
#include "scanners.h"

#include <algorithm> // std::min, std::max
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <cstdint>   // std::int32_t, std::int64_t, std::uint8_t, std::uint32_t
#include <cstring>   // std::memchr
#include <string>    // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core::markdown {

constexpr std::ptrdiff_t TAB_STOP{4};
constexpr std::ptrdiff_t CODE_INDENT{4};
// GFM section 4.10: "If there are a number of cells fewer than the number of
// cells in the header row, empty cells are inserted", which lets a small input
// turn into a huge table, so the conversion throws once a table inserts more
// empty cells than this
constexpr std::int64_t MAXIMUM_AUTOCOMPLETED_CELLS{0x80000};
// The destinations and titles that rendered link references expand to can take
// up to this many times the size of the input or the minimum below, whichever
// is larger, before the conversion throws
constexpr std::size_t REFERENCE_SIZE_FACTOR{16};
constexpr std::size_t MINIMUM_REFERENCE_SIZE_LIMIT{1048576};

struct TableCellSpan {
  std::size_t offset;
  std::size_t length;
};

inline auto is_block_type(const NodeType type) noexcept -> bool {
  return type <= NodeType::TableCell;
}

inline auto is_inline_type(const NodeType type) noexcept -> bool {
  return type > NodeType::TableCell;
}

inline auto accepts_lines(const NodeType type) noexcept -> bool {
  return type == NodeType::Paragraph || type == NodeType::Heading ||
         type == NodeType::CodeBlock;
}

// Whether the content is blank up to its first line ending
inline auto is_blank_until_line_end(const std::string_view content) noexcept
    -> bool {
  for (const auto character : content) {
    if (character == '\r' || character == '\n') {
      return true;
    }

    if (character != ' ' && character != '\t') {
      return false;
    }
  }

  return true;
}

// The position of the next line feed, or the size of the input if there is
// none
inline auto find_line_feed(const std::string_view input,
                           const std::size_t position) noexcept -> std::size_t {
  const auto *const found{static_cast<const char *>(
      std::memchr(input.data() + position, '\n', input.size() - position))};
  return found == nullptr ? input.size()
                          : static_cast<std::size_t>(found - input.data());
}

// A row of the GFM table extension, where a string of several lines resolves
// to the row on its last line, whose offset is reported
inline auto parse_table_row(const std::string_view string,
                            std::vector<TableCellSpan> &cells,
                            std::size_t &last_line_offset) -> bool {
  cells.clear();
  last_line_offset = 0;
  const auto length{string.size()};
  auto offset{scan_table_cell_end(string, 0)};
  bool expect_more_cells{true};
  while (offset < length && expect_more_cells) {
    const auto cell_length{scan_table_cell(string, offset)};
    const auto pipe_length{scan_table_cell_end(string, offset + cell_length)};
    if (cell_length > 0 || pipe_length > 0) {
      cells.push_back({.offset = offset, .length = cell_length});
    }

    offset += cell_length + pipe_length;
    if (pipe_length > 0) {
      continue;
    }

    const auto row_end{scan_table_row_end(string, offset)};
    offset += row_end;
    if (row_end > 0 && offset != length) {
      last_line_offset = offset;
      cells.clear();
      offset += scan_table_cell_end(string, offset);
    } else {
      expect_more_cells = false;
    }
  }

  return offset == length && !cells.empty();
}

// The block structure of GFM section 4 and 5 and of the table and task list
// extensions, following the parsing strategy of the specification appendix
class BlockParser {
public:
  explicit BlockParser(Document &document) : document_{document} {}

  auto parse(const std::string_view input, const std::size_t total_size)
      -> void {
    this->current_ = ROOT_NODE;
    this->line_number_ = 0;
    this->blank_line_matched_everything_ = false;
    this->skip_blank_continuations_ = false;
    this->open_footnote_definitions_ = 0;
    this->last_blank_container_ = NO_NODE;
    const auto size{input.size()};
    const auto has_carriage_return{input.find('\r') != std::string_view::npos};
    std::size_t position{0};
    while (position < size) {
      auto end{size};
      if (has_carriage_return) {
        end = position;
        while (end < size && input[end] != '\n' && input[end] != '\r') {
          ++end;
        }
      } else {
        end = find_line_feed(input, position);
      }

      if (end < size && input[end] == '\n') {
        this->process_line(input.substr(position, end + 1 - position));
        position = end + 1;
        continue;
      }

      this->line_buffer_.assign(input.substr(position, end - position));
      this->line_buffer_.push_back('\n');
      this->process_line(this->line_buffer_);
      position = end;
      if (position < size) {
        ++position;
        if (position < size && input[position] == '\n') {
          ++position;
        }
      }
    }

    while (this->current_ != ROOT_NODE) {
      this->current_ = this->finalize(this->current_);
    }

    this->finalize(ROOT_NODE);
    this->document_.reference_size_limit = std::max(
        total_size * REFERENCE_SIZE_FACTOR, MINIMUM_REFERENCE_SIZE_LIMIT);
  }

private:
  auto node(const std::uint32_t index) -> Node & {
    return this->document_.nodes[index];
  }

  [[nodiscard]] auto peek(const std::ptrdiff_t index) const noexcept -> char {
    return index >= 0 && static_cast<std::size_t>(index) < this->line_.size()
               ? this->line_[static_cast<std::size_t>(index)]
               : '\0';
  }

  [[nodiscard]] auto line_size() const noexcept -> std::ptrdiff_t {
    return static_cast<std::ptrdiff_t>(this->line_.size());
  }

  auto process_line(const std::string_view line) -> void {
    this->line_ = line;
    this->line_end_ = this->line_size();
    this->offset_ = 0;
    this->column_ = 0;
    this->first_nonspace_ = 0;
    this->first_nonspace_column_ = 0;
    this->thematic_break_kill_position_ = 0;
    this->indent_ = 0;
    this->blank_ = false;
    this->partially_consumed_tab_ = false;
    if (this->line_number_ == 0 && line.starts_with("\xEF\xBB\xBF")) {
      this->offset_ += 3;
    }

    ++this->line_number_;
    const auto starting_tip{this->current_};
    const auto starting_nodes{this->document_.nodes.size()};
    const auto starting_offset{static_cast<std::size_t>(this->offset_)};
    // An open footnote definition only continues on an unindented blank line
    // when the line is empty, and an item without blocks does not continue on
    // a blank line at all
    this->skip_blank_continuations_ =
        this->blank_line_matched_everything_ &&
        is_blank_until_line_end(line.substr(starting_offset)) &&
        (this->open_footnote_definitions_ == 0 || this->peek(0) == '\n' ||
         (this->peek(0) == '\r' && this->peek(1) == '\n')) &&
        !(this->node(starting_tip).type == NodeType::Item &&
          this->node(starting_tip).first_child == NO_NODE);
    this->blank_line_matched_everything_ = false;
    bool all_matched{true};
    const auto last_matched{this->check_open_blocks(all_matched)};
    if (last_matched == NO_NODE) {
      return;
    }

    auto container{last_matched};
    this->open_new_blocks(container, all_matched);
    this->add_text_to_container(container, last_matched);
    this->blank_line_matched_everything_ =
        all_matched && last_matched == starting_tip &&
        this->current_ == starting_tip &&
        this->document_.nodes.size() == starting_nodes && this->blank_ &&
        is_blank_until_line_end(line.substr(starting_offset));
  }

  auto find_first_nonspace() -> void {
    auto chars_to_tab{TAB_STOP - (this->column_ % TAB_STOP)};
    if (this->first_nonspace_ <= this->offset_) {
      this->first_nonspace_ = this->offset_;
      this->first_nonspace_column_ = this->column_;
      while (true) {
        const auto character{this->peek(this->first_nonspace_)};
        if (character == ' ') {
          ++this->first_nonspace_;
          ++this->first_nonspace_column_;
          --chars_to_tab;
          if (chars_to_tab == 0) {
            chars_to_tab = TAB_STOP;
          }
        } else if (character == '\t') {
          ++this->first_nonspace_;
          this->first_nonspace_column_ += chars_to_tab;
          chars_to_tab = TAB_STOP;
        } else {
          break;
        }
      }
    }

    this->indent_ = this->first_nonspace_column_ - this->column_;
    this->blank_ = is_line_end(this->peek(this->first_nonspace_));
  }

  // Advance by a number of bytes, or by a number of columns where a tab might
  // only be partially consumed
  auto advance_offset(std::ptrdiff_t count, const bool columns) -> void {
    // Without tabs, advancing by bytes also advances by the same columns
    if (!columns && count > 0 && this->offset_ + count <= this->line_size() &&
        std::memchr(this->line_.data() + this->offset_, '\t',
                    static_cast<std::size_t>(count)) == nullptr) {
      this->partially_consumed_tab_ = false;
      this->offset_ += count;
      this->column_ += count;
      return;
    }

    while (count > 0) {
      const auto character{this->peek(this->offset_)};
      if (character == '\0') {
        break;
      }

      if (character != '\t') {
        this->partially_consumed_tab_ = false;
        ++this->offset_;
        ++this->column_;
        --count;
        continue;
      }

      const auto chars_to_tab{TAB_STOP - (this->column_ % TAB_STOP)};
      if (columns) {
        this->partially_consumed_tab_ = chars_to_tab > count;
        const auto chars_to_advance{std::min(count, chars_to_tab)};
        this->column_ += chars_to_advance;
        this->offset_ += this->partially_consumed_tab_ ? 0 : 1;
        count -= chars_to_advance;
      } else {
        this->partially_consumed_tab_ = false;
        this->column_ += chars_to_tab;
        ++this->offset_;
        --count;
      }
    }
  }

  auto advance_to_line_end() -> void {
    this->advance_offset(this->line_size() - 1 - this->offset_, false);
  }

  auto advance_to_first_nonspace() -> void {
    this->advance_offset(this->first_nonspace_ - this->offset_, false);
  }

  auto last_child_is_open(const std::uint32_t index) -> bool {
    const auto last_child{this->node(index).last_child};
    return last_child != NO_NODE && has_flag(this->node(last_child), FLAG_OPEN);
  }

  auto can_contain(const std::uint32_t parent, const NodeType child) -> bool {
    const auto &parent_node{this->node(parent)};
    switch (parent_node.type) {
      case NodeType::Document:
      case NodeType::BlockQuote:
      case NodeType::FootnoteDefinition:
        return is_block_type(child) && child != NodeType::Item;
      case NodeType::Item:
        return has_flag(parent_node, FLAG_TASK) ||
               (is_block_type(child) && child != NodeType::Item);
      case NodeType::List:
        return child == NodeType::Item;
      case NodeType::Table:
        return child == NodeType::TableRow;
      case NodeType::TableRow:
        return child == NodeType::TableCell;
      case NodeType::Paragraph:
      case NodeType::Heading:
        return is_inline_type(child);
      case NodeType::CodeBlock:
      case NodeType::HTMLBlock:
      case NodeType::ThematicBreak:
      case NodeType::TableCell:
      case NodeType::Text:
      case NodeType::SoftBreak:
      case NodeType::LineBreak:
      case NodeType::Code:
      case NodeType::HTMLInline:
      case NodeType::Emphasis:
      case NodeType::Strong:
      case NodeType::Link:
      case NodeType::Image:
      case NodeType::FootnoteReference:
      case NodeType::Strikethrough:
        return false;
    }

    return false;
  }

  auto add_child(std::uint32_t parent, const NodeType type) -> std::uint32_t {
    while (!this->can_contain(parent, type)) {
      parent = this->finalize(parent);
    }

    const auto child{this->document_.create(type)};
    this->node(child).flags = FLAG_OPEN;
    this->document_.append_child(parent, child);
    if (type == NodeType::FootnoteDefinition) {
      ++this->open_footnote_definitions_;
    }

    return child;
  }

  auto add_line(const std::uint32_t index) -> void {
    auto &content{this->document_.content};
    auto &target{this->node(index)};
    if (target.content_length == 0) {
      target.content_offset = static_cast<std::uint32_t>(content.size());
    } else if (target.content_offset + target.content_length !=
               content.size()) {
      this->relocation_buffer_.assign(this->document_.content_of(target));
      target.content_offset = static_cast<std::uint32_t>(content.size());
      content.append(this->relocation_buffer_);
    }

    const auto before{content.size()};
    if (this->partially_consumed_tab_) {
      ++this->offset_;
      const auto chars_to_tab{TAB_STOP - (this->column_ % TAB_STOP)};
      content.append(static_cast<std::size_t>(chars_to_tab), ' ');
    }

    if (this->line_end_ > this->offset_) {
      content.append(this->line_.substr(
          static_cast<std::size_t>(this->offset_),
          static_cast<std::size_t>(this->line_end_ - this->offset_)));
    }

    target.content_length +=
        static_cast<std::uint32_t>(content.size() - before);
  }

  auto resolve_reference_definitions(const std::uint32_t index) -> bool {
    const auto content{this->document_.content_of(this->node(index))};
    std::size_t position{0};
    while (position < content.size() && content[position] == '[') {
      const auto consumed{
          parse_reference_definition(this->document_, content.substr(position),
                                     this->label_buffer_, this->value_buffer_)};
      if (consumed == 0) {
        break;
      }

      position += consumed;
    }

    auto &target{this->node(index)};
    target.content_offset += static_cast<std::uint32_t>(position);
    target.content_length -= static_cast<std::uint32_t>(position);
    return !is_blank_until_line_end(content.substr(position));
  }

  auto ends_with_blank_line(std::uint32_t index) -> bool {
    while (true) {
      auto &current{this->node(index)};
      if (has_flag(current, FLAG_LAST_LINE_CHECKED)) {
        return has_flag(current, FLAG_LAST_LINE_BLANK);
      }

      set_flag(current, FLAG_LAST_LINE_CHECKED, true);
      if ((current.type == NodeType::List || current.type == NodeType::Item) &&
          current.last_child != NO_NODE) {
        index = current.last_child;
        continue;
      }

      return has_flag(current, FLAG_LAST_LINE_BLANK);
    }
  }

  auto finalize_list(const std::uint32_t index) -> void {
    bool tight{true};
    for (auto item{this->node(index).first_child}; item != NO_NODE && tight;
         item = this->node(item).next) {
      if (has_flag(this->node(item), FLAG_LAST_LINE_BLANK) &&
          this->node(item).next != NO_NODE) {
        tight = false;
        break;
      }

      for (auto child{this->node(item).first_child}; child != NO_NODE;
           child = this->node(child).next) {
        if ((this->node(item).next != NO_NODE ||
             this->node(child).next != NO_NODE) &&
            this->ends_with_blank_line(child)) {
          tight = false;
          break;
        }
      }
    }

    set_flag(this->node(index), FLAG_TIGHT, tight);
  }

  auto finalize_code_block(const std::uint32_t index) -> void {
    auto &target{this->node(index)};
    const auto content{this->document_.content_of(target)};
    if (!has_flag(target, FLAG_FENCED)) {
      auto end{content.size()};
      while (end > 0 && (content[end - 1] == ' ' || content[end - 1] == '\t' ||
                         is_line_end(content[end - 1]))) {
        --end;
      }

      if (end == 0) {
        target.content_length = 0;
        target.literal = "\n";
        return;
      }

      const auto line_end{content.find_first_of("\r\n", end - 1)};
      if (line_end == std::string_view::npos) {
        this->value_buffer_.assign(content);
        this->value_buffer_.push_back('\n');
        target.literal = this->document_.strings.store(this->value_buffer_);
        return;
      }

      if (content[line_end] == '\n') {
        target.content_length = static_cast<std::uint32_t>(line_end + 1);
        return;
      }

      this->value_buffer_.assign(content.substr(0, line_end));
      this->value_buffer_.push_back('\n');
      target.literal = this->document_.strings.store(this->value_buffer_);

      return;
    }

    auto position{content.find_first_of("\r\n")};
    if (position == std::string_view::npos) {
      position = content.size();
    }

    this->value_buffer_.clear();
    decode_escapes_and_references(this->value_buffer_,
                                  content.substr(0, position));
    target.title = this->document_.strings.store(
        sourcemeta::core::trim(this->value_buffer_, is_space));
    if (character_at(content, position) == '\r') {
      ++position;
    }

    if (character_at(content, position) == '\n') {
      ++position;
    }

    position = std::min(position, content.size());
    target.content_offset += static_cast<std::uint32_t>(position);
    target.content_length -= static_cast<std::uint32_t>(position);
  }

  auto finalize(const std::uint32_t index) -> std::uint32_t {
    const auto parent{this->node(index).parent};
    set_flag(this->node(index), FLAG_OPEN, false);
    const auto type{this->node(index).type};
    if (type == NodeType::FootnoteDefinition) {
      --this->open_footnote_definitions_;
    }

    if (type == NodeType::Paragraph) {
      if (!this->resolve_reference_definitions(index)) {
        this->document_.unlink(index);
      }
    } else if (type == NodeType::CodeBlock) {
      this->finalize_code_block(index);
    } else if (type == NodeType::List) {
      this->finalize_list(index);
    }

    return parent;
  }

  auto parse_block_quote_prefix() -> bool {
    if (this->indent_ > 3 || this->peek(this->first_nonspace_) != '>') {
      return false;
    }

    this->advance_offset(this->indent_ + 1, true);
    if (is_space_or_tab(this->peek(this->offset_))) {
      this->advance_offset(1, true);
    }

    return true;
  }

  auto parse_footnote_definition_prefix() -> bool {
    if (this->indent_ >= CODE_INDENT) {
      this->advance_offset(CODE_INDENT, true);
      return true;
    }

    return this->peek(0) == '\n' ||
           (this->peek(0) == '\r' && this->peek(1) == '\n');
  }

  auto parse_item_prefix(const std::uint32_t index) -> bool {
    const auto &data{this->document_.lists[this->node(index).data]};
    const auto width{static_cast<std::ptrdiff_t>(data.marker_offset) +
                     static_cast<std::ptrdiff_t>(data.padding)};
    if (this->indent_ >= width) {
      this->advance_offset(width, true);
      return true;
    }

    if (this->blank_ && this->node(index).first_child != NO_NODE) {
      this->advance_to_first_nonspace();
      return true;
    }

    return false;
  }

  auto parse_code_block_prefix(const std::uint32_t index, bool &should_continue)
      -> bool {
    if (!has_flag(this->node(index), FLAG_FENCED)) {
      if (this->indent_ >= CODE_INDENT) {
        this->advance_offset(CODE_INDENT, true);
        return true;
      }

      if (this->blank_) {
        this->advance_to_first_nonspace();
        return true;
      }

      return false;
    }

    std::size_t matched{0};
    if (this->indent_ <= 3 &&
        static_cast<unsigned char>(this->peek(this->first_nonspace_)) ==
            this->node(index).data) {
      matched = scan_close_code_fence(
          this->line_, static_cast<std::size_t>(this->first_nonspace_));
    }

    if (matched > 0 && matched >= this->node(index).level) {
      should_continue = false;
      this->advance_offset(static_cast<std::ptrdiff_t>(matched), false);
      this->current_ = this->finalize(index);
      return false;
    }

    auto remaining{static_cast<std::ptrdiff_t>(this->node(index).extra)};
    while (remaining > 0 && is_space_or_tab(this->peek(this->offset_))) {
      this->advance_offset(1, true);
      --remaining;
    }

    return true;
  }

  auto table_row_matches() -> bool {
    std::size_t last_line_offset{0};
    this->cached_row_valid_ = parse_table_row(
        this->line_.substr(static_cast<std::size_t>(this->first_nonspace_)),
        this->cells_, last_line_offset);
    this->cached_row_line_ = this->line_number_;
    this->cached_row_offset_ = this->first_nonspace_;
    return this->cached_row_valid_;
  }

  auto continues_container(const std::uint32_t index, bool &should_continue)
      -> bool {
    switch (this->node(index).type) {
      case NodeType::BlockQuote:
        return this->parse_block_quote_prefix();
      case NodeType::Item:
        return this->parse_item_prefix(index);
      case NodeType::CodeBlock:
        return this->parse_code_block_prefix(index, should_continue);
      case NodeType::HTMLBlock:
        return this->node(index).level <= 5 || !this->blank_;
      case NodeType::Paragraph:
        return !this->blank_;
      case NodeType::FootnoteDefinition:
        return this->parse_footnote_definition_prefix();
      case NodeType::Table:
        return this->table_row_matches();
      // GFM section 4.1 makes a thematic break out of a single line, so it does
      // not continue, and a blank line after it reaches the blocks around it
      case NodeType::Heading:
      case NodeType::ThematicBreak:
      case NodeType::TableRow:
      case NodeType::TableCell:
        return false;
      case NodeType::Document:
      case NodeType::List:
      case NodeType::Text:
      case NodeType::SoftBreak:
      case NodeType::LineBreak:
      case NodeType::Code:
      case NodeType::HTMLInline:
      case NodeType::Emphasis:
      case NodeType::Strong:
      case NodeType::Link:
      case NodeType::Image:
      case NodeType::FootnoteReference:
      case NodeType::Strikethrough:
        return true;
    }

    return true;
  }

  auto check_open_blocks(bool &all_matched) -> std::uint32_t {
    bool should_continue{true};
    all_matched = true;
    auto container{ROOT_NODE};
    while (this->last_child_is_open(container)) {
      container = this->node(container).last_child;
      this->find_first_nonspace();
      // Once a blank line has no indentation left, every remaining open block
      // continues without consuming anything if the previous line was blank,
      // matched every open block, and changed nothing, which spares walking
      // through deeply nested blocks for every blank line
      if (this->skip_blank_continuations_ && this->blank_ &&
          this->indent_ == 0) {
        return this->current_;
      }

      if (!this->continues_container(container, should_continue)) {
        all_matched = false;
        break;
      }
    }

    if (!all_matched) {
      container = this->node(container).parent;
    }

    return should_continue ? container : NO_NODE;
  }

  auto scan_thematic_break(const std::ptrdiff_t position) -> std::ptrdiff_t {
    const auto marker{this->peek(position)};
    if (marker != '*' && marker != '_' && marker != '-') {
      this->thematic_break_kill_position_ = position;
      return 0;
    }

    std::ptrdiff_t count{1};
    auto index{position};
    char next{'\0'};
    while (true) {
      ++index;
      next = this->peek(index);
      if (next == '\0') {
        break;
      }

      if (next == marker) {
        ++count;
      } else if (next != ' ' && next != '\t') {
        break;
      }
    }

    if (count >= 3 && (next == '\r' || next == '\n')) {
      return index - position + 1;
    }

    this->thematic_break_kill_position_ = index;
    return 0;
  }

  auto parse_list_marker(const std::ptrdiff_t position,
                         const bool interrupts_paragraph, ListData &data)
      -> std::ptrdiff_t {
    auto cursor{position};
    const auto marker{this->peek(cursor)};
    if (marker == '*' || marker == '-' || marker == '+') {
      ++cursor;
      if (!is_space(this->peek(cursor))) {
        return 0;
      }

      if (interrupts_paragraph) {
        auto index{cursor};
        while (is_space_or_tab(this->peek(index))) {
          ++index;
        }

        if (this->peek(index) == '\n') {
          return 0;
        }
      }

      data = ListData{};
      data.bullet_character = marker;
      return cursor - position;
    }

    if (!sourcemeta::core::is_digit(marker)) {
      return 0;
    }

    std::int32_t start{0};
    std::int32_t digits{0};
    while (true) {
      start = (10 * start) + (this->peek(cursor) - '0');
      ++cursor;
      ++digits;
      if (digits >= 9 || !sourcemeta::core::is_digit(this->peek(cursor))) {
        break;
      }
    }

    if (interrupts_paragraph && start != 1) {
      return 0;
    }

    const auto delimiter{this->peek(cursor)};
    if (delimiter != '.' && delimiter != ')') {
      return 0;
    }

    ++cursor;
    if (!is_space(this->peek(cursor))) {
      return 0;
    }

    if (interrupts_paragraph) {
      auto index{cursor};
      while (is_space_or_tab(this->peek(index))) {
        ++index;
      }

      if (is_line_end(this->peek(index))) {
        return 0;
      }
    }

    data = ListData{};
    data.start = start;
    data.delimiter = delimiter;
    data.ordered = true;
    return cursor - position;
  }

  auto append_table_cell_content(const std::uint32_t cell,
                                 const std::string_view raw) -> void {
    const auto text{sourcemeta::core::trim(raw, is_space)};
    auto &content{this->document_.content};
    auto &target{this->node(cell)};
    target.content_offset = static_cast<std::uint32_t>(content.size());
    std::size_t run_start{0};
    for (std::size_t index{0}; index + 1 < text.size(); ++index) {
      if (text[index] == '\\' && text[index + 1] == '|') {
        content.append(text.substr(run_start, index - run_start));
        run_start = index + 1;
      }
    }

    content.append(text.substr(run_start));

    target.content_length =
        static_cast<std::uint32_t>(content.size() - target.content_offset);
  }

  auto try_opening_table_header(const std::uint32_t paragraph)
      -> std::uint32_t {
    this->cached_row_line_ = -1;
    if (has_flag(this->node(paragraph), FLAG_TABLE_VISITED)) {
      return paragraph;
    }

    const auto nonspace{static_cast<std::size_t>(this->first_nonspace_)};
    std::size_t last_line_offset{0};
    if (scan_table_start(this->line_, nonspace) == 0 ||
        !parse_table_row(this->line_.substr(nonspace), this->cells_,
                         last_line_offset)) {
      return paragraph;
    }

    this->header_buffer_.assign(
        this->document_.content_of(this->node(paragraph)));
    if (!parse_table_row(this->header_buffer_, this->header_cells_,
                         last_line_offset) ||
        this->header_cells_.size() != this->cells_.size()) {
      set_flag(this->node(paragraph), FLAG_TABLE_VISITED, true);
      return paragraph;
    }

    if (last_line_offset > 0) {
      const auto preceding{this->document_.create(NodeType::Paragraph)};
      this->append_table_cell_content(
          preceding,
          std::string_view{this->header_buffer_}.substr(0, last_line_offset));
      this->document_.insert_before(paragraph, preceding);
    }

    const auto table_index{
        static_cast<std::uint32_t>(this->document_.tables.size())};
    this->document_.tables.push_back(
        {.alignments_offset =
             static_cast<std::uint32_t>(this->document_.alignments.size()),
         .columns = static_cast<std::uint32_t>(this->header_cells_.size()),
         .rows = 1,
         .nonempty_cells =
             static_cast<std::int64_t>(this->header_cells_.size())});
    for (const auto &cell : this->cells_) {
      const auto marker{sourcemeta::core::trim(
          this->line_.substr(nonspace + cell.offset, cell.length), is_space)};
      const auto left{!marker.empty() && marker.front() == ':'};
      const auto right{!marker.empty() && marker.back() == ':'};
      std::uint8_t alignment{0};
      if (left && right) {
        alignment = 'c';
      } else if (left) {
        alignment = 'l';
      } else if (right) {
        alignment = 'r';
      }

      this->document_.alignments.push_back(alignment);
    }

    auto &table{this->node(paragraph)};
    table.type = NodeType::Table;
    table.data = table_index;
    const auto row{this->add_child(paragraph, NodeType::TableRow)};
    set_flag(this->node(row), FLAG_HEADER, true);
    for (std::size_t index{0}; index < this->header_cells_.size(); ++index) {
      const auto cell{this->add_child(row, NodeType::TableCell)};
      this->node(cell).data = static_cast<std::uint32_t>(index);
      this->append_table_cell_content(
          cell, std::string_view{this->header_buffer_}.substr(
                    this->header_cells_[index].offset,
                    this->header_cells_[index].length));
    }

    this->advance_to_line_end();
    return paragraph;
  }

  auto try_opening_table_row(const std::uint32_t table) -> std::uint32_t {
    if (this->blank_) {
      return NO_NODE;
    }

    const auto table_index{this->node(table).data};

    const auto row{this->add_child(table, NodeType::TableRow)};
    const auto nonspace{static_cast<std::size_t>(this->first_nonspace_)};
    std::size_t last_line_offset{0};
    const auto cached{this->cached_row_line_ == this->line_number_ &&
                      this->cached_row_offset_ == this->first_nonspace_};
    if (!(cached ? this->cached_row_valid_
                 : parse_table_row(this->line_.substr(nonspace), this->cells_,
                                   last_line_offset))) {
      this->document_.unlink(row);
      return NO_NODE;
    }

    const auto columns{this->document_.tables[table_index].columns};
    std::size_t index{0};
    for (; index < this->cells_.size() && index < columns; ++index) {
      const auto cell{this->add_child(row, NodeType::TableCell)};
      this->node(cell).data = static_cast<std::uint32_t>(index);
      this->append_table_cell_content(
          cell, this->line_.substr(nonspace + this->cells_[index].offset,
                                   this->cells_[index].length));
    }

    auto &data{this->document_.tables[table_index]};
    data.rows += 1;
    data.nonempty_cells += static_cast<std::int64_t>(index);
    if ((static_cast<std::int64_t>(data.columns) * data.rows) -
            data.nonempty_cells >
        MAXIMUM_AUTOCOMPLETED_CELLS) {
      throw sourcemeta::core::MarkdownError{
          "The table inserts more empty cells than its bound"};
    }
    for (; index < columns; ++index) {
      const auto cell{this->add_child(row, NodeType::TableCell)};
      this->node(cell).data = static_cast<std::uint32_t>(index);
    }

    this->advance_to_line_end();
    return row;
  }

  // A list item whose first block is a paragraph that starts with a task list
  // item marker of GFM section 5.3, which the item can only get while it has
  // no block yet
  auto open_task_list_item(const std::uint32_t item) -> void {
    if (this->node(item).type != NodeType::Item ||
        this->node(item).first_child != NO_NODE ||
        !scan_task_list_marker(
            this->line_, static_cast<std::size_t>(this->first_nonspace_))) {
      return;
    }

    set_flag(this->node(item), FLAG_TASK, true);
    // GFM section 5.3: "If the character between the brackets is a whitespace
    // character, the checkbox is unchecked. Otherwise, the checkbox is checked"
    const auto state{this->peek(this->first_nonspace_ + 1)};
    set_flag(this->node(item), FLAG_CHECKED, state == 'x' || state == 'X');
    this->advance_offset(this->first_nonspace_ + 3 - this->offset_, false);
  }

  auto register_footnote_definition(const std::uint32_t definition) -> void {
    for (auto ancestor{this->node(definition).parent}; ancestor != NO_NODE;
         ancestor = this->node(ancestor).parent) {
      if (this->node(ancestor).type == NodeType::FootnoteDefinition) {
        this->document_.nested_footnote_definitions = true;
        break;
      }
    }

    this->document_.footnote_definition_nodes.push_back(definition);
  }

  auto open_block(std::uint32_t &container, const NodeType container_type,
                  const bool all_matched, const bool maybe_lazy) -> bool {
    const auto indented{this->indent_ >= CODE_INDENT};
    const auto nonspace{static_cast<std::size_t>(this->first_nonspace_)};
    const auto character{this->peek(this->first_nonspace_)};
    if (!indented && this->peek(this->first_nonspace_) == '>') {
      this->advance_offset(this->first_nonspace_ + 1 - this->offset_, false);
      if (is_space_or_tab(this->peek(this->offset_))) {
        this->advance_offset(1, true);
      }

      container = this->add_child(container, NodeType::BlockQuote);
      return true;
    }

    if (!indented && character == '#') {
      const auto matched{scan_atx_heading_start(this->line_, nonspace)};
      if (matched > 0) {
        this->advance_offset(this->first_nonspace_ +
                                 static_cast<std::ptrdiff_t>(matched) -
                                 this->offset_,
                             false);
        container = this->add_child(container, NodeType::Heading);
        auto hash{this->line_.find('#', nonspace)};
        std::uint8_t level{0};
        while (hash < this->line_.size() && this->line_[hash] == '#') {
          ++level;
          ++hash;
        }

        this->node(container).level = level;
        return true;
      }
    }

    if (!indented && (character == '`' || character == '~')) {
      const auto matched{scan_open_code_fence(this->line_, nonspace)};
      if (matched > 0) {
        container = this->add_child(container, NodeType::CodeBlock);
        auto &code{this->node(container)};
        set_flag(code, FLAG_FENCED, true);
        code.data =
            static_cast<unsigned char>(this->peek(this->first_nonspace_));
        code.level =
            static_cast<std::uint8_t>(std::min<std::size_t>(matched, 255));
        code.extra =
            static_cast<std::uint32_t>(this->first_nonspace_ - this->offset_);
        this->advance_offset(this->first_nonspace_ +
                                 static_cast<std::ptrdiff_t>(matched) -
                                 this->offset_,
                             false);
        return true;
      }
    }

    if (!indented && character == '<') {
      auto condition{scan_html_block_start(this->line_, nonspace)};
      if (condition == 0 && container_type != NodeType::Paragraph &&
          scan_html_block_start_7(this->line_, nonspace)) {
        condition = 7;
      }

      if (condition > 0) {
        container = this->add_child(container, NodeType::HTMLBlock);
        this->node(container).level = static_cast<std::uint8_t>(condition);
        return true;
      }
    }

    if (!indented && container_type == NodeType::Paragraph &&
        (character == '=' || character == '-')) {
      const auto level{scan_setext_heading_line(this->line_, nonspace)};
      if (level > 0) {
        if (this->resolve_reference_definitions(container)) {
          auto &heading{this->node(container)};
          heading.type = NodeType::Heading;
          heading.level = level;
          set_flag(heading, FLAG_SETEXT, true);
          this->advance_to_line_end();
        }

        return true;
      }
    }

    if (!indented &&
        (character == '*' || character == '_' || character == '-') &&
        !(container_type == NodeType::Paragraph && !all_matched) &&
        this->thematic_break_kill_position_ <= this->first_nonspace_ &&
        this->scan_thematic_break(this->first_nonspace_) > 0) {
      container = this->add_child(container, NodeType::ThematicBreak);
      this->advance_to_line_end();
      return true;
    }

    if (!indented && character == '[') {
      const auto matched{scan_footnote_definition(this->line_, nonspace)};
      if (matched > 0) {
        auto label{this->line_.substr(nonspace + 2, matched - 2)};
        while (!label.empty() && label.back() != ']') {
          label.remove_suffix(1);
        }

        if (!label.empty()) {
          label.remove_suffix(1);
        }

        const auto stored{this->document_.strings.store(label)};
        this->advance_offset(this->first_nonspace_ +
                                 static_cast<std::ptrdiff_t>(matched) -
                                 this->offset_,
                             false);
        container = this->add_child(container, NodeType::FootnoteDefinition);
        this->register_footnote_definition(container);
        this->node(container).literal = stored;
        return true;
      }
    }

    if ((character == '*' || character == '-' || character == '+' ||
         sourcemeta::core::is_digit(character)) &&
        (!indented || container_type == NodeType::List) &&
        this->indent_ < CODE_INDENT) {
      ListData data{};
      const auto matched{this->parse_list_marker(
          this->first_nonspace_, container_type == NodeType::Paragraph, data)};
      if (matched > 0) {
        this->advance_offset(this->first_nonspace_ + matched - this->offset_,
                             false);
        const auto saved_partially_consumed_tab{this->partially_consumed_tab_};
        const auto saved_offset{this->offset_};
        const auto saved_column{this->column_};
        while (this->column_ - saved_column <= 5 &&
               is_space_or_tab(this->peek(this->offset_))) {
          this->advance_offset(1, true);
        }

        const auto spaces{this->column_ - saved_column};
        if (spaces >= 5 || spaces < 1 ||
            is_line_end(this->peek(this->offset_))) {
          data.padding = static_cast<std::int32_t>(matched + 1);
          this->offset_ = saved_offset;
          this->column_ = saved_column;
          this->partially_consumed_tab_ = saved_partially_consumed_tab;
          if (spaces > 0) {
            this->advance_offset(1, true);
          }
        } else {
          data.padding = static_cast<std::int32_t>(matched + spaces);
        }

        data.marker_offset = static_cast<std::int32_t>(this->indent_);
        auto &lists{this->document_.lists};
        if (container_type != NodeType::List ||
            !lists_match(lists[this->node(container).data], data)) {
          container = this->add_child(container, NodeType::List);
          this->node(container).data = static_cast<std::uint32_t>(lists.size());
          lists.push_back(data);
        }

        data.start_line = this->line_number_;
        container = this->add_child(container, NodeType::Item);
        this->node(container).data = static_cast<std::uint32_t>(lists.size());
        lists.push_back(data);
        return true;
      }
    }

    if (indented && !maybe_lazy && !this->blank_) {
      this->advance_offset(CODE_INDENT, true);
      container = this->add_child(container, NodeType::CodeBlock);
      return true;
    }

    if (!indented && container_type == NodeType::Paragraph) {
      if (character == '|' || character == ':' || character == '-' ||
          character == '\v' || character == '\f') {
        container = this->try_opening_table_header(container);
      }
      return true;
    }

    if (!indented && container_type == NodeType::Table) {
      const auto row{this->try_opening_table_row(container)};
      if (row != NO_NODE) {
        container = row;
        return true;
      }
    }

    this->open_task_list_item(container);
    return false;
  }

  static auto lists_match(const ListData &list, const ListData &item) noexcept
      -> bool {
    return list.ordered == item.ordered && list.delimiter == item.delimiter &&
           list.bullet_character == item.bullet_character;
  }

  auto open_new_blocks(std::uint32_t &container, const bool all_matched)
      -> void {
    bool maybe_lazy{this->node(this->current_).type == NodeType::Paragraph};
    auto container_type{this->node(container).type};
    while (container_type != NodeType::CodeBlock &&
           container_type != NodeType::HTMLBlock) {
      this->find_first_nonspace();
      if (!this->open_block(container, container_type, all_matched,
                            maybe_lazy)) {
        break;
      }

      container_type = this->node(container).type;
      if (accepts_lines(container_type)) {
        break;
      }

      maybe_lazy = false;
    }
  }

  auto chop_trailing_hashes() -> void {
    auto length{this->line_end_};
    while (length > 0 && is_space(this->peek(length - 1))) {
      --length;
    }

    auto position{length - 1};
    while (position >= 0 && this->peek(position) == '#') {
      --position;
    }

    if (position != length - 1 && position >= 0 &&
        is_space_or_tab(this->peek(position))) {
      length = position;
      while (length > 0 && is_space(this->peek(length - 1))) {
        --length;
      }
    }

    this->line_end_ = length;
  }

  auto add_text_to_container(std::uint32_t container,
                             const std::uint32_t last_matched) -> void {
    this->find_first_nonspace();
    if (this->blank_ && this->node(container).last_child != NO_NODE) {
      set_flag(this->node(this->node(container).last_child),
               FLAG_LAST_LINE_BLANK, true);
    }

    const auto container_type{this->node(container).type};
    // A table only contains the lines of its rows, as GFM section 4.10 says
    // that "The table is broken at the first empty line", so it never ends with
    // a blank line, even when its delimiter row consumed the rest of its line
    const auto last_line_blank{
        this->blank_ && container_type != NodeType::BlockQuote &&
        container_type != NodeType::Heading &&
        container_type != NodeType::ThematicBreak &&
        container_type != NodeType::Table &&
        container_type != NodeType::TableRow &&
        !(container_type == NodeType::CodeBlock &&
          has_flag(this->node(container), FLAG_FENCED)) &&
        !(container_type == NodeType::Item &&
          this->node(container).first_child == NO_NODE &&
          this->document_.lists[this->node(container).data].start_line ==
              this->line_number_)};
    // The ancestors of the container do not end with a blank line. The only
    // open block that can is the container of the previous line, which is the
    // deepest open block when this line starts, so it is an ancestor exactly
    // when this line matched it and opened blocks under it
    const auto previous_blank{this->last_blank_container_};
    if (previous_blank != NO_NODE && previous_blank == last_matched &&
        previous_blank != container &&
        has_flag(this->node(previous_blank), FLAG_OPEN)) {
      set_flag(this->node(previous_blank), FLAG_LAST_LINE_BLANK, false);
    }

    set_flag(this->node(container), FLAG_LAST_LINE_BLANK, last_line_blank);
    this->last_blank_container_ = last_line_blank ? container : NO_NODE;

    if (this->current_ != last_matched && container == last_matched &&
        !this->blank_ &&
        this->node(this->current_).type == NodeType::Paragraph) {
      this->add_line(this->current_);
      return;
    }

    while (this->current_ != last_matched) {
      this->current_ = this->finalize(this->current_);
    }

    if (container_type == NodeType::CodeBlock) {
      this->add_line(container);
    } else if (container_type == NodeType::HTMLBlock) {
      this->add_line(container);
      const auto condition{this->node(container).level};
      if (condition <= 5 &&
          scan_html_block_end(this->line_,
                              static_cast<std::size_t>(this->first_nonspace_),
                              condition)) {
        container = this->finalize(container);
      }
    } else if (this->blank_) {
      // Blank lines only separate blocks
    } else if (accepts_lines(container_type)) {
      if (container_type == NodeType::Heading &&
          !has_flag(this->node(container), FLAG_SETEXT)) {
        this->chop_trailing_hashes();
      }

      this->advance_to_first_nonspace();
      this->add_line(container);
    } else {
      container = this->add_child(container, NodeType::Paragraph);
      this->advance_to_first_nonspace();
      this->add_line(container);
    }

    this->current_ = container;
  }

  Document &document_;
  std::string_view line_;
  std::ptrdiff_t line_end_{0};
  std::string line_buffer_;
  std::string relocation_buffer_;
  std::string label_buffer_;
  std::string value_buffer_;
  std::string header_buffer_;
  std::vector<TableCellSpan> cells_;
  std::vector<TableCellSpan> header_cells_;
  std::int32_t cached_row_line_{-1};
  std::ptrdiff_t cached_row_offset_{0};
  bool cached_row_valid_{false};
  std::uint32_t current_{ROOT_NODE};
  std::int32_t line_number_{0};
  std::ptrdiff_t offset_{0};
  std::ptrdiff_t column_{0};
  std::ptrdiff_t first_nonspace_{0};
  std::ptrdiff_t first_nonspace_column_{0};
  std::ptrdiff_t thematic_break_kill_position_{0};
  std::ptrdiff_t indent_{0};
  bool blank_{false};
  bool partially_consumed_tab_{false};
  bool blank_line_matched_everything_{false};
  bool skip_blank_continuations_{false};
  std::size_t open_footnote_definitions_{0};
  std::uint32_t last_blank_container_{NO_NODE};
};

} // namespace sourcemeta::core::markdown

#endif
