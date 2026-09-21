#ifndef SOURCEMETA_CORE_MARKDOWN_INLINES_H_
#define SOURCEMETA_CORE_MARKDOWN_INLINES_H_

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include "characters.h"
#include "document.h"
#include "references.h"
#include "scanners.h"

#include <algorithm>   // std::min
#include <array>       // std::array
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdint>     // std::uint8_t, std::uint32_t
#include <cstring>     // std::memchr
#include <functional>  // std::less
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace sourcemeta::core::markdown {

constexpr std::uint32_t NO_DELIMITER{0xFFFFFFFF};

constexpr std::uint8_t SKIP_HTML_CDATA{1U << 0U};
constexpr std::uint8_t SKIP_HTML_DECLARATION{1U << 1U};
constexpr std::uint8_t SKIP_HTML_PROCESSING_INSTRUCTION{1U << 2U};

// The bytes that may start an inline other than plain text, which are the
// line endings, the backslash, the backtick, the ampersand, the angle bracket,
// the emphasis and strikethrough delimiters, the brackets and the exclamation
// mark, and the colon and the letter that may start an extended autolink
constexpr std::array<std::uint8_t, 256> INLINE_SPECIAL{{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, // 0x00
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x10
    0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, // 0x20
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, // 0x30
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x40
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, // 0x50
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x60
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, // 0x70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x80
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0x90
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xA0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xB0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xC0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xD0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 0xE0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // 0xF0
}};

struct Delimiter {
  std::uint32_t previous{NO_DELIMITER};
  std::uint32_t next{NO_DELIMITER};
  std::uint32_t text{NO_NODE};
  std::size_t position{0};
  std::size_t length{0};
  char character{0};
  bool can_open{false};
  bool can_close{false};
};

struct Bracket {
  std::uint32_t text{NO_NODE};
  std::size_t position{0};
  std::ptrdiff_t start_column{0};
  bool image{false};
  bool bracket_after{false};
};

// A character of a segment of a valid domain of GFM section 6.9, which
// "consists of segments of alphanumeric characters, underscores (_) and hyphens
// (-) separated by periods (.)", where alphanumeric characters are ASCII as
// the whitespace characters of GFM section 2.1 are, which the specification
// only extends to Unicode when it says so
inline auto is_domain_character(const char character) noexcept -> bool {
  return sourcemeta::core::is_alphanum(character) || character == '_' ||
         character == '-';
}

// GFM section 6.9: "All such recognized autolinks can only come at the
// beginning of a line, after whitespace, or any of the delimiting characters
// *, _, ~, and ("
inline auto may_precede_extended_autolink(const char character) noexcept
    -> bool {
  return is_whitespace_character(character) || character == '*' ||
         character == '_' || character == '~' || character == '(';
}

// The end of an extended autolink once its trailing punctuation, unbalanced
// parentheses, and entity references are removed as GFM section 6.9 describes
inline auto trim_autolink_end(const std::string_view data,
                              std::size_t link_end) noexcept -> std::size_t {
  std::size_t opening{0};
  std::size_t closing{0};
  for (std::size_t index{0}; index < link_end; ++index) {
    const auto character{data[index]};
    if (character == '<') {
      link_end = index;
      break;
    }

    if (character == '(') {
      ++opening;
    } else if (character == ')') {
      ++closing;
    }
  }

  while (link_end > 0) {
    switch (data[link_end - 1]) {
      case ')':
        if (closing <= opening) {
          return link_end;
        }

        --closing;
        --link_end;
        break;
      // GFM section 6.9: "Trailing punctuation (specifically, ?, !, ., ,, :,
      // *, _, and ~) will not be considered part of the autolink"
      case '?':
      case '!':
      case '.':
      case ',':
      case ':':
      case '*':
      case '_':
      case '~':
        --link_end;
        break;
      case ';': {
        if (link_end < 2) {
          --link_end;
          break;
        }

        // GFM section 6.9 only excludes a semicolon from an autolink along
        // with an entity reference lookalike, which is "& followed by one or
        // more alphanumeric characters"
        auto entity_start{link_end - 2};
        while (entity_start > 0 &&
               sourcemeta::core::is_alphanum(data[entity_start])) {
          --entity_start;
        }

        if (entity_start < link_end - 2 && data[entity_start] == '&') {
          link_end = entity_start;
          break;
        }

        return link_end;
      }

      default:
        return link_end;
    }
  }

  return link_end;
}

// Whether the lengths of the delimiter runs of an opener and a closer of the
// same character let them match. GFM section 6.5 wraps strikethrough text in
// "a matching pair of one or two tildes", while GFM section 6.4 says that "If
// one of the delimiters can both open and close emphasis, then the sum of the
// lengths of the delimiter runs containing the opening and closing delimiters
// must not be a multiple of 3 unless both lengths are multiples of 3"
inline auto delimiter_lengths_match(const Delimiter &opener,
                                    const Delimiter &closer) noexcept -> bool {
  if (closer.character == '~') {
    return opener.length == closer.length;
  }

  return !(closer.can_open || opener.can_close) || closer.length % 3 == 0 ||
         (opener.length + closer.length) % 3 != 0;
}

// The inlines of GFM section 6 and of the strikethrough and autolink
// extensions
class InlineParser {
public:
  explicit InlineParser(Document &document) : document_{document} {}

  auto parse(const std::uint32_t parent) -> void {
    this->parent_ = parent;
    this->input_ = sourcemeta::core::strip_right(
        this->document_.content_of(this->document_.nodes[parent]), is_space);
    this->position_ = 0;
    this->column_offset_ = 0;
    this->last_delimiter_ = NO_DELIMITER;
    this->delimiters_.clear();
    this->brackets_.clear();
    this->backticks_.clear();
    this->scanned_for_backticks_ = false;
    this->rejected_domain_start_ = 0;
    this->rejected_domain_limit_ = 0;
    this->last_link_opener_position_ = 0;
    this->flags_ = 0;
    while (this->position_ < this->input_.size()) {
      this->parse_inline();
    }

    this->process_emphasis(0);
  }

private:
  auto node(const std::uint32_t index) -> Node & {
    return this->document_.nodes[index];
  }

  [[nodiscard]] auto peek() const noexcept -> char {
    return character_at(this->input_, this->position_);
  }

  auto append(const NodeType type, const std::string_view literal)
      -> std::uint32_t {
    const auto index{this->document_.create(type)};
    this->node(index).literal = literal;
    this->document_.append_child(this->parent_, index);
    return index;
  }

  auto append_text_child(const std::uint32_t parent,
                         const std::string_view literal) -> void {
    const auto index{this->document_.create(NodeType::Text)};
    this->node(index).literal = literal;
    this->document_.append_child(parent, index);
  }

  auto parse_inline() -> void {
    const auto character{this->input_[this->position_]};
    switch (character) {
      case '\n':
      case '\r':
        this->handle_newline();
        return;
      case '`':
        this->handle_backticks();
        return;
      case '\\':
        this->handle_backslash();
        return;
      case '&':
        this->handle_entity();
        return;
      case '<':
        this->handle_pointy_brace();
        return;
      case '*':
      case '_':
        this->handle_delimiter(character);
        return;
      case '\'':
      case '"':
      case '-':
      case '.':
        this->append(NodeType::Text, this->input_.substr(this->position_, 1));
        ++this->position_;
        return;
      case '[':
        ++this->position_;
        this->push_bracket(
            false, this->append(NodeType::Text,
                                this->input_.substr(this->position_ - 1, 1)));
        return;
      case ']':
        this->handle_close_bracket();
        return;
      case '!':
        this->handle_bang();
        return;
      case '~':
        this->handle_tilde();
        return;
      default:
        break;
    }

    if (character == ':' && this->match_url_autolink()) {
      return;
    }

    if (character == 'w' && this->match_www_autolink()) {
      return;
    }

    const auto end{this->find_special_character(this->position_ + 1)};
    auto contents{this->input_.substr(this->position_, end - this->position_)};
    this->position_ = end;
    if (end < this->input_.size() && is_line_end(this->input_[end])) {
      contents = sourcemeta::core::strip_right(contents, is_space);
    }

    this->append(NodeType::Text, contents);
  }

  // Extended autolinks do not start inside brackets that may turn into link
  // text. GFM section 6.6 says that autolinks "bind more tightly than the
  // brackets in link text", but the path of an extended autolink of GFM
  // section 6.9 takes "zero or more non-space non-< characters", so it would
  // take the rest of a link such as [www.example.com](/x) along with it
  [[nodiscard]] auto
  may_start_www_autolink(const std::size_t index) const noexcept -> bool {
    if (!this->brackets_.empty()) {
      return false;
    }

    if (index > 0 && !may_precede_extended_autolink(this->input_[index - 1])) {
      return false;
    }

    return this->input_.substr(index, 4) == "www.";
  }

  [[nodiscard]] auto
  may_start_url_autolink(const std::size_t index) const noexcept -> bool {
    return this->brackets_.empty() && index + 3 < this->input_.size() &&
           this->input_[index + 1] == '/' && this->input_[index + 2] == '/';
  }

  [[nodiscard]] auto
  find_special_character(const std::size_t from) const noexcept -> std::size_t {
    const auto size{this->input_.size()};
    for (auto index{from}; index < size; ++index) {
      const auto character{this->input_[index]};
      if (INLINE_SPECIAL[static_cast<unsigned char>(character)] == 0) {
        continue;
      }

      if ((character == 'w' && !this->may_start_www_autolink(index)) ||
          (character == ':' && !this->may_start_url_autolink(index))) {
        continue;
      }

      return index;
    }

    return size;
  }

  auto handle_newline() -> void {
    const auto newline_position{this->position_};
    if (this->input_[this->position_] == '\r') {
      ++this->position_;
    }

    if (this->peek() == '\n') {
      ++this->position_;
    }

    this->column_offset_ = -static_cast<std::ptrdiff_t>(this->position_);
    skip_spaces(this->input_, this->position_);
    if (newline_position > 1 && this->input_[newline_position - 1] == ' ' &&
        this->input_[newline_position - 2] == ' ') {
      this->append(NodeType::LineBreak, {});
    } else {
      this->append(NodeType::SoftBreak, {});
    }
  }

  auto scan_to_closing_backticks(const std::size_t opening_length)
      -> std::size_t {
    // GFM section 6.3 puts no bound on the length of a backtick string. Once
    // the rest of the input was scanned, the last position of every backtick
    // string length is known, so an opening backtick string without a closing
    // one of the same length after it is rejected without scanning again
    if (this->scanned_for_backticks_ &&
        (opening_length >= this->backticks_.size() ||
         this->backticks_[opening_length] <= this->position_)) {
      return 0;
    }

    const auto size{this->input_.size()};
    while (this->position_ < size) {
      const auto *const found{static_cast<const char *>(std::memchr(
          this->input_.data() + this->position_, '`', size - this->position_))};
      if (found == nullptr) {
        this->position_ = size;
        break;
      }

      this->position_ = static_cast<std::size_t>(found - this->input_.data());
      std::size_t count{0};
      while (this->position_ < size && this->input_[this->position_] == '`') {
        ++this->position_;
        ++count;
      }

      if (count >= this->backticks_.size()) {
        this->backticks_.resize(count + 1, 0);
      }

      // A scan that starts after a complete one must not replace the last
      // position of a backtick string length with an earlier one
      this->backticks_[count] =
          std::max(this->backticks_[count], this->position_ - count);

      if (count == opening_length) {
        return this->position_;
      }
    }

    this->scanned_for_backticks_ = true;
    return 0;
  }

  // The content of a code span of GFM section 6.3: "First, line endings are
  // converted to spaces. If the resulting string both begins and ends with a
  // space character, but does not consist entirely of space characters, a
  // single space character is removed from the front and back"
  auto normalize_code(const std::string_view raw) -> std::string_view {
    if (raw.find_first_of("\r\n") == std::string_view::npos) {
      if (raw.size() >= 2 && raw.front() == ' ' && raw.back() == ' ' &&
          raw.find_first_not_of(' ') != std::string_view::npos) {
        return raw.substr(1, raw.size() - 2);
      }

      return raw;
    }

    auto &buffer{this->buffer_};
    buffer.assign(raw);
    const auto size{buffer.size()};
    std::size_t write{0};
    bool contains_nonspace{false};
    for (std::size_t read{0}; read < size; ++read) {
      const auto character{buffer[read]};
      if (character == '\r') {
        if (read + 1 >= size || buffer[read + 1] != '\n') {
          buffer[write] = ' ';
          ++write;
        }
      } else if (character == '\n') {
        buffer[write] = ' ';
        ++write;
      } else {
        buffer[write] = character;
        ++write;
        if (character != ' ') {
          contains_nonspace = true;
        }
      }
    }

    if (contains_nonspace && write >= 2 && buffer[0] == ' ' &&
        buffer[write - 1] == ' ') {
      return this->document_.strings.store(
          std::string_view{buffer}.substr(1, write - 2));
    }

    return this->document_.strings.store(
        std::string_view{buffer}.substr(0, write));
  }

  auto handle_backticks() -> void {
    const auto start{this->position_};
    while (this->peek() == '`') {
      ++this->position_;
    }

    const auto opening_length{this->position_ - start};
    const auto content_start{this->position_};
    const auto end{this->scan_to_closing_backticks(opening_length)};
    if (end == 0) {
      this->position_ = content_start;
      this->append(NodeType::Text, this->input_.substr(start, opening_length));
      return;
    }

    this->append(NodeType::Code,
                 this->normalize_code(this->input_.substr(
                     content_start, end - content_start - opening_length)));
  }

  auto handle_backslash() -> void {
    ++this->position_;
    const auto next{this->peek()};
    if (sourcemeta::core::is_punctuation(next)) {
      ++this->position_;
      this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
      return;
    }

    if (next == '\r' || next == '\n') {
      skip_line_end(this->input_, this->position_);
      this->append(NodeType::LineBreak, {});
      return;
    }

    this->append(NodeType::Text, "\\");
  }

  auto handle_entity() -> void {
    ++this->position_;
    this->buffer_.clear();
    const auto consumed{decode_character_reference(
        this->buffer_, this->input_.substr(this->position_))};
    if (consumed == 0) {
      this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
      return;
    }

    this->position_ += consumed;
    this->append(NodeType::Text, this->document_.strings.store(this->buffer_));
  }

  auto make_autolink(const std::string_view contents, const bool is_email)
      -> void {
    const auto trimmed{sourcemeta::core::trim(contents, is_space)};
    const auto link{this->append(NodeType::Link, {})};
    if (!trimmed.empty()) {
      this->buffer_.clear();
      if (is_email) {
        this->buffer_.append("mailto:");
      }

      if (!decode_character_references(this->buffer_, trimmed)) {
        this->buffer_.append(trimmed);
      }

      this->node(link).literal = this->document_.strings.store(this->buffer_);
    }

    this->buffer_.clear();
    this->append_text_child(link,
                            decode_character_references(this->buffer_, trimmed)
                                ? this->document_.strings.store(this->buffer_)
                                : trimmed);
  }

  auto scan_raw_html() -> std::size_t {
    const auto size{this->input_.size()};
    const auto position{this->position_};
    if (position + 2 > size) {
      return 0;
    }

    const auto character{this->input_[position]};
    if (character == '!') {
      const auto next{this->input_[position + 1]};
      if (next == '-' && character_at(this->input_, position + 2) == '-') {
        const auto length{scan_html_comment(this->input_, position + 1)};
        return length > 0 ? length + 1 : 0;
      }

      if (next == '[') {
        if ((this->flags_ & SKIP_HTML_CDATA) != 0) {
          return 0;
        }

        const auto length{scan_html_cdata(this->input_, position + 2)};
        if (length == 0) {
          return 0;
        }

        if (position + length + 5 > size) {
          this->flags_ |= SKIP_HTML_CDATA;
          return 0;
        }

        return length + 5;
      }

      if ((this->flags_ & SKIP_HTML_DECLARATION) != 0) {
        return 0;
      }

      const auto length{scan_html_declaration(this->input_, position + 1)};
      if (length == 0) {
        return 0;
      }

      if (position + length + 2 > size) {
        this->flags_ |= SKIP_HTML_DECLARATION;
        return 0;
      }

      return length + 2;
    }

    if (character == '?') {
      if ((this->flags_ & SKIP_HTML_PROCESSING_INSTRUCTION) != 0) {
        return 0;
      }

      const auto length{
          scan_html_processing_instruction(this->input_, position + 1) + 3};
      if (position + length > size) {
        this->flags_ |= SKIP_HTML_PROCESSING_INSTRUCTION;
        return 0;
      }

      return length;
    }

    return scan_html_tag(this->input_, position);
  }

  auto handle_pointy_brace() -> void {
    ++this->position_;
    auto length{scan_autolink_uri(this->input_, this->position_)};
    if (length > 0) {
      const auto contents{this->input_.substr(this->position_, length - 1)};
      this->position_ += length;
      this->make_autolink(contents, false);
      return;
    }

    length = scan_autolink_email(this->input_, this->position_);
    if (length > 0) {
      const auto contents{this->input_.substr(this->position_, length - 1)};
      this->position_ += length;
      this->make_autolink(contents, true);
      return;
    }

    length = this->scan_raw_html();
    if (length > 0) {
      const auto contents{this->input_.substr(this->position_ - 1, length + 1)};
      this->position_ += length;
      this->append(NodeType::HTMLInline, contents);
      return;
    }

    this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
  }

  // The characters around a delimiter run of GFM section 6.4, where "the
  // beginning and the end of the line count as Unicode whitespace"
  [[nodiscard]] auto character_before_delimiters() const noexcept -> char32_t {
    if (this->position_ == 0) {
      return U'\n';
    }

    auto index{this->position_ - 1};
    while (index > 0 && sourcemeta::core::is_utf8_continuation(
                            static_cast<unsigned char>(this->input_[index]))) {
      --index;
    }

    const auto preceding{sourcemeta::core::utf8_decode(
        this->input_.substr(index, this->position_ - index), 0)};
    return preceding.has_value() ? preceding->first : U'\n';
  }

  [[nodiscard]] auto character_after_delimiters() const noexcept -> char32_t {
    const auto following{
        sourcemeta::core::utf8_decode(this->input_, this->position_)};
    return following.has_value() ? following->first : U'\n';
  }

  auto handle_delimiter(const char character) -> void {
    const auto before{this->character_before_delimiters()};
    const auto start{this->position_};
    while (this->peek() == character) {
      ++this->position_;
    }

    const auto count{this->position_ - start};
    const auto after{this->character_after_delimiters()};
    const auto space_before{is_unicode_whitespace(before)};
    const auto space_after{is_unicode_whitespace(after)};
    const auto punctuation_before{is_unicode_punctuation(before)};
    const auto punctuation_after{is_unicode_punctuation(after)};
    const auto left_flanking{
        !space_after &&
        (!punctuation_after || space_before || punctuation_before)};
    const auto right_flanking{
        !space_before &&
        (!punctuation_before || space_after || punctuation_after)};
    bool can_open{left_flanking};
    bool can_close{right_flanking};
    if (character == '_') {
      can_open = left_flanking && (!right_flanking || punctuation_before);
      can_close = right_flanking && (!left_flanking || punctuation_after);
    }

    const auto text{
        this->append(NodeType::Text, this->input_.substr(start, count))};
    if (can_open || can_close) {
      this->push_delimiter(character, can_open, can_close, text);
    }
  }

  // GFM section 6.5: "Strikethrough text is any text wrapped in a matching pair
  // of one or two tildes (~)", and "Three or more tildes do not create a
  // strikethrough"
  auto handle_tilde() -> void {
    const auto before{this->character_before_delimiters()};
    const auto start{this->position_};
    while (this->peek() == '~') {
      ++this->position_;
    }

    const auto count{this->position_ - start};
    const auto after{this->character_after_delimiters()};
    const auto space_before{is_unicode_whitespace(before)};
    const auto space_after{is_unicode_whitespace(after)};
    const auto punctuation_before{is_unicode_punctuation(before)};
    const auto punctuation_after{is_unicode_punctuation(after)};
    const auto left_flanking{
        !space_after &&
        (!punctuation_after || space_before || punctuation_before)};
    const auto right_flanking{
        !space_before &&
        (!punctuation_before || space_after || punctuation_after)};
    const auto text{
        this->append(NodeType::Text, this->input_.substr(start, count))};
    if ((left_flanking || right_flanking) && count <= 2) {
      this->push_delimiter('~', left_flanking, right_flanking, text);
    }
  }

  auto handle_bang() -> void {
    ++this->position_;
    if (this->peek() == '[' &&
        character_at(this->input_, this->position_ + 1) != '^') {
      ++this->position_;
      this->push_bracket(
          true, this->append(NodeType::Text,
                             this->input_.substr(this->position_ - 2, 2)));
      return;
    }

    this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
  }

  auto push_delimiter(const char character, const bool can_open,
                      const bool can_close, const std::uint32_t text) -> void {
    const auto index{static_cast<std::uint32_t>(this->delimiters_.size())};
    this->delimiters_.push_back({.previous = this->last_delimiter_,
                                 .next = NO_DELIMITER,
                                 .text = text,
                                 .position = this->position_,
                                 .length = this->node(text).literal.size(),
                                 .character = character,
                                 .can_open = can_open,
                                 .can_close = can_close});
    if (this->last_delimiter_ != NO_DELIMITER) {
      this->delimiters_[this->last_delimiter_].next = index;
    }

    this->last_delimiter_ = index;
  }

  auto remove_delimiter(const std::uint32_t index) -> void {
    const auto &delimiter{this->delimiters_[index]};
    if (delimiter.next == NO_DELIMITER) {
      this->last_delimiter_ = delimiter.previous;
    } else {
      this->delimiters_[delimiter.next].previous = delimiter.previous;
    }

    if (delimiter.previous != NO_DELIMITER) {
      this->delimiters_[delimiter.previous].next = delimiter.next;
    }
  }

  auto push_bracket(const bool image, const std::uint32_t text) -> void {
    if (!this->brackets_.empty()) {
      this->brackets_.back().bracket_after = true;
    }

    const auto position{static_cast<std::ptrdiff_t>(this->position_)};
    this->brackets_.push_back(
        {.text = text,
         .position = this->position_,
         .start_column =
             (image ? position - 1 : position) + this->column_offset_,
         .image = image,
         .bracket_after = false});
  }

  static auto delimiter_index(const char character) noexcept -> std::size_t {
    if (character == '*') {
      return 0;
    }

    return character == '_' ? 1 : 2;
  }

  // Move the inlines between two siblings into a new parent that takes the
  // place right after the first sibling
  auto wrap_between(const std::uint32_t first, const std::uint32_t last,
                    const std::uint32_t wrapper) -> void {
    auto current{this->node(first).next};
    while (current != NO_NODE && current != last) {
      const auto next{this->node(current).next};
      this->document_.unlink(current);
      this->document_.append_child(wrapper, current);
      current = next;
    }
  }

  auto insert_emphasis(const std::uint32_t opener, std::uint32_t closer)
      -> std::uint32_t {
    const auto opener_text{this->delimiters_[opener].text};
    const auto closer_text{this->delimiters_[closer].text};
    auto opener_characters{this->node(opener_text).literal.size()};
    auto closer_characters{this->node(closer_text).literal.size()};
    const std::size_t used{
        closer_characters >= 2 && opener_characters >= 2 ? 2U : 1U};
    opener_characters -= used;
    closer_characters -= used;
    this->node(opener_text).literal =
        this->node(opener_text).literal.substr(0, opener_characters);
    this->node(closer_text).literal =
        this->node(closer_text).literal.substr(0, closer_characters);

    auto between{this->delimiters_[closer].previous};
    while (between != NO_DELIMITER && between != opener) {
      const auto previous{this->delimiters_[between].previous};
      this->remove_delimiter(between);
      between = previous;
    }

    const auto emphasis{this->document_.create(used == 1 ? NodeType::Emphasis
                                                         : NodeType::Strong)};
    this->wrap_between(opener_text, closer_text, emphasis);
    this->document_.insert_after(opener_text, emphasis);
    if (opener_characters == 0) {
      this->document_.unlink(opener_text);
      this->remove_delimiter(opener);
    }

    if (closer_characters == 0) {
      this->document_.unlink(closer_text);
      const auto next{this->delimiters_[closer].next};
      this->remove_delimiter(closer);
      closer = next;
    }

    return closer;
  }

  auto insert_strikethrough(const std::uint32_t opener,
                            const std::uint32_t closer) -> std::uint32_t {
    const auto result{this->delimiters_[closer].next};
    const auto opener_text{this->delimiters_[opener].text};
    const auto closer_text{this->delimiters_[closer].text};
    this->node(opener_text).type = NodeType::Strikethrough;
    this->node(opener_text).literal = {};
    this->wrap_between(opener_text, closer_text, opener_text);
    this->document_.unlink(closer_text);

    auto current{closer};
    while (current != NO_DELIMITER && current != opener) {
      const auto previous{this->delimiters_[current].previous};
      this->remove_delimiter(current);
      current = previous;
    }

    this->remove_delimiter(opener);
    return result;
  }

  auto process_emphasis(const std::size_t stack_bottom) -> void {
    std::array<std::array<std::size_t, 3>, 3> openers_bottom{};
    for (auto &row : openers_bottom) {
      row.fill(stack_bottom);
    }

    auto closer{NO_DELIMITER};
    auto candidate{this->last_delimiter_};
    while (candidate != NO_DELIMITER &&
           this->delimiters_[candidate].position >= stack_bottom) {
      closer = candidate;
      candidate = this->delimiters_[candidate].previous;
    }

    while (closer != NO_DELIMITER) {
      const auto closer_delimiter{this->delimiters_[closer]};
      if (!closer_delimiter.can_close) {
        closer = closer_delimiter.next;
        continue;
      }

      const auto kind{delimiter_index(closer_delimiter.character)};
      const auto remainder{closer_delimiter.length % 3};
      auto opener{closer_delimiter.previous};
      bool opener_found{false};
      while (opener != NO_DELIMITER &&
             this->delimiters_[opener].position >= stack_bottom &&
             this->delimiters_[opener].position >=
                 openers_bottom[remainder][kind]) {
        const auto &opener_delimiter{this->delimiters_[opener]};
        if (opener_delimiter.can_open &&
            opener_delimiter.character == closer_delimiter.character &&
            delimiter_lengths_match(opener_delimiter, closer_delimiter)) {
          opener_found = true;
          break;
        }

        opener = opener_delimiter.previous;
      }

      if (opener_found) {
        closer = closer_delimiter.character == '~'
                     ? this->insert_strikethrough(opener, closer)
                     : this->insert_emphasis(opener, closer);
        continue;
      }

      openers_bottom[remainder][kind] = closer_delimiter.position;
      const auto next{closer_delimiter.next};
      if (!closer_delimiter.can_open) {
        this->remove_delimiter(closer);
      }

      closer = next;
    }

    while (this->last_delimiter_ != NO_DELIMITER &&
           this->delimiters_[this->last_delimiter_].position >= stack_bottom) {
      this->remove_delimiter(this->last_delimiter_);
    }
  }

  auto handle_close_bracket() -> void {
    ++this->position_;
    const auto initial_position{this->position_};
    if (this->brackets_.empty()) {
      this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
      return;
    }

    const auto opener{this->brackets_.back()};
    // GFM section 6.6: "Links may not contain other links, at any level of
    // nesting", so a bracket that opens before the latest link cannot close
    // another one
    if (!opener.image && opener.position < this->last_link_opener_position_) {
      this->brackets_.pop_back();
      this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
      return;
    }

    std::string_view url;
    std::string_view title;
    bool matched{false};
    bool from_reference{false};
    if (this->peek() == '(') {
      const auto spaces{scan_spacechars(this->input_, this->position_ + 1)};
      std::string_view destination;
      const auto destination_length{scan_link_destination(
          this->input_, this->position_ + 1 + spaces, destination)};
      if (destination_length >= 0) {
        const auto url_end{this->position_ + 1 + spaces +
                           static_cast<std::size_t>(destination_length)};
        const auto title_start{url_end +
                               scan_spacechars(this->input_, url_end)};
        const auto title_end{
            title_start == url_end
                ? title_start
                : title_start + scan_link_title(this->input_, title_start)};
        const auto end{title_end + scan_spacechars(this->input_, title_end)};
        if (character_at(this->input_, end) == ')') {
          this->position_ = end + 1;
          url = clean_url(this->document_, destination, this->buffer_, true);
          title = clean_title(
              this->document_,
              this->input_.substr(title_start, title_end - title_start),
              this->buffer_, true);
          matched = true;
        }
      }
    }

    if (!matched) {
      this->position_ = initial_position;
      std::string_view label;
      auto found_label{scan_link_label(this->input_, this->position_, label)};
      if ((!found_label || label.empty()) && !opener.bracket_after) {
        label = this->input_.substr(opener.position,
                                    initial_position - opener.position - 1);
        found_label = true;
      }

      const auto *const reference{
          found_label
              ? find_reference(this->document_, label, this->label_buffer_)
              : nullptr};
      if (reference != nullptr) {
        url = reference->url;
        title = reference->title;
        matched = true;
        from_reference = true;
      }
    }

    if (!matched) {
      this->handle_unmatched_bracket(opener, initial_position);
      return;
    }

    const auto link{this->document_.create(opener.image ? NodeType::Image
                                                        : NodeType::Link)};
    this->node(link).literal = url;
    this->node(link).title = title;
    set_flag(this->node(link), FLAG_REFERENCE_LINK, from_reference);
    this->document_.insert_before(opener.text, link);
    this->wrap_between(opener.text, NO_NODE, link);
    this->document_.unlink(opener.text);
    this->process_emphasis(opener.position);
    this->brackets_.pop_back();
    if (!opener.image) {
      this->last_link_opener_position_ =
          std::max(this->last_link_opener_position_, opener.position);
    }
  }

  // A bracket that is neither a link nor an image might still be a footnote
  // reference, whose label is the text between its brackets
  auto handle_unmatched_bracket(const Bracket &opener,
                                const std::size_t initial_position) -> void {
    const auto after_opener{this->node(opener.text).next};
    if (after_opener != NO_NODE &&
        this->node(after_opener).type == NodeType::Text) {
      const auto literal{this->node(after_opener).literal};
      if (!literal.empty() && literal.front() == '^' &&
          (literal.size() > 1 || this->node(after_opener).next != NO_NODE)) {
        this->position_ = initial_position;
        const auto end_column{static_cast<std::ptrdiff_t>(this->position_) +
                              this->column_offset_};
        const auto length{
            opener.start_column + 2 <= end_column
                ? static_cast<std::size_t>(end_column - opener.start_column - 2)
                : std::size_t{0}};
        const auto *const label_start{literal.data() + 1};
        const auto *const input_start{this->input_.data()};
        const auto *const input_end{this->input_.data() + this->input_.size()};
        const std::less<const char *> before{};
        const auto available{
            !before(label_start, input_start) && !before(input_end, label_start)
                ? static_cast<std::size_t>(input_end - label_start)
                : literal.size() - 1};
        const auto reference{
            this->document_.create(NodeType::FootnoteReference)};
        this->document_.footnote_reference_nodes.push_back(reference);
        this->node(reference).literal =
            std::string_view{label_start, std::min(length, available)};
        this->document_.insert_before(opener.text, reference);
        this->process_emphasis(opener.position);
        auto current{this->node(opener.text).next};
        while (current != NO_NODE) {
          const auto next{this->node(current).next};
          this->document_.unlink(current);
          current = next;
        }

        this->document_.unlink(opener.text);
        this->brackets_.pop_back();
        return;
      }
    }

    this->brackets_.pop_back();
    this->position_ = initial_position;
    this->append(NodeType::Text, this->input_.substr(this->position_ - 1, 1));
  }

  // GFM section 6.9: "An extended url autolink will be recognised when one of
  // the schemes http://, or https://, followed by a valid domain", where RFC
  // 3986 Section 3.1 says that "schemes are case-insensitive"
  static auto
  starts_with_extended_url_scheme(const std::string_view link) noexcept
      -> bool {
    for (const auto scheme :
         std::array<std::string_view, 2>{{"http://", "https://"}}) {
      if (link.size() > scheme.size() &&
          sourcemeta::core::starts_with_ignore_case(link, scheme) &&
          is_domain_character(link[scheme.size()])) {
        return true;
      }
    }

    return false;
  }

  // The length of the valid domain of GFM section 6.9 at a position of the
  // input, or zero if there is none. "There must be at least one period, and
  // no underscores may be present in the last two segments of the domain"
  auto scan_autolink_domain(const std::size_t start) -> std::size_t {
    // A later start within a domain that was rejected for its underscores has
    // the same last two segments until the second to last period, so it is
    // rejected without scanning the same characters again
    if (start > this->rejected_domain_start_ &&
        start < this->rejected_domain_limit_) {
      return 0;
    }

    const auto data{this->input_.substr(start)};
    std::size_t index{0};
    std::size_t periods{0};
    std::size_t last_period{0};
    std::size_t second_to_last_period{0};
    std::size_t underscores_before_last_period{0};
    std::size_t underscores_after_last_period{0};
    for (; index < data.size(); ++index) {
      const auto character{data[index]};
      if (character == '.') {
        // The segments that periods separate have at least one character
        if (index + 1 >= data.size() || !is_domain_character(data[index + 1])) {
          break;
        }

        underscores_before_last_period = underscores_after_last_period;
        underscores_after_last_period = 0;
        second_to_last_period = last_period;
        last_period = index;
        ++periods;
      } else if (character == '_') {
        ++underscores_after_last_period;
      } else if (!is_domain_character(character)) {
        break;
      }
    }

    if (underscores_before_last_period > 0 ||
        underscores_after_last_period > 0) {
      if (periods >= 2) {
        this->rejected_domain_start_ = start;
        this->rejected_domain_limit_ = start + second_to_last_period;
      }

      return 0;
    }

    return periods > 0 ? index : 0;
  }

  auto match_www_autolink() -> bool {
    if (!this->may_start_www_autolink(this->position_)) {
      return false;
    }

    const auto data{this->input_.substr(this->position_)};
    auto link_end{this->scan_autolink_domain(this->position_)};
    if (link_end == 0) {
      return false;
    }

    while (link_end < data.size() && !is_space(data[link_end]) &&
           data[link_end] != '<') {
      ++link_end;
    }

    link_end = trim_autolink_end(data, link_end);
    if (link_end == 0) {
      return false;
    }

    this->position_ += link_end;
    this->buffer_.assign("http://");
    this->buffer_.append(data.substr(0, link_end));
    const auto link{this->append(NodeType::Link,
                                 this->document_.strings.store(this->buffer_))};
    this->append_text_child(link, data.substr(0, link_end));
    return true;
  }

  auto remove_trailing_text(std::size_t count) -> void {
    auto index{this->node(this->parent_).last_child};
    while (count > 0 && index != NO_NODE &&
           this->node(index).type == NodeType::Text) {
      auto &literal{this->node(index).literal};
      if (literal.size() < count) {
        count -= literal.size();
        literal = literal.substr(0, 0);
      } else {
        literal = literal.substr(0, literal.size() - count);
        count = 0;
      }

      index = this->node(index).previous;
    }
  }

  auto match_url_autolink() -> bool {
    if (!this->may_start_url_autolink(this->position_)) {
      return false;
    }

    const auto data{this->input_.substr(this->position_)};
    std::size_t rewind{0};
    while (rewind < this->position_ &&
           sourcemeta::core::is_alpha(
               this->input_[this->position_ - rewind - 1])) {
      ++rewind;
    }

    const auto start{this->position_ - rewind};
    if ((start > 0 &&
         !may_precede_extended_autolink(this->input_[start - 1])) ||
        !starts_with_extended_url_scheme(this->input_.substr(start))) {
      return false;
    }

    const auto domain_length{this->scan_autolink_domain(this->position_ + 3)};
    if (domain_length == 0) {
      return false;
    }

    auto link_end{3 + domain_length};
    while (link_end < data.size() && !is_space(data[link_end]) &&
           data[link_end] != '<') {
      ++link_end;
    }

    link_end = trim_autolink_end(data, link_end);
    if (link_end == 0) {
      return false;
    }

    this->position_ += link_end;
    this->remove_trailing_text(rewind);
    const auto url{this->input_.substr(start, link_end + rewind)};
    const auto link{this->append(NodeType::Link, url)};
    this->append_text_child(link, url);
    return true;
  }

  Document &document_;
  std::uint32_t parent_{NO_NODE};
  std::string_view input_;
  std::size_t position_{0};
  std::ptrdiff_t column_offset_{0};
  std::uint32_t last_delimiter_{NO_DELIMITER};
  std::vector<Delimiter> delimiters_;
  std::vector<Bracket> brackets_;
  std::vector<std::size_t> backticks_;
  bool scanned_for_backticks_{false};
  std::size_t rejected_domain_start_{0};
  std::size_t rejected_domain_limit_{0};
  std::uint8_t flags_{0};
  std::string buffer_;
  std::string label_buffer_;
  std::size_t last_link_opener_position_{0};
};

} // namespace sourcemeta::core::markdown

#endif
