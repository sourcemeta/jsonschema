#ifndef SOURCEMETA_CORE_YAML_LEXER_H_
#define SOURCEMETA_CORE_YAML_LEXER_H_

#include <sourcemeta/core/unicode.h>
#include <sourcemeta/core/yaml_error.h>

#include <cstdint>     // std::uint8_t, std::uint64_t
#include <deque>       // std::deque
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core::yaml {

enum class TokenType : std::uint8_t {
  StreamStart,
  StreamEnd,
  DocumentStart,
  DocumentEnd,
  MappingStart,
  MappingEnd,
  SequenceStart,
  SequenceEnd,
  BlockMappingKey,
  BlockMappingValue,
  BlockSequenceEntry,
  FlowEntry,
  Scalar,
  Anchor,
  Alias,
  Tag,
  DirectiveYAML,
  DirectiveTag,
  DirectiveReserved
};

enum class ScalarStyle : std::uint8_t {
  Plain,
  SingleQuoted,
  DoubleQuoted,
  Literal,
  Folded
};

struct Token {
  TokenType type;
  std::string_view value;
  std::uint64_t line;
  std::uint64_t column;
  std::size_t position{0};
  ScalarStyle scalar_style{ScalarStyle::Plain};
  bool multiline{false};
};

class Lexer {
public:
  Lexer(const std::string_view input) : input_{input} {}

  auto next() -> std::optional<Token> {
    this->skip_whitespace_and_comments();

    if (this->position_ >= this->input_.size()) {
      if (!this->stream_started_) {
        this->stream_started_ = true;
        return Token{.type = TokenType::StreamStart,
                     .value = {},
                     .line = this->line_,
                     .column = this->column_};
      }
      if (!this->stream_ended_) {
        this->stream_ended_ = true;
        const auto end_line{this->column_ > 0 ? this->line_ + 1 : this->line_};
        const std::uint64_t end_column{0};
        return Token{.type = TokenType::StreamEnd,
                     .value = {},
                     .line = end_line,
                     .column = end_column};
      }
      return std::nullopt;
    }

    if (!this->stream_started_) {
      this->stream_started_ = true;
      return Token{.type = TokenType::StreamStart,
                   .value = {},
                   .line = this->line_,
                   .column = this->column_};
    }

    const auto current_line{this->line_};
    const auto current_column{this->column_};
    const auto current_position{this->position_};

    if (this->tab_at_line_start_) {
      this->tab_at_line_start_ = false;
      const char next_char{this->peek()};
      if (next_char != '{' && next_char != '[') {
        throw YAMLParseError{current_line, current_column,
                             "Tab characters cannot be used for indentation"};
      }
    }

    if (this->column_ == 1 && this->check_document_marker('-')) {
      this->advance(3);
      return Token{.type = TokenType::DocumentStart,
                   .value = "---",
                   .line = current_line,
                   .column = current_column,
                   .position = current_position};
    }

    if (this->column_ == 1 && this->check_document_marker('.')) {
      this->advance(3);
      this->validate_trailing_content();
      return Token{.type = TokenType::DocumentEnd,
                   .value = "...",
                   .line = current_line,
                   .column = current_column,
                   .position = current_position};
    }

    const char current{this->peek()};

    if (current == '{') {
      this->advance(1);
      this->flow_level_++;
      return Token{.type = TokenType::MappingStart,
                   .value = "{",
                   .line = current_line,
                   .column = current_column};
    }

    if (current == '[') {
      this->advance(1);
      this->flow_level_++;
      return Token{.type = TokenType::SequenceStart,
                   .value = "[",
                   .line = current_line,
                   .column = current_column};
    }

    if (this->flow_level_ > 0) {
      if (current == '}') {
        this->advance(1);
        this->flow_level_--;
        if (this->flow_level_ == 0) {
          this->validate_trailing_content();
        }
        return Token{.type = TokenType::MappingEnd,
                     .value = "}",
                     .line = current_line,
                     .column = current_column};
      }
      if (current == ']') {
        this->advance(1);
        this->flow_level_--;
        if (this->flow_level_ == 0) {
          this->validate_trailing_content();
        }
        return Token{.type = TokenType::SequenceEnd,
                     .value = "]",
                     .line = current_line,
                     .column = current_column};
      }
      if (current == ',') {
        this->advance(1);
        return Token{.type = TokenType::FlowEntry,
                     .value = ",",
                     .line = current_line,
                     .column = current_column};
      }
    } else if (current == '-' && this->is_followed_by_whitespace()) {
      this->advance(1);
      return Token{.type = TokenType::BlockSequenceEntry,
                   .value = "-",
                   .line = current_line,
                   .column = current_column};
    }

    if (current == '?' && this->is_followed_by_whitespace()) {
      this->advance(1);
      return Token{.type = TokenType::BlockMappingKey,
                   .value = "?",
                   .line = current_line,
                   .column = current_column};
    }

    if (current == ':' && this->is_value_indicator()) {
      this->advance(1);
      this->last_was_quoted_scalar_ = false;
      return Token{.type = TokenType::BlockMappingValue,
                   .value = ":",
                   .line = current_line,
                   .column = current_column};
    }

    if (current == '&') {
      return this->scan_anchor_or_alias(TokenType::Anchor);
    }

    if (current == '*') {
      return this->scan_anchor_or_alias(TokenType::Alias);
    }

    if (current == '!') {
      return this->scan_tag();
    }

    if (current == '%') {
      return this->scan_directive();
    }

    if (current == '\'') {
      this->last_was_quoted_scalar_ = true;
      return this->scan_single_quoted_scalar();
    }

    if (current == '"') {
      this->last_was_quoted_scalar_ = true;
      return this->scan_double_quoted_scalar();
    }

    if (current == '|') {
      this->last_was_quoted_scalar_ = false;
      return this->scan_block_scalar(ScalarStyle::Literal);
    }

    if (current == '>') {
      this->last_was_quoted_scalar_ = false;
      return this->scan_block_scalar(ScalarStyle::Folded);
    }

    if (current == '#') {
      throw YAMLParseError{current_line, current_column,
                           "Unexpected '#' character"};
    }

    this->last_was_quoted_scalar_ = false;
    return this->scan_plain_scalar();
  }

public:
  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    if (this->position_ >= this->input_.size() && this->column_ > 1) {
      return this->line_ + 1;
    }
    return this->line_;
  }

  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    if (this->position_ >= this->input_.size()) {
      return 0;
    }
    return this->column_;
  }

  [[nodiscard]] auto flow_level() const noexcept -> std::size_t {
    return this->flow_level_;
  }

  auto set_block_indent(const std::size_t indent) noexcept -> void {
    this->block_indent_ = indent;
  }

  [[nodiscard]] auto block_indent() const noexcept -> std::size_t {
    return this->block_indent_;
  }

  [[nodiscard]] auto position() const noexcept -> std::size_t {
    return this->position_;
  }

private:
  [[nodiscard]] static auto is_whitespace(const char character) noexcept
      -> bool {
    return character == ' ' || character == '\t' || character == '\n' ||
           character == '\r';
  }

  [[nodiscard]] static auto is_flow_indicator(const char character) noexcept
      -> bool {
    return character == ',' || character == '[' || character == ']' ||
           character == '{' || character == '}';
  }

  [[nodiscard]] auto peek(const std::size_t offset = 0) const noexcept -> char {
    const auto index{this->position_ + offset};
    if (index >= this->input_.size()) {
      return '\0';
    }
    return this->input_[index];
  }

  auto advance(const std::size_t count) noexcept -> void {
    for (std::size_t index = 0; index < count; ++index) {
      if (this->position_ >= this->input_.size()) {
        break;
      }
      if (this->input_[this->position_] == '\n') {
        this->line_++;
        this->column_ = 1;
      } else {
        this->column_++;
      }
      this->position_++;
    }
  }

  auto skip_whitespace_and_comments() -> void {
    bool preceded_by_whitespace{
        this->column_ == 1 ||
        (this->position_ > 0 &&
         is_whitespace(this->input_[this->position_ - 1]))};
    bool at_line_start{this->column_ == 1};
    this->tab_at_line_start_ = false;
    while (this->position_ < this->input_.size()) {
      const char current{this->peek()};

      if (current == ' ') {
        preceded_by_whitespace = true;
        this->advance(1);
        continue;
      }

      if (current == '\t') {
        if (this->flow_level_ == 0 && at_line_start) {
          this->tab_at_line_start_ = true;
        }
        preceded_by_whitespace = true;
        this->advance(1);
        continue;
      }

      if (current == '\n' || current == '\r') {
        this->advance(1);
        if (current == '\r' && this->peek() == '\n') {
          this->advance(1);
        }
        preceded_by_whitespace = true;
        at_line_start = true;
        this->tab_at_line_start_ = false;
        continue;
      }

      if (current == '#' && preceded_by_whitespace) {
        while (this->position_ < this->input_.size() && this->peek() != '\n') {
          this->advance(1);
        }
        continue;
      }

      break;
    }
  }

  [[nodiscard]] auto check_document_marker(const char marker) const noexcept
      -> bool {
    if (this->position_ + 2 >= this->input_.size()) {
      return false;
    }
    if (this->input_[this->position_] != marker ||
        this->input_[this->position_ + 1] != marker ||
        this->input_[this->position_ + 2] != marker) {
      return false;
    }
    if (this->position_ + 3 < this->input_.size()) {
      const char after{this->input_[this->position_ + 3]};
      return is_whitespace(after) || after == '\0';
    }
    return true;
  }

  [[nodiscard]] auto is_followed_by_whitespace() const noexcept -> bool {
    if (this->position_ + 1 >= this->input_.size()) {
      return true;
    }
    return is_whitespace(this->input_[this->position_ + 1]);
  }

  [[nodiscard]] auto is_value_indicator() const noexcept -> bool {
    if (this->flow_level_ > 0) {
      if (this->last_was_quoted_scalar_) {
        return true;
      }
      if (this->position_ + 1 >= this->input_.size()) {
        return true;
      }
      const char after{this->input_[this->position_ + 1]};
      return is_whitespace(after) || is_flow_indicator(after);
    }
    return this->is_followed_by_whitespace();
  }

  [[nodiscard]] auto line_contains_mapping_key() const noexcept -> bool {
    auto scan_position{this->position_};
    while (scan_position < this->input_.size()) {
      const char character{this->input_[scan_position]};
      if (character == '\n' || character == '\r') {
        return false;
      }
      if (character == ':') {
        if (scan_position + 1 >= this->input_.size()) {
          return true;
        }
        const char after{this->input_[scan_position + 1]};
        if (is_whitespace(after)) {
          return true;
        }
      }
      scan_position++;
    }
    return false;
  }

  auto scan_anchor_or_alias(const TokenType type) -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};
    const auto start_position{this->position_};
    this->advance(1);
    while (this->position_ < this->input_.size()) {
      const char current{this->peek()};
      if (is_whitespace(current) || is_flow_indicator(current)) {
        break;
      }
      this->advance(1);
    }
    const auto length{this->position_ - start_position};
    return Token{.type = type,
                 .value = this->input_.substr(start_position + 1, length - 1),
                 .line = start_line,
                 .column = start_column};
  }

  auto scan_tag() -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};
    const auto start_position{this->position_};

    this->advance(1);

    if (this->peek() == '<') {
      this->advance(1);
      while (this->position_ < this->input_.size() && this->peek() != '>') {
        this->advance(1);
      }
      if (this->peek() == '>') {
        this->advance(1);
      }
    } else {
      while (this->position_ < this->input_.size()) {
        const char current{this->peek()};
        if (is_whitespace(current) || is_flow_indicator(current)) {
          break;
        }
        this->advance(1);
      }
    }

    const auto length{this->position_ - start_position};

    if (this->position_ < this->input_.size() && this->flow_level_ == 0) {
      const char after_tag{this->peek()};
      if (after_tag == ',') {
        throw YAMLParseError{this->line_, this->column_,
                             "Invalid character after tag in block context"};
      }
    }

    return Token{.type = TokenType::Tag,
                 .value = this->input_.substr(start_position, length),
                 .line = start_line,
                 .column = start_column};
  }

  auto scan_directive() -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};
    const auto start_position{this->position_};

    this->advance(1);

    while (this->position_ < this->input_.size() && this->peek() != '\n' &&
           this->peek() != '\r') {
      this->advance(1);
    }

    const auto length{this->position_ - start_position};
    const auto directive_content{this->input_.substr(start_position, length)};

    TokenType token_type{TokenType::DirectiveReserved};
    if (directive_content.starts_with("%YAML")) {
      token_type = TokenType::DirectiveYAML;
    } else if (directive_content.starts_with("%TAG")) {
      token_type = TokenType::DirectiveTag;
    }

    return Token{.type = token_type,
                 .value = directive_content,
                 .line = start_line,
                 .column = start_column};
  }

  auto scan_single_quoted_scalar() -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};

    this->advance(1);

    auto &buffer{this->get_buffer()};
    std::string line_content;
    bool first_line{true};
    std::size_t pending_newlines{0};
    bool found_closing_quote{false};

    while (this->position_ < this->input_.size()) {
      const char current{this->peek()};

      if (current == '\'') {
        if (this->peek(1) == '\'') {
          line_content += '\'';
          this->advance(2);
        } else {
          this->flush_flow_line(buffer, line_content, pending_newlines,
                                first_line, true);
          this->advance(1);
          found_closing_quote = true;
          break;
        }
      } else if (current == '\n' || current == '\r') {
        this->flush_flow_line(buffer, line_content, pending_newlines,
                              first_line);
        first_line = false;
        this->skip_flow_scalar_line_break(current, pending_newlines);
      } else {
        line_content += current;
        this->advance(1);
      }
    }

    if (!found_closing_quote) {
      throw YAMLParseError{start_line, start_column,
                           "Missing closing quote in single-quoted scalar"};
    }

    this->validate_trailing_content();

    return Token{.type = TokenType::Scalar,
                 .value = buffer,
                 .line = start_line,
                 .column = start_column,
                 .scalar_style = ScalarStyle::SingleQuoted,
                 .multiline = !first_line};
  }

  auto flush_flow_line(std::string &buffer, std::string &line_content,
                       std::size_t &pending_newlines, const bool first_line,
                       const bool is_final = false) -> void {
    if (!is_final) {
      while (!line_content.empty() &&
             (line_content.back() == ' ' || line_content.back() == '\t')) {
        line_content.pop_back();
      }
    }

    if (pending_newlines > 0 && !first_line) {
      if (pending_newlines == 1) {
        buffer += ' ';
      } else {
        for (std::size_t count = 1; count < pending_newlines; ++count) {
          buffer += '\n';
        }
      }
      pending_newlines = 0;
    }

    if (!line_content.empty()) {
      buffer += line_content;
    }
    line_content.clear();
  }

  auto skip_flow_scalar_line_break(const char current,
                                   std::size_t &pending_newlines) -> void {
    this->advance(1);
    if (current == '\r' && this->peek() == '\n') {
      this->advance(1);
    }
    pending_newlines++;
    while (this->position_ < this->input_.size()) {
      const char character{this->peek()};
      if (character == ' ' || character == '\t') {
        this->advance(1);
      } else if (character == '\n') {
        pending_newlines++;
        this->advance(1);
      } else if (character == '\r') {
        pending_newlines++;
        this->advance(1);
        if (this->peek() == '\n') {
          this->advance(1);
        }
      } else {
        break;
      }
    }
    this->validate_flow_scalar_continuation();
  }

  auto validate_flow_scalar_continuation() -> void {
    if (this->position_ >= this->input_.size()) {
      return;
    }
    if (this->column_ == 1 && (this->check_document_marker('-') ||
                               this->check_document_marker('.'))) {
      throw YAMLParseError{this->line_, this->column_,
                           "Document marker inside flow scalar"};
    }
    if (this->flow_level_ == 0 && this->block_indent_ != SIZE_MAX) {
      const auto current_indent{static_cast<std::size_t>(this->column_ - 1)};
      if (current_indent <= this->block_indent_) {
        throw YAMLParseError{this->line_, this->column_,
                             "Insufficient indentation in flow scalar"};
      }
    }
  }

  auto validate_trailing_content() -> void {
    auto lookahead{this->position_};
    bool seen_whitespace{false};
    while (lookahead < this->input_.size()) {
      const char character{this->input_[lookahead]};
      if (character == ' ' || character == '\t') {
        seen_whitespace = true;
        lookahead++;
        continue;
      }
      if (character == '\n' || character == '\r') {
        return;
      }
      if (character == '#') {
        if (seen_whitespace) {
          return;
        }
        throw YAMLParseError{this->line_, this->column_,
                             "Invalid trailing content"};
      }
      if (character == ':') {
        return;
      }
      if (this->flow_level_ > 0 && is_flow_indicator(character)) {
        return;
      }
      throw YAMLParseError{this->line_, this->column_,
                           "Invalid trailing content"};
    }
  }

  auto scan_double_quoted_scalar() -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};

    this->advance(1);

    auto &buffer{this->get_buffer()};
    std::string line_content;
    bool first_line{true};
    std::size_t pending_newlines{0};
    bool found_closing_quote{false};

    while (this->position_ < this->input_.size()) {
      const char current{this->peek()};

      if (current == '"') {
        this->flush_flow_line(buffer, line_content, pending_newlines,
                              first_line, true);
        this->advance(1);
        found_closing_quote = true;
        break;
      }

      if (current == '\\') {
        this->advance(1);
        if (this->position_ < this->input_.size()) {
          const char escaped{this->peek()};
          switch (escaped) {
            case '0':
              line_content += '\0';
              break;
            case 'a':
              line_content += '\a';
              break;
            case 'b':
              line_content += '\b';
              break;
            case 't':
            case '\t':
              line_content += '\t';
              break;
            case 'n':
              line_content += '\n';
              break;
            case 'v':
              line_content += '\v';
              break;
            case 'f':
              line_content += '\f';
              break;
            case 'r':
              line_content += '\r';
              break;
            case 'e':
              line_content += '\x1b';
              break;
            case ' ':
              line_content += ' ';
              break;
            case '"':
              line_content += '"';
              break;
            case '/':
              line_content += '/';
              break;
            case '\\':
              line_content += '\\';
              break;
            case 'N':
              line_content += "\xc2\x85";
              break;
            case '_':
              line_content += "\xc2\xa0";
              break;
            case 'L':
              line_content += "\xe2\x80\xa8";
              break;
            case 'P':
              line_content += "\xe2\x80\xa9";
              break;
            case 'x':
              this->advance(1);
              line_content += this->parse_hex_escape(2);
              continue;
            case 'u':
              this->advance(1);
              line_content += this->parse_hex_escape(4);
              continue;
            case 'U':
              this->advance(1);
              line_content += this->parse_hex_escape(8);
              continue;
            case '\n':
            case '\r':
              if (escaped == '\r' && this->peek(1) == '\n') {
                this->advance(1);
              }
              this->advance(1);
              while (this->position_ < this->input_.size() &&
                     (this->peek() == ' ' || this->peek() == '\t')) {
                this->advance(1);
              }
              continue;
            default:
              throw YAMLParseError{this->line_, this->column_,
                                   "Invalid escape sequence in "
                                   "double-quoted scalar"};
          }
          this->advance(1);
        }
      } else if (current == '\n' || current == '\r') {
        this->flush_flow_line(buffer, line_content, pending_newlines,
                              first_line);
        first_line = false;
        this->skip_flow_scalar_line_break(current, pending_newlines);
      } else {
        line_content += current;
        this->advance(1);
      }
    }

    if (!found_closing_quote) {
      throw YAMLParseError{start_line, start_column,
                           "Missing closing quote in double-quoted scalar"};
    }

    this->validate_trailing_content();

    return Token{.type = TokenType::Scalar,
                 .value = buffer,
                 .line = start_line,
                 .column = start_column,
                 .scalar_style = ScalarStyle::DoubleQuoted,
                 .multiline = !first_line};
  }

  auto parse_hex_escape(const std::size_t digits) -> std::string {
    std::string hex;
    for (std::size_t index = 0;
         index < digits && this->position_ < this->input_.size(); ++index) {
      hex += this->peek();
      this->advance(1);
    }

    if (hex.size() != digits) {
      throw YAMLParseError{this->line_, this->column_,
                           "Truncated hex escape sequence"};
    }

    std::size_t parsed{0};
    unsigned long codepoint{};
    try {
      codepoint = std::stoul(hex, &parsed, 16);
    } catch (...) {
      throw YAMLParseError{this->line_, this->column_,
                           "Invalid hex escape sequence"};
    }

    if (parsed != hex.size()) {
      throw YAMLParseError{this->line_, this->column_,
                           "Invalid hex escape sequence"};
    }

    return codepoint_to_utf8(static_cast<char32_t>(codepoint));
  }

  [[nodiscard]] auto calculate_parent_indentation(
      const std::size_t indicator_position) const noexcept -> std::size_t {
    std::size_t line_start{indicator_position};
    while (line_start > 0 && this->input_[line_start - 1] != '\n' &&
           this->input_[line_start - 1] != '\r') {
      line_start--;
    }

    std::size_t leading_spaces{0};
    std::size_t scan_position{line_start};
    while (scan_position < this->input_.size() &&
           this->input_[scan_position] == ' ') {
      leading_spaces++;
      scan_position++;
    }

    bool in_sequence_entry{false};
    if (scan_position < this->input_.size() - 1 &&
        this->input_[scan_position] == '-' &&
        this->input_[scan_position + 1] == ' ') {
      in_sequence_entry = true;
    }

    bool is_mapping_value_same_line{false};
    for (std::size_t index = line_start; index < indicator_position; ++index) {
      if (this->input_[index] == ':') {
        is_mapping_value_same_line = true;
        break;
      }
    }

    if (in_sequence_entry && is_mapping_value_same_line) {
      return leading_spaces + 2;
    }

    if (is_mapping_value_same_line) {
      return leading_spaces;
    }

    return 0;
  }

  auto detect_block_scalar_indent(const std::size_t explicit_indent,
                                  const std::size_t indicator_position,
                                  const std::uint64_t start_line,
                                  const std::uint64_t start_column)
      -> std::size_t {
    std::size_t content_indent{0};

    if (explicit_indent > 0) {
      const auto parent_indent{
          this->calculate_parent_indentation(indicator_position)};
      content_indent = parent_indent + explicit_indent;
    } else {
      const auto saved_position{this->position_};
      const auto saved_line{this->line_};
      const auto saved_column{this->column_};

      std::size_t max_leading_empty_indent{0};
      std::size_t current_empty_indent{0};
      while (this->position_ < this->input_.size()) {
        if (this->peek() == ' ') {
          content_indent++;
          current_empty_indent++;
          this->advance(1);
        } else if (this->peek() == '\n' || this->peek() == '\r') {
          if (current_empty_indent > max_leading_empty_indent) {
            max_leading_empty_indent = current_empty_indent;
          }
          content_indent = 0;
          current_empty_indent = 0;
          this->advance(1);
        } else {
          break;
        }
      }

      this->position_ = saved_position;
      this->line_ = saved_line;
      this->column_ = saved_column;

      if (max_leading_empty_indent > content_indent && content_indent > 0) {
        throw YAMLParseError{
            start_line, start_column,
            "Leading empty line has more spaces than content indentation"};
      }
    }

    if (content_indent == 0 && start_column > 5) {
      content_indent = 1;
    }

    return content_indent;
  }

  auto scan_block_scalar(const ScalarStyle style) -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};
    const auto indicator_position{this->position_};

    this->advance(1);

    char chomping{'c'};
    std::size_t explicit_indent{0};

    bool seen_header_whitespace{false};
    while (this->position_ < this->input_.size()) {
      const char current{this->peek()};
      if (current == '-') {
        chomping = '-';
        this->advance(1);
      } else if (current == '+') {
        chomping = '+';
        this->advance(1);
      } else if (current >= '1' && current <= '9') {
        explicit_indent = static_cast<std::size_t>(current - '0');
        this->advance(1);
      } else if (current == ' ' || current == '\t') {
        seen_header_whitespace = true;
        this->advance(1);
      } else if (current == '#' && seen_header_whitespace) {
        while (this->position_ < this->input_.size() && this->peek() != '\n') {
          this->advance(1);
        }
      } else if (current == '\n' || current == '\r') {
        break;
      } else {
        throw YAMLParseError{this->line_, this->column_,
                             "Invalid content in block scalar header"};
      }
    }

    if (this->peek() == '\n' || this->peek() == '\r') {
      this->advance(1);
      if (this->input_[this->position_ - 1] == '\r' && this->peek() == '\n') {
        this->advance(1);
      }
    }

    auto &buffer{this->get_buffer()};
    const auto content_indent{this->detect_block_scalar_indent(
        explicit_indent, indicator_position, start_line, start_column)};

    std::size_t blank_line_count{0};
    bool previous_was_more_indented{false};
    bool previous_started_with_whitespace{false};
    bool had_line_break{false};
    std::string trailing_newlines;

    while (this->position_ < this->input_.size()) {
      std::size_t line_indent{0};
      while (this->position_ < this->input_.size() && this->peek() == ' ') {
        line_indent++;
        this->advance(1);
      }

      if (this->peek() == '\n' || this->peek() == '\r') {
        if (style == ScalarStyle::Literal) {
          if (line_indent > content_indent) {
            buffer += trailing_newlines;
            trailing_newlines.clear();
            for (std::size_t index = content_indent; index < line_indent;
                 ++index) {
              buffer += ' ';
            }
          }
          trailing_newlines += '\n';
        } else {
          blank_line_count++;
        }
        this->advance(1);
        if (this->input_[this->position_ - 1] == '\r' && this->peek() == '\n') {
          this->advance(1);
        }
        continue;
      }

      if (line_indent < content_indent) {
        for (std::size_t index = 0; index < line_indent; ++index) {
          this->position_--;
          this->column_--;
        }
        break;
      }

      if (line_indent == 0 && this->position_ + 2 < this->input_.size()) {
        if ((this->peek() == '-' && this->peek(1) == '-' &&
             this->peek(2) == '-') ||
            (this->peek() == '.' && this->peek(1) == '.' &&
             this->peek(2) == '.')) {
          break;
        }
      }

      if (style == ScalarStyle::Literal) {
        buffer += trailing_newlines;
        trailing_newlines.clear();
      } else {
        const bool starts_with_whitespace{this->peek() == '\t'};
        if (had_line_break) {
          const bool preserve_line_break{
              previous_was_more_indented || previous_started_with_whitespace ||
              line_indent > content_indent || starts_with_whitespace};
          if (blank_line_count == 0 && !preserve_line_break) {
            buffer += ' ';
          } else {
            if (preserve_line_break) {
              buffer += '\n';
            }
            for (std::size_t count = 0; count < blank_line_count; ++count) {
              buffer += '\n';
            }
          }
        } else if (blank_line_count > 0) {
          for (std::size_t count = 0; count < blank_line_count; ++count) {
            buffer += '\n';
          }
        }
        blank_line_count = 0;
        had_line_break = false;
        previous_started_with_whitespace = starts_with_whitespace;
      }

      for (std::size_t index = content_indent; index < line_indent; ++index) {
        buffer += ' ';
      }

      while (this->position_ < this->input_.size() && this->peek() != '\n' &&
             this->peek() != '\r') {
        buffer += this->peek();
        this->advance(1);
      }

      if (style == ScalarStyle::Folded) {
        previous_was_more_indented = (line_indent > content_indent);
      }

      if (this->peek() == '\n' || this->peek() == '\r') {
        if (style == ScalarStyle::Literal) {
          trailing_newlines += '\n';
        } else {
          had_line_break = true;
        }
        this->advance(1);
        if (this->input_[this->position_ - 1] == '\r' && this->peek() == '\n') {
          this->advance(1);
        }
      }
    }

    if (chomping == '+') {
      if (style == ScalarStyle::Literal) {
        buffer += trailing_newlines;
      } else {
        if (had_line_break) {
          buffer += '\n';
        }
        for (std::size_t count = 0; count < blank_line_count; ++count) {
          buffer += '\n';
        }
      }
    } else if (chomping == 'c' && !buffer.empty()) {
      if (style == ScalarStyle::Literal) {
        if (!trailing_newlines.empty()) {
          buffer += '\n';
        }
      } else if (had_line_break || blank_line_count > 0) {
        buffer += '\n';
      }
    }

    return Token{.type = TokenType::Scalar,
                 .value = buffer,
                 .line = start_line,
                 .column = start_column,
                 .scalar_style = style};
  }

  auto scan_plain_scalar() -> Token {
    const auto start_line{this->line_};
    const auto start_column{this->column_};
    const auto start_position{this->position_};
    const bool in_flow{this->flow_level_ > 0};

    if (in_flow) {
      const char first{this->peek()};
      if (first == '-' || first == '?' || first == ':') {
        const char after{this->peek(1)};
        if (after == '\0' || is_whitespace(after) || is_flow_indicator(after)) {
          throw YAMLParseError{start_line, start_column,
                               "Invalid plain scalar start in flow context"};
        }
      }
    }

    const std::size_t min_indent{
        in_flow
            ? 0
            : (this->block_indent_ == SIZE_MAX ? 0 : this->block_indent_ + 1)};
    bool used_multiline{false};
    std::string pending_whitespace;
    std::string *buffer{nullptr};

    while (this->position_ < this->input_.size()) {
      const auto line_start{this->position_};

      while (this->position_ < this->input_.size()) {
        const char current{this->peek()};

        if (current == ':') {
          const char after{this->peek(1)};
          if (after == '\0' || is_whitespace(after)) {
            break;
          }
          if (in_flow && is_flow_indicator(after)) {
            break;
          }
        }

        if (current == '#') {
          if (this->position_ > line_start) {
            const char before{this->input_[this->position_ - 1]};
            if (before == ' ' || before == '\t') {
              break;
            }
          }
        }

        if (in_flow && is_flow_indicator(current)) {
          break;
        }

        if (current == '\n' || current == '\r') {
          break;
        }

        this->advance(1);
      }

      auto segment_end{this->position_};
      while (segment_end > line_start &&
             (this->input_[segment_end - 1] == ' ' ||
              this->input_[segment_end - 1] == '\t')) {
        segment_end--;
      }

      const auto segment{
          this->input_.substr(line_start, segment_end - line_start)};

      if (!segment.empty()) {
        if (used_multiline) {
          *buffer += pending_whitespace;
        }
        if (buffer != nullptr) {
          *buffer += segment;
        }
        pending_whitespace.clear();
      }

      if (this->position_ >= this->input_.size()) {
        break;
      }

      const char current{this->peek()};
      if (current != '\n' && current != '\r') {
        break;
      }

      const auto saved_position{this->position_};
      const auto saved_line{this->line_};
      const auto saved_column{this->column_};

      std::size_t newline_count{0};
      std::size_t next_line_indent{0};

      while (this->position_ < this->input_.size()) {
        const char character{this->peek()};
        if (character == '\n') {
          newline_count++;
          this->advance(1);
          next_line_indent = 0;
        } else if (character == '\r') {
          newline_count++;
          this->advance(1);
          if (this->peek() == '\n') {
            this->advance(1);
          }
          next_line_indent = 0;
        } else if (character == ' ' || character == '\t') {
          next_line_indent++;
          this->advance(1);
        } else {
          break;
        }
      }

      if (this->position_ >= this->input_.size()) {
        this->position_ = saved_position;
        this->line_ = saved_line;
        this->column_ = saved_column;
        break;
      }

      if (next_line_indent < min_indent) {
        this->position_ = saved_position;
        this->line_ = saved_line;
        this->column_ = saved_column;
        break;
      }

      const char next_char{this->peek()};

      if (in_flow && is_flow_indicator(next_char)) {
        this->position_ = saved_position;
        this->line_ = saved_line;
        this->column_ = saved_column;
        break;
      }

      if (next_char == '-' || next_char == '?' || next_char == ':') {
        const char after{this->peek(1)};
        if (after == '\0' || is_whitespace(after)) {
          if (next_line_indent == 0 || start_column < 3 ||
              next_line_indent <= start_column - 3) {
            this->position_ = saved_position;
            this->line_ = saved_line;
            this->column_ = saved_column;
            break;
          }
        }
        if (in_flow && next_char == ':') {
          if (is_flow_indicator(after)) {
            this->position_ = saved_position;
            this->line_ = saved_line;
            this->column_ = saved_column;
            break;
          }
        }
      }

      if (!in_flow && this->line_contains_mapping_key()) {
        this->position_ = saved_position;
        this->line_ = saved_line;
        this->column_ = saved_column;
        break;
      }

      if (next_line_indent == 0) {
        if ((next_char == '-' && this->peek(1) == '-' &&
             this->peek(2) == '-') ||
            (next_char == '.' && this->peek(1) == '.' &&
             this->peek(2) == '.')) {
          this->position_ = saved_position;
          this->line_ = saved_line;
          this->column_ = saved_column;
          break;
        }
      }

      if (next_char == '#') {
        this->position_ = saved_position;
        this->line_ = saved_line;
        this->column_ = saved_column;
        break;
      }

      if (!used_multiline) {
        buffer = &this->get_buffer();
        *buffer =
            this->input_.substr(start_position, segment_end - start_position);
      }
      used_multiline = true;
      if (newline_count == 1) {
        pending_whitespace = " ";
      } else {
        pending_whitespace = std::string(newline_count - 1, '\n');
      }
    }

    if (used_multiline && buffer != nullptr) {
      return Token{.type = TokenType::Scalar,
                   .value = *buffer,
                   .line = start_line,
                   .column = start_column,
                   .scalar_style = ScalarStyle::Plain,
                   .multiline = true};
    }

    auto length{this->position_ - start_position};
    while (length > 0 && (this->input_[start_position + length - 1] == ' ' ||
                          this->input_[start_position + length - 1] == '\t')) {
      length--;
    }

    return Token{.type = TokenType::Scalar,
                 .value = this->input_.substr(start_position, length),
                 .line = start_line,
                 .column = start_column,
                 .scalar_style = ScalarStyle::Plain};
  }

  auto get_buffer() -> std::string & {
    this->scalar_buffers_.emplace_back();
    return this->scalar_buffers_.back();
  }

  std::string_view input_;
  std::size_t position_{0};
  std::uint64_t line_{1};
  std::uint64_t column_{1};
  std::size_t flow_level_{0};
  bool stream_started_{false};
  bool stream_ended_{false};
  bool last_was_quoted_scalar_{false};
  bool tab_at_line_start_{false};
  // SIZE_MAX means "not set" (top-level), 0 means parent at indent 0
  std::size_t block_indent_{SIZE_MAX};
  std::deque<std::string> scalar_buffers_;
};

} // namespace sourcemeta::core::yaml

#endif
