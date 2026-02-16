#ifndef SOURCEMETA_CORE_YAML_PARSER_H_
#define SOURCEMETA_CORE_YAML_PARSER_H_

#include "lexer.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/yaml_error.h>

#include <cassert>       // assert
#include <cstdint>       // std::uint64_t
#include <optional>      // std::optional
#include <sstream>       // std::ostringstream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

namespace sourcemeta::core::yaml {

struct CallbackRecord {
  JSON::ParsePhase phase;
  JSON::Type type;
  std::uint64_t line;
  std::uint64_t column;
  JSON::ParseContext context;
  std::size_t index;
  std::string property;
};

struct AnchoredValue {
  JSON value;
  std::vector<CallbackRecord> callbacks;
};

class Parser {
public:
  Parser(Lexer *lexer, const JSON::ParseCallback *callback)
      : lexer_{lexer}, callback_{callback} {}

  auto parse() -> JSON {
    std::optional<Token> token;

    if (!this->pending_tokens_.empty()) {
      token = this->pending_tokens_.front();
      this->pending_tokens_.pop_front();
      if (this->pending_tokens_.empty()) {
        this->pending_token_position_.reset();
      }
    } else {
      token = this->lexer_->next();
      if (!token.has_value() || token->type != TokenType::StreamStart) {
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Expected stream start"};
      }
      token = this->lexer_->next();
    }

    if (!token.has_value() || token->type == TokenType::StreamEnd) {
      throw YAMLParseError{1, 1, "Empty YAML document"};
    }

    if (token->type == TokenType::DirectiveYAML ||
        token->type == TokenType::DirectiveTag ||
        token->type == TokenType::DirectiveReserved) {
      this->process_directives(token.value());
    }

    if (token->type == TokenType::DocumentStart) {
      this->document_start_line_ = token->line;
      const auto pos_before_next{this->lexer_->position()};
      token = this->lexer_->next();

      if (!token.has_value() || token->type == TokenType::StreamEnd ||
          token->type == TokenType::DocumentEnd ||
          token->type == TokenType::DocumentStart) {
        if (token.has_value() && token->type == TokenType::DocumentStart) {
          this->pending_tokens_.push_back(token.value());
          this->pending_token_position_ = pos_before_next;
        }
        return JSON{nullptr};
      }
    } else if (!token.has_value() || token->type == TokenType::StreamEnd) {
      throw YAMLParseError{1, 1, "Empty YAML document"};
    } else if (token->type == TokenType::DocumentEnd) {
      while (token.has_value() && token->type == TokenType::DocumentEnd) {
        token = this->lexer_->next();
      }
      if (!token.has_value() || token->type == TokenType::StreamEnd) {
        throw YAMLParseError{1, 1, "Empty YAML document"};
      }
      this->pending_tokens_.push_back(token.value());
      return JSON{nullptr};
    }

    auto result{this->parse_value(token.value(), JSON::ParseContext::Root, 0,
                                  std::string_view{})};

    auto pos_before_token{this->lexer_->position()};
    token = this->next_token();
    while (token.has_value() && token->type == TokenType::DocumentEnd) {
      pos_before_token = this->lexer_->position();
      token = this->next_token();
    }

    if (token.has_value() && token->type != TokenType::StreamEnd) {
      this->pending_tokens_.push_back(token.value());
      if (token->type == TokenType::DocumentStart) {
        this->pending_token_position_ = token->position;
      } else {
        this->pending_token_position_ = pos_before_token;
      }
    }

    return result;
  }

  [[nodiscard]] auto position() const noexcept -> std::size_t {
    if (this->pending_token_position_.has_value()) {
      return *this->pending_token_position_;
    }
    return this->lexer_->position();
  }

  auto validate_end_of_stream() -> void {
    auto token{this->next_token()};
    bool saw_document_end{false};
    while (token.has_value() && token->type == TokenType::DocumentEnd) {
      saw_document_end = true;
      token = this->next_token();
    }
    if (!token.has_value() || token->type == TokenType::StreamEnd) {
      return;
    }
    while (token.has_value() && token->type != TokenType::StreamEnd) {
      if (token->type == TokenType::DocumentStart) {
        this->tag_directives_.clear();
        token = this->next_token();
        if (!token.has_value() || token->type == TokenType::StreamEnd) {
          return;
        }
        if (token->type == TokenType::DocumentEnd ||
            token->type == TokenType::DocumentStart) {
          continue;
        }
      }
      if (token->type == TokenType::DirectiveYAML ||
          token->type == TokenType::DirectiveTag ||
          token->type == TokenType::DirectiveReserved) {
        if (!saw_document_end) {
          throw YAMLParseError{token->line, token->column,
                               "Directive not allowed without preceding "
                               "document end marker"};
        }
        this->process_directives(token.value());
        continue;
      }
      if (!saw_document_end && token->type != TokenType::DocumentStart) {
        throw YAMLParseError{token->line, token->column,
                             "Unexpected content after document"};
      }
      this->parse_value(token.value(), JSON::ParseContext::Root, 0,
                        std::string_view{});
      saw_document_end = false;
      token = this->next_token();
      while (token.has_value() && token->type == TokenType::DocumentEnd) {
        saw_document_end = true;
        token = this->next_token();
      }
    }
  }

private:
  auto process_directives(Token &token) -> void {
    bool seen_yaml_directive{false};
    while (token.type == TokenType::DirectiveYAML ||
           token.type == TokenType::DirectiveTag ||
           token.type == TokenType::DirectiveReserved) {
      if (token.type == TokenType::DirectiveYAML) {
        if (seen_yaml_directive) {
          throw YAMLParseError{token.line, token.column,
                               "Duplicate %YAML directive"};
        }
        seen_yaml_directive = true;
        const auto content{token.value};
        auto cursor{static_cast<std::size_t>(5)};
        while (cursor < content.size() &&
               (content[cursor] == ' ' || content[cursor] == '\t')) {
          cursor++;
        }
        while (cursor < content.size() && content[cursor] != ' ' &&
               content[cursor] != '\t' && content[cursor] != '#') {
          cursor++;
        }
        while (cursor < content.size() &&
               (content[cursor] == ' ' || content[cursor] == '\t')) {
          cursor++;
        }
        if (cursor < content.size() && content[cursor] != '#') {
          throw YAMLParseError{token.line, token.column,
                               "Invalid content in %YAML directive"};
        }
      } else if (token.type == TokenType::DirectiveTag) {
        const auto content{token.value};
        auto cursor{static_cast<std::size_t>(4)};
        while (cursor < content.size() &&
               (content[cursor] == ' ' || content[cursor] == '\t')) {
          cursor++;
        }
        const auto handle_start{cursor};
        while (cursor < content.size() && content[cursor] != ' ' &&
               content[cursor] != '\t') {
          cursor++;
        }
        const auto handle{
            std::string{content.substr(handle_start, cursor - handle_start)}};
        while (cursor < content.size() &&
               (content[cursor] == ' ' || content[cursor] == '\t')) {
          cursor++;
        }
        const auto prefix_start{cursor};
        while (cursor < content.size() && content[cursor] != ' ' &&
               content[cursor] != '\t' && content[cursor] != '\n' &&
               content[cursor] != '\r') {
          cursor++;
        }
        const auto prefix{
            std::string{content.substr(prefix_start, cursor - prefix_start)}};
        if (!handle.empty() && !prefix.empty()) {
          this->tag_directives_.insert_or_assign(handle, prefix);
        }
      }
      auto next{this->lexer_->next()};
      if (!next.has_value()) {
        break;
      }
      token = next.value();
    }
  }

  auto resolve_tag(const std::string_view raw_tag) -> std::string {
    if (raw_tag.size() > 2 && raw_tag[0] == '!' && raw_tag[1] == '<' &&
        raw_tag.back() == '>') {
      return std::string{raw_tag.substr(2, raw_tag.size() - 3)};
    }

    if (raw_tag.starts_with("!!")) {
      const auto iterator{this->tag_directives_.find("!!")};
      if (iterator != this->tag_directives_.end()) {
        return iterator->second + std::string{raw_tag.substr(2)};
      }
      return "tag:yaml.org,2002:" + std::string{raw_tag.substr(2)};
    }

    if (raw_tag.size() > 1 && raw_tag[0] == '!') {
      const auto second_bang{raw_tag.find('!', 1)};
      if (second_bang != std::string_view::npos &&
          second_bang < raw_tag.size() - 1) {
        const auto handle{std::string{raw_tag.substr(0, second_bang + 1)}};
        const auto iterator{this->tag_directives_.find(handle)};
        if (iterator != this->tag_directives_.end()) {
          return iterator->second +
                 std::string{raw_tag.substr(second_bang + 1)};
        }
      }
    }

    return std::string{raw_tag};
  }

  auto invoke_callback(const JSON::ParsePhase phase, const JSON::Type type,
                       const std::uint64_t line, const std::uint64_t column,
                       const JSON::ParseContext context,
                       const std::size_t index, const std::string_view property)
      -> void {
    if (this->callback_ && *this->callback_) {
      (*this->callback_)(phase, type, line, column, context, index, property);
    }

    if (this->recording_anchor_) {
      this->current_anchor_callbacks_.push_back(
          {phase, type, line, column, context, index, std::string{property}});
    }
  }

  [[nodiscard]] auto effective_line(const Token &token,
                                    const JSON::ParseContext context,
                                    const std::uint64_t key_line) const
      -> std::uint64_t {
    return (context == JSON::ParseContext::Property && key_line > 0)
               ? key_line
               : token.line;
  }

  [[nodiscard]] auto effective_column(const Token &token,
                                      const JSON::ParseContext context,
                                      const std::uint64_t key_column) const
      -> std::uint64_t {
    return (context == JSON::ParseContext::Property && key_column > 0)
               ? key_column
               : token.column;
  }

  [[nodiscard]] auto json_to_key_string(const JSON &value) const
      -> std::string {
    if (value.is_string()) {
      return value.to_string();
    }
    if (value.is_null()) {
      return "";
    }
    std::ostringstream stream;
    stream << value;
    return stream.str();
  }

  auto parse_value(const Token &token, const JSON::ParseContext context,
                   const std::size_t index, const std::string_view property,
                   const std::uint64_t key_line = 0,
                   const std::uint64_t key_column = 0) -> JSON {
    std::optional<std::string_view> anchor_name;
    std::uint64_t anchor_line{0};
    std::optional<std::string> tag;
    std::size_t anchor_count{0};
    Token current_token{token};
    std::uint64_t node_start_column{token.column};
    std::uint64_t prefix_line{token.line};

    while (current_token.type == TokenType::Anchor ||
           current_token.type == TokenType::Tag) {
      if (this->lexer_->flow_level() == 0 &&
          context == JSON::ParseContext::Property && key_line > 0 &&
          current_token.line != key_line) {
        const auto value_indent{
            current_token.column > 0
                ? static_cast<std::size_t>(current_token.column - 1)
                : static_cast<std::size_t>(0)};
        const auto parent_indent{this->lexer_->block_indent()};
        if (parent_indent != SIZE_MAX && value_indent <= parent_indent) {
          throw YAMLParseError{current_token.line, current_token.column,
                               "Node property at wrong indentation level"};
        }
      }
      if (current_token.type == TokenType::Anchor) {
        anchor_name = current_token.value;
        anchor_line = current_token.line;
        anchor_count++;
      } else {
        tag = this->resolve_tag(current_token.value);
      }

      auto next{this->lexer_->next()};
      if (!next.has_value() || next->type == TokenType::StreamEnd ||
          next->type == TokenType::DocumentEnd ||
          next->type == TokenType::DocumentStart) {
        JSON empty_value{nullptr};
        if (tag.has_value()) {
          if (tag.value() == "tag:yaml.org,2002:str") {
            empty_value = JSON{std::string{}};
          }
        }
        if (next.has_value()) {
          this->pending_tokens_.push_back(next.value());
        }
        return empty_value;
      }
      current_token = next.value();

      if (current_token.type == TokenType::Scalar &&
          current_token.column <= key_column && key_column > 0) {
        auto after{this->lexer_->next()};
        if (after.has_value() && after->type == TokenType::BlockMappingValue) {
          this->pending_tokens_.push_back(current_token);
          this->pending_tokens_.push_back(after.value());
          if (anchor_name.has_value()) {
            this->recording_anchor_ = true;
            this->current_anchor_callbacks_.clear();
            JSON null_value{nullptr};
            this->invoke_callback(JSON::ParsePhase::Pre, JSON::Type::Null,
                                  token.line, token.column, context, index,
                                  property);
            this->invoke_callback(
                JSON::ParsePhase::Post, JSON::Type::Null, token.line,
                token.column, JSON::ParseContext::Root, 0, std::string_view{});
            this->recording_anchor_ = false;
            this->anchors_.insert_or_assign(
                std::string{anchor_name.value()},
                AnchoredValue{.value = null_value,
                              .callbacks =
                                  std::move(this->current_anchor_callbacks_)});
            this->current_anchor_callbacks_.clear();
          }
          return JSON{nullptr};
        }
        if (after.has_value()) {
          this->pending_tokens_.push_back(after.value());
        }
      }
    }

    if (tag.has_value() && (current_token.type == TokenType::FlowEntry ||
                            current_token.type == TokenType::MappingEnd ||
                            current_token.type == TokenType::SequenceEnd)) {
      JSON empty_value{nullptr};
      if (tag.value() == "tag:yaml.org,2002:str") {
        empty_value = JSON{std::string{}};
      }
      this->pending_tokens_.push_back(current_token);
      return empty_value;
    }

    if (current_token.line != prefix_line) {
      node_start_column = 0;
    }

    if ((anchor_name.has_value() || tag.has_value()) &&
        this->lexer_->flow_level() == 0 && current_token.line == prefix_line &&
        current_token.type == TokenType::BlockSequenceEntry) {
      throw YAMLParseError{current_token.line, current_token.column,
                           "Block sequence after node property must start "
                           "on a new line"};
    }

    if (anchor_name.has_value()) {
      this->recording_anchor_ = true;
      this->current_anchor_callbacks_.clear();
    }

    JSON result{nullptr};

    switch (current_token.type) {
      case TokenType::Scalar: {
        auto next{this->next_token()};
        if (next.has_value() && next->type == TokenType::BlockMappingValue) {
          if (current_token.multiline) {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Multi-line implicit mapping key"};
          }
          if (this->lexer_->flow_level() > 0 &&
              next->line != current_token.line) {
            throw YAMLParseError{next->line, next->column,
                                 "Implicit key and value indicator on "
                                 "different lines in flow context"};
          }
          if (this->lexer_->flow_level() == 0 &&
              context == JSON::ParseContext::Property && key_line > 0 &&
              current_token.line == key_line) {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Implicit mapping key in block value on "
                                 "same line as parent key"};
          }
          if (this->lexer_->flow_level() == 0 &&
              (anchor_name.has_value() || tag.has_value()) &&
              this->document_start_line_ > 0 &&
              current_token.line == this->document_start_line_) {
            throw YAMLParseError{
                current_token.line, current_token.column,
                "Node properties before implicit mapping key on "
                "document start line"};
          }
          if (anchor_name.has_value() && anchor_line == current_token.line) {
            JSON key_value{std::string{current_token.value}};
            this->recording_anchor_ = false;
            this->anchors_.insert_or_assign(
                std::string{anchor_name.value()},
                AnchoredValue{.value = key_value,
                              .callbacks =
                                  std::move(this->current_anchor_callbacks_)});
            this->current_anchor_callbacks_.clear();
            anchor_name.reset();
          }
          result = this->parse_block_mapping_from_first_key(
              current_token, context, index, property, key_line, key_column,
              node_start_column);
        } else {
          if (anchor_count > 1) {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Multiple anchors on a scalar node"};
          }
          result = this->parse_scalar(current_token, tag, context, index,
                                      property, key_line, key_column);
          if (next.has_value()) {
            this->pending_tokens_.push_back(next.value());
          }
        }
        break;
      }
      case TokenType::MappingStart:
        result = this->parse_flow_mapping(current_token, context, index,
                                          property, key_line, key_column);
        break;
      case TokenType::SequenceStart:
        result = this->parse_flow_sequence(current_token, context, index,
                                           property, key_line, key_column);
        break;
      case TokenType::BlockSequenceEntry:
        result = this->parse_block_sequence(current_token, context, index,
                                            property, key_line, key_column);
        break;
      case TokenType::BlockMappingKey:
      case TokenType::BlockMappingValue:
        result = this->parse_block_mapping(current_token, context, index,
                                           property, key_line, key_column);
        break;
      case TokenType::Alias: {
        auto next{this->next_token()};
        if (next.has_value() && next->type == TokenType::BlockMappingValue) {
          const std::string alias_name{current_token.value};
          const auto iterator{this->anchors_.find(alias_name)};
          if (iterator == this->anchors_.end()) {
            throw YAMLUnknownAnchorError{alias_name, current_token.line,
                                         current_token.column};
          }
          const auto key_string{
              this->json_to_key_string(iterator->second.value)};
          Token key_token{current_token};
          key_token.type = TokenType::Scalar;
          key_token.value = key_string;
          result = this->parse_block_mapping_from_first_key(
              key_token, context, index, property, key_line, key_column,
              node_start_column);
        } else {
          if (anchor_name.has_value()) {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Cannot anchor an alias node"};
          }
          result = this->resolve_alias(current_token, context, index, property,
                                       key_line, key_column);
          if (next.has_value()) {
            this->pending_tokens_.push_back(next.value());
          }
        }
        break;
      }
      default:
        throw YAMLParseError{current_token.line, current_token.column,
                             "Unexpected token"};
    }

    if (anchor_name.has_value()) {
      this->recording_anchor_ = false;
      this->anchors_.insert_or_assign(
          std::string{anchor_name.value()},
          AnchoredValue{.value = result,
                        .callbacks =
                            std::move(this->current_anchor_callbacks_)});
      this->current_anchor_callbacks_.clear();
    }

    return result;
  }

  auto parse_scalar(const Token &token, const std::optional<std::string> &tag,
                    const JSON::ParseContext context, const std::size_t index,
                    const std::string_view property,
                    const std::uint64_t key_line = 0,
                    const std::uint64_t key_column = 0) -> JSON {
    JSON result{this->interpret_scalar(token.value, token.scalar_style, tag)};

    this->invoke_callback(JSON::ParsePhase::Pre, result.type(),
                          this->effective_line(token, context, key_line),
                          this->effective_column(token, context, key_column),
                          context, index, property);

    auto end_column{token.column};
    if (!token.value.empty()) {
      end_column += static_cast<std::uint64_t>(token.value.size()) - 1;
    }
    if (token.scalar_style == ScalarStyle::SingleQuoted ||
        token.scalar_style == ScalarStyle::DoubleQuoted) {
      end_column += 2;
    }

    this->invoke_callback(JSON::ParsePhase::Post, result.type(), token.line,
                          end_column, JSON::ParseContext::Root, 0,
                          std::string_view{});

    return result;
  }

  auto interpret_scalar(const std::string_view value, const ScalarStyle style,
                        const std::optional<std::string> &tag) -> JSON {
    if (tag.has_value()) {
      const auto &tag_value{tag.value()};
      if (tag_value == "!" || tag_value == "tag:yaml.org,2002:str") {
        return JSON{std::string{value}};
      }
      if (tag_value == "tag:yaml.org,2002:null") {
        return JSON{nullptr};
      }
      if (tag_value == "tag:yaml.org,2002:bool") {
        if (value == "true" || value == "True" || value == "TRUE") {
          return JSON{true};
        }
        return JSON{false};
      }
      if (tag_value == "tag:yaml.org,2002:int") {
        return this->parse_integer(value);
      }
      if (tag_value == "tag:yaml.org,2002:float") {
        return this->parse_float(value);
      }
      return JSON{std::string{value}};
    }

    if (style != ScalarStyle::Plain) {
      return JSON{std::string{value}};
    }

    if (value.empty()) {
      return JSON{nullptr};
    }

    if (value == "null" || value == "Null" || value == "NULL" || value == "~") {
      return JSON{nullptr};
    }

    if (value == "true" || value == "True" || value == "TRUE") {
      return JSON{true};
    }

    if (value == "false" || value == "False" || value == "FALSE") {
      return JSON{false};
    }

    if (value == ".inf" || value == ".Inf" || value == ".INF" ||
        value == "+.inf" || value == "+.Inf" || value == "+.INF" ||
        value == "-.inf" || value == "-.Inf" || value == "-.INF" ||
        value == ".nan" || value == ".NaN" || value == ".NAN") {
      return JSON{std::string{value}};
    }

    if (this->looks_like_number(value)) {
      return this->parse_number(value);
    }

    return JSON{std::string{value}};
  }

  [[nodiscard]] auto looks_like_number(const std::string_view value) const
      -> bool {
    if (value.empty()) {
      return false;
    }

    std::size_t start{0};
    if (value[0] == '-' || value[0] == '+') {
      start = 1;
      if (start >= value.size()) {
        return false;
      }
    }

    if (value.size() > start + 1 && value[start] == '0') {
      if (value[start + 1] == 'x' || value[start + 1] == 'X') {
        return true;
      }
      if (value[start + 1] == 'o' || value[start + 1] == 'O') {
        return true;
      }
    }

    bool has_digit{false};
    bool has_dot{false};
    bool has_exp{false};

    for (std::size_t index = start; index < value.size(); ++index) {
      const char current{value[index]};
      if (current >= '0' && current <= '9') {
        has_digit = true;
      } else if (current == '.') {
        if (has_dot || has_exp) {
          return false;
        }
        has_dot = true;
      } else if (current == 'e' || current == 'E') {
        if (has_exp || !has_digit) {
          return false;
        }
        has_exp = true;
        if (index + 1 < value.size() &&
            (value[index + 1] == '+' || value[index + 1] == '-')) {
          ++index;
        }
      } else {
        return false;
      }
    }

    return has_digit;
  }

  auto parse_number(const std::string_view value) -> JSON {
    const std::size_t prefix{(value[0] == '-' || value[0] == '+') ? 1u : 0u};
    if (value.size() > prefix + 1 && value[prefix] == '0') {
      const char indicator{value[prefix + 1]};
      if (indicator == 'x' || indicator == 'X') {
        return this->parse_base_integer(value, 16);
      }
      if (indicator == 'o' || indicator == 'O') {
        return this->parse_base_integer(value, 8);
      }
    }

    bool has_dot{false};
    bool has_exp{false};
    for (const char character : value) {
      if (character == '.') {
        has_dot = true;
      }
      if (character == 'e' || character == 'E') {
        has_exp = true;
      }
    }

    if (has_exp) {
      return JSON{Decimal{std::string{value}}};
    }

    if (has_dot) {
      return this->parse_float(value);
    }

    return this->parse_integer(value);
  }

  auto parse_integer(const std::string_view value) -> JSON {
    const auto result{to_int64_t(std::string{value})};
    return result.has_value() ? JSON{result.value()}
                              : JSON{Decimal{std::string{value}}};
  }

  auto parse_base_integer(const std::string_view value, const int base)
      -> JSON {
    const bool negative{value[0] == '-'};
    const std::size_t start{(value[0] == '-' || value[0] == '+') ? 3u : 2u};
    const auto result{to_int64_t(std::string{value.substr(start)}, base)};
    if (result.has_value()) {
      return JSON{negative ? -result.value() : result.value()};
    }
    return JSON{std::string{value}};
  }

  auto parse_float(const std::string_view value) -> JSON {
    std::size_t significant_digits{0};
    bool seen_nonzero{false};
    for (const char character : value) {
      if (character >= '0' && character <= '9') {
        if (character != '0' || seen_nonzero) {
          seen_nonzero = true;
          significant_digits++;
        }
      }
    }

    constexpr std::size_t double_precision_limit{15};
    if (significant_digits > double_precision_limit) {
      return JSON{Decimal{std::string{value}}};
    }

    const auto result{to_double(std::string{value})};
    if (!result.has_value()) {
      return JSON{Decimal{std::string{value}}};
    }

    const auto as_integer{static_cast<std::int64_t>(result.value())};
    if (result.value() == static_cast<double>(as_integer)) {
      return JSON{as_integer};
    }

    return JSON{result.value()};
  }

  auto parse_flow_mapping(const Token &start_token,
                          const JSON::ParseContext context,
                          const std::size_t index,
                          const std::string_view property,
                          const std::uint64_t key_line = 0,
                          const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Object,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_object()};
    std::unordered_set<std::string> seen_keys;

    auto token{this->next_token()};

    while (token.has_value() && token->type != TokenType::MappingEnd) {
      if (token->type == TokenType::FlowEntry) {
        token = this->next_token();
        continue;
      }

      auto key_token{token.value()};

      std::optional<std::string> key_tag;
      while (key_token.type == TokenType::Anchor ||
             key_token.type == TokenType::Tag) {
        if (key_token.type == TokenType::Tag) {
          key_tag = this->resolve_tag(key_token.value);
        }
        token = this->next_token();
        if (!token.has_value()) {
          throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                               "Unexpected end of input in flow mapping"};
        }
        key_token = token.value();
      }

      std::string key;
      if (key_token.type == TokenType::BlockMappingValue) {
        if (key_tag.has_value() && key_tag.value() == "tag:yaml.org,2002:str") {
          key = "";
        }
      } else if (key_token.type == TokenType::Scalar) {
        key = std::string{key_token.value};
      } else {
        throw YAMLParseError{key_token.line, key_token.column,
                             "Expected scalar key in mapping"};
      }

      if (seen_keys.contains(key)) {
        throw YAMLDuplicateKeyError{key, key_token.line, key_token.column};
      }
      seen_keys.insert(key);

      if (key_token.type != TokenType::BlockMappingValue) {
        token = this->next_token();

        if (!token.has_value()) {
          throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                               "Unexpected end of input in flow mapping"};
        }

        if (token->type == TokenType::FlowEntry ||
            token->type == TokenType::MappingEnd) {
          result.assign(key, JSON{nullptr});
          continue;
        }

        if (token->type != TokenType::BlockMappingValue) {
          const auto colon_column{key_token.column +
                                  static_cast<std::uint64_t>(key.size())};
          throw YAMLParseError{key_token.line, colon_column,
                               "Expected ':' after mapping key"};
        }
      }

      token = this->next_token();
      if (!token.has_value()) {
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Expected value after ':'"};
      }

      if (token->type == TokenType::FlowEntry ||
          token->type == TokenType::MappingEnd) {
        result.assign(key, JSON{nullptr});
      } else {
        auto value{this->parse_value(token.value(),
                                     JSON::ParseContext::Property, 0, key,
                                     key_token.line, key_token.column)};
        result.assign(key, std::move(value));
      }

      if (token->type != TokenType::FlowEntry &&
          token->type != TokenType::MappingEnd) {
        token = this->next_token();
        if (token.has_value() && token->type != TokenType::FlowEntry &&
            token->type != TokenType::MappingEnd) {
          throw YAMLParseError{token->line, token->column,
                               "Missing comma between flow mapping entries"};
        }
      }
    }

    const auto end_line{token.has_value() ? token->line : this->lexer_->line()};
    const auto end_column{token.has_value() ? token->column
                                            : this->lexer_->column()};
    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Object, end_line,
                          end_column, JSON::ParseContext::Root, 0,
                          std::string_view{});

    return result;
  }

  auto parse_flow_sequence(const Token &start_token,
                           const JSON::ParseContext context,
                           const std::size_t index,
                           const std::string_view property,
                           const std::uint64_t key_line = 0,
                           const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Array,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_array()};
    const auto parent_block_indent{this->lexer_->block_indent()};

    auto token{this->next_token()};
    std::size_t element_index{0};

    while (token.has_value() && token->type != TokenType::SequenceEnd) {
      if (parent_block_indent != SIZE_MAX && token->line != start_token.line) {
        const auto token_indent{
            token->column > 0 ? static_cast<std::size_t>(token->column - 1)
                              : static_cast<std::size_t>(0)};
        if (token_indent <= parent_block_indent) {
          throw YAMLParseError{
              token->line, token->column,
              "Flow content indented less than or equal to parent block level"};
        }
      }
      if (token->type == TokenType::FlowEntry) {
        if (element_index == 0) {
          throw YAMLParseError{token->line, token->column,
                               "Leading comma in flow sequence"};
        }
        token = this->next_token();
        if (token.has_value() && token->type == TokenType::FlowEntry) {
          throw YAMLParseError{token->line, token->column,
                               "Empty entry in flow sequence"};
        }
        continue;
      }

      if (token->type == TokenType::BlockMappingKey) {
        auto mapping{JSON::make_object()};
        token = this->next_token();
        if (!token.has_value()) {
          throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                               "Unexpected end after explicit key in flow"};
        }

        std::string key_string;
        if (token->type == TokenType::Scalar) {
          key_string = std::string{token->value};
          token = this->next_token();
        } else {
          // For non-scalar keys, parse the value and stringify
          auto key_value{this->parse_value(token.value(),
                                           JSON::ParseContext::Index,
                                           element_index, std::string_view{})};
          key_string = this->json_to_key_string(key_value);
          token = this->next_token();
        }

        if (token.has_value() && token->type == TokenType::BlockMappingValue) {
          token = this->next_token();
          if (token.has_value() && token->type != TokenType::SequenceEnd &&
              token->type != TokenType::FlowEntry) {
            auto value{this->parse_value(
                token.value(), JSON::ParseContext::Property, 0, key_string)};
            mapping.assign(key_string, std::move(value));
            token = this->next_token();
          } else {
            mapping.assign(key_string, JSON{nullptr});
          }
        } else {
          mapping.assign(key_string, JSON{nullptr});
        }
        result.push_back(std::move(mapping));
        element_index++;
        continue;
      }

      auto value{this->parse_value(token.value(), JSON::ParseContext::Index,
                                   element_index, std::string_view{})};
      result.push_back(std::move(value));
      element_index++;

      token = this->next_token();
      if (token.has_value() && token->type != TokenType::FlowEntry &&
          token->type != TokenType::SequenceEnd) {
        throw YAMLParseError{token->line, token->column,
                             "Missing comma in flow sequence"};
      }
    }

    const auto end_line{token.has_value() ? token->line : this->lexer_->line()};
    const auto end_column{token.has_value() ? token->column
                                            : this->lexer_->column()};
    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Array, end_line,
                          end_column, JSON::ParseContext::Root, 0,
                          std::string_view{});

    return result;
  }

  auto parse_block_sequence(const Token &start_token,
                            const JSON::ParseContext context,
                            const std::size_t index,
                            const std::string_view property,
                            const std::uint64_t key_line = 0,
                            const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Array,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_array()};
    std::size_t element_index{0};
    const auto base_column{start_token.column};
    const auto sequence_indent{base_column > 0
                                   ? static_cast<std::size_t>(base_column - 1)
                                   : static_cast<std::size_t>(0)};
    this->lexer_->set_block_indent(sequence_indent);

    auto token{this->next_token()};

    if (token.has_value() && token->type != TokenType::BlockSequenceEntry &&
        token->type != TokenType::StreamEnd &&
        token->type != TokenType::DocumentEnd &&
        token->type != TokenType::DocumentStart) {
      auto value{this->parse_value(token.value(), JSON::ParseContext::Index,
                                   element_index, std::string_view{})};
      result.push_back(std::move(value));
      element_index++;
      token = this->next_token();
    } else if (token.has_value() &&
               token->type == TokenType::BlockSequenceEntry &&
               token->column == base_column) {
      result.push_back(JSON{nullptr});
      element_index++;
    }

    while (token.has_value() && token->type == TokenType::BlockSequenceEntry &&
           token->column >= base_column) {
      this->lexer_->set_block_indent(sequence_indent);

      if (token->column > base_column) {
        if (token->column < base_column + 2) {
          throw YAMLParseError{token->line, token->column,
                               "Wrong indentation for sequence entry"};
        }
        auto value{this->parse_value(token.value(), JSON::ParseContext::Index,
                                     element_index, std::string_view{})};
        result.push_back(std::move(value));
        element_index++;
        token = this->next_token();
        continue;
      }

      token = this->next_token();

      if (!token.has_value() ||
          (token->type == TokenType::BlockSequenceEntry &&
           token->column == base_column) ||
          token->type == TokenType::StreamEnd ||
          token->type == TokenType::DocumentEnd) {
        result.push_back(JSON{nullptr});
      } else {
        auto value{this->parse_value(token.value(), JSON::ParseContext::Index,
                                     element_index, std::string_view{})};
        result.push_back(std::move(value));
        token = this->next_token();
      }

      element_index++;
    }

    std::uint64_t end_line{this->lexer_->line()};
    std::uint64_t end_column{this->lexer_->column()};

    if (token.has_value()) {
      this->pending_tokens_.push_back(token.value());
      end_line = token->line;
      end_column = 0;
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Array, end_line,
                          end_column, JSON::ParseContext::Root, 0,
                          std::string_view{});

    return result;
  }

  auto parse_block_mapping(const Token &start_token,
                           const JSON::ParseContext context,
                           const std::size_t index,
                           const std::string_view property,
                           const std::uint64_t key_line = 0,
                           const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Object,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_object()};
    std::unordered_set<std::string_view> seen_keys;

    auto token{start_token};
    const auto mapping_indent{
        start_token.column > 0
            ? static_cast<std::size_t>(start_token.column - 1)
            : static_cast<std::size_t>(0)};

    while (true) {
      this->lexer_->set_block_indent(mapping_indent);

      if (token.type == TokenType::BlockMappingKey) {
        auto next{this->next_token()};
        assert(next.has_value());
        token = next.value();
      }

      while (token.type == TokenType::Tag || token.type == TokenType::Anchor) {
        auto next{this->next_token()};
        assert(next.has_value());
        token = next.value();
      }

      if (token.type != TokenType::Scalar &&
          token.type != TokenType::BlockMappingValue) {
        break;
      }

      std::string_view key;
      std::uint64_t current_key_line{0};
      std::uint64_t current_key_column{0};

      if (token.type == TokenType::Scalar) {
        key = token.value;
        current_key_line = token.line;
        current_key_column = token.column;

        if (seen_keys.contains(key)) {
          throw YAMLDuplicateKeyError{key, token.line, token.column};
        }
        seen_keys.insert(key);

        auto next{this->next_token()};
        if (!next.has_value() || next->type != TokenType::BlockMappingValue) {
          result.assign(std::string{key}, JSON{nullptr});
          if (!next.has_value()) {
            break;
          }
          token = next.value();
          continue;
        }
        token = next.value();
      }

      if (token.type == TokenType::BlockMappingValue) {
        auto next{this->next_token()};

        if (!next.has_value() || next->type == TokenType::StreamEnd ||
            next->type == TokenType::DocumentEnd ||
            next->type == TokenType::DocumentStart) {
          result.assign(std::string{key}, JSON{nullptr});
          if (!next.has_value()) {
            break;
          }
          token = next.value();
          continue;
        }

        if (next->type == TokenType::BlockMappingValue ||
            next->type == TokenType::BlockMappingKey) {
          if (key.empty() && next->type == TokenType::BlockMappingKey) {
            token = next.value();
            continue;
          }
          result.assign(std::string{key}, JSON{nullptr});
          token = next.value();
          continue;
        }

        if (key.empty() && next->type == TokenType::Scalar) {
          key = next->value;
          if (seen_keys.contains(key)) {
            throw YAMLDuplicateKeyError{key, next->line, next->column};
          }
          seen_keys.insert(key);
          result.assign(std::string{key}, JSON{nullptr});
          auto next_after_key{this->next_token()};
          assert(next_after_key.has_value());
          token = next_after_key.value();
          continue;
        }

        auto value{this->parse_value(next.value(), JSON::ParseContext::Property,
                                     0, key, current_key_line,
                                     current_key_column)};
        result.assign(std::string{key}, std::move(value));

        auto after{this->next_token()};
        if (!after.has_value()) {
          break;
        }
        token = after.value();
      }
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Object,
                          this->lexer_->line(), this->lexer_->column(),
                          JSON::ParseContext::Root, 0, std::string_view{});

    return result;
  }

  auto resolve_alias(const Token &token, const JSON::ParseContext context,
                     const std::size_t index, const std::string_view property,
                     const std::uint64_t key_line = 0,
                     const std::uint64_t key_column = 0) -> JSON {
    const std::string anchor_name{token.value};
    const auto iterator{this->anchors_.find(anchor_name)};

    if (iterator == this->anchors_.end()) {
      throw YAMLUnknownAnchorError{anchor_name, token.line, token.column};
    }

    const auto &anchored{iterator->second};
    const auto alias_end_column{token.column +
                                static_cast<std::uint64_t>(token.value.size())};

    bool is_first_pre{true};
    bool is_last_post{false};
    std::size_t callback_index{0};
    for (const auto &record : anchored.callbacks) {
      is_last_post = (callback_index == anchored.callbacks.size() - 1 &&
                      record.phase == JSON::ParsePhase::Post);

      std::uint64_t callback_line{record.line};
      std::uint64_t callback_column{record.column};
      auto callback_context{record.context};
      auto callback_idx{record.index};
      std::string_view callback_property{record.property};

      if (is_first_pre && record.phase == JSON::ParsePhase::Pre) {
        if (context == JSON::ParseContext::Property && key_line > 0) {
          callback_line = key_line;
          callback_column = key_column;
        }
        callback_context = context;
        callback_idx = index;
        callback_property = property;
        is_first_pre = false;
      }

      if (is_last_post) {
        callback_line = token.line;
        callback_column = alias_end_column;
        callback_context = JSON::ParseContext::Root;
        callback_idx = 0;
        callback_property = std::string_view{};
      }

      this->invoke_callback(record.phase, record.type, callback_line,
                            callback_column, callback_context, callback_idx,
                            callback_property);
      callback_index++;
    }

    return anchored.value;
  }

  auto next_token() -> std::optional<Token> {
    std::optional<Token> result;
    if (!this->pending_tokens_.empty()) {
      result = this->pending_tokens_.front();
      this->pending_tokens_.pop_front();
      if (this->pending_tokens_.empty()) {
        this->pending_token_position_.reset();
      }
    } else {
      result = this->lexer_->next();
    }
    return result;
  }

  auto parse_block_mapping_from_first_key(
      const Token &key_token, const JSON::ParseContext context,
      const std::size_t index, const std::string_view property,
      const std::uint64_t parent_key_line = 0,
      const std::uint64_t parent_key_column = 0,
      const std::uint64_t node_start_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Object,
        this->effective_line(key_token, context, parent_key_line),
        this->effective_column(key_token, context, parent_key_column), context,
        index, property);

    JSON result{JSON::make_object()};
    std::unordered_set<std::string> seen_keys;
    const auto base_column{node_start_column > 0 ? node_start_column
                                                 : key_token.column};

    std::string key{key_token.value};
    std::uint64_t key_line{key_token.line};
    std::uint64_t key_column{key_token.column};
    const auto first_key_line{key_token.line};
    seen_keys.insert(key);

    this->lexer_->set_block_indent(static_cast<std::size_t>(base_column - 1));
    auto next{this->next_token()};

    if (!next.has_value() || next->type == TokenType::Scalar ||
        next->type == TokenType::StreamEnd ||
        next->type == TokenType::DocumentEnd) {
      if (next.has_value() && next->type == TokenType::Scalar) {
        auto value{this->parse_value(next.value(), JSON::ParseContext::Property,
                                     0, key, key_line, key_column)};
        result.assign(std::string{key}, std::move(value));
        next = this->next_token();
      } else {
        this->invoke_callback(JSON::ParsePhase::Pre, JSON::Type::Null, key_line,
                              key_column, JSON::ParseContext::Property, 0, key);
        const auto null_post_column{key_column +
                                    static_cast<std::uint64_t>(key.size())};
        this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Null,
                              key_line, null_post_column,
                              JSON::ParseContext::Root, 0, std::string_view{});
        result.assign(std::string{key}, JSON{nullptr});
      }
    } else if (next->type == TokenType::MappingStart ||
               next->type == TokenType::SequenceStart ||
               next->type == TokenType::BlockSequenceEntry ||
               next->type == TokenType::Anchor ||
               next->type == TokenType::Tag || next->type == TokenType::Alias) {
      if (next->type == TokenType::BlockSequenceEntry &&
          next->line == key_line) {
        throw YAMLParseError{
            next->line, next->column,
            "Block sequence entry on same line as mapping key"};
      }
      auto value{this->parse_value(next.value(), JSON::ParseContext::Property,
                                   0, key, key_line, key_column)};
      result.assign(std::string{key}, std::move(value));
      next = this->next_token();
    } else {
      result.assign(std::string{key}, JSON{nullptr});
    }

    while (next.has_value() &&
           (next->type == TokenType::Scalar ||
            next->type == TokenType::BlockMappingKey ||
            next->type == TokenType::Anchor || next->type == TokenType::Tag ||
            next->type == TokenType::Alias)) {
      if (this->document_start_line_ > 0 &&
          first_key_line == this->document_start_line_ &&
          next->line != this->document_start_line_) {
        throw YAMLParseError{next->line, next->column,
                             "Block mapping continuation after document "
                             "start line"};
      }
      this->lexer_->set_block_indent(static_cast<std::size_t>(base_column - 1));

      if (next->type == TokenType::BlockMappingKey) {
        if (next->column < base_column) {
          break;
        }
        next = this->next_token();
        if (!next.has_value() || next->type != TokenType::Scalar) {
          result.assign("", JSON{nullptr});
          next = this->next_token();
          continue;
        }

        key = next->value;
        key_line = next->line;
        key_column = next->column;

        if (seen_keys.contains(key)) {
          throw YAMLDuplicateKeyError{key, next->line, next->column};
        }
        seen_keys.insert(key);

        auto colon{this->next_token()};
        if (!colon.has_value() || colon->type != TokenType::BlockMappingValue) {
          result.assign(std::string{key}, JSON{nullptr});
          if (colon.has_value()) {
            this->pending_tokens_.push_back(colon.value());
          }
          next = this->next_token();
          continue;
        }

        next = this->next_token();
        if (!next.has_value() || next->type == TokenType::StreamEnd ||
            next->type == TokenType::DocumentEnd ||
            next->type == TokenType::DocumentStart) {
          result.assign(std::string{key}, JSON{nullptr});
          if (next.has_value()) {
            this->pending_tokens_.push_back(next.value());
          }
          break;
        }
        if (next->type == TokenType::BlockMappingValue ||
            next->type == TokenType::BlockMappingKey) {
          result.assign(std::string{key}, JSON{nullptr});
        } else {
          this->lexer_->set_block_indent(
              static_cast<std::size_t>(base_column - 1));
          auto value{this->parse_value(next.value(),
                                       JSON::ParseContext::Property, 0, key,
                                       key_line, key_column)};
          result.assign(std::string{key}, std::move(value));
          next = this->next_token();
        }
        continue;
      }

      auto effective_column{next->column};

      if (next->type == TokenType::Anchor) {
        next = this->next_token();
        if (!next.has_value() || next->type != TokenType::Scalar) {
          continue;
        }
      }

      if (next->type == TokenType::Tag) {
        next = this->next_token();
        if (!next.has_value() || next->type != TokenType::Scalar) {
          continue;
        }
      }

      if (next->type == TokenType::Alias) {
        if (effective_column != base_column) {
          break;
        }
        const std::string alias_name{next->value};
        const auto iterator{this->anchors_.find(alias_name)};
        if (iterator == this->anchors_.end()) {
          throw YAMLUnknownAnchorError{alias_name, next->line, next->column};
        }
        key = this->json_to_key_string(iterator->second.value);
        key_line = next->line;
        key_column = next->column;

        if (seen_keys.contains(key)) {
          throw YAMLDuplicateKeyError{key, next->line, next->column};
        }
        seen_keys.insert(key);

        auto colon{this->next_token()};
        if (!colon.has_value() || colon->type != TokenType::BlockMappingValue) {
          result.assign(std::string{key}, JSON{nullptr});
          if (colon.has_value()) {
            this->pending_tokens_.push_back(colon.value());
          }
          next = this->next_token();
          continue;
        }

        next = this->next_token();

        if (!next.has_value() || next->type == TokenType::Scalar) {
          if (next.has_value()) {
            auto value{this->parse_value(next.value(),
                                         JSON::ParseContext::Property, 0, key,
                                         key_line, key_column)};
            result.assign(std::string{key}, std::move(value));
            next = this->next_token();
          } else {
            result.assign(std::string{key}, JSON{nullptr});
          }
        } else if (next->type == TokenType::StreamEnd ||
                   next->type == TokenType::DocumentEnd ||
                   next->type == TokenType::DocumentStart) {
          result.assign(std::string{key}, JSON{nullptr});
          break;
        } else {
          auto value{this->parse_value(next.value(),
                                       JSON::ParseContext::Property, 0, key,
                                       key_line, key_column)};
          result.assign(std::string{key}, std::move(value));
          next = this->next_token();
        }
        continue;
      }

      if (effective_column != base_column) {
        break;
      }

      key = next->value;
      key_line = next->line;
      key_column = next->column;

      if (next->multiline) {
        throw YAMLParseError{next->line, next->column,
                             "Multi-line implicit mapping key"};
      }

      if (seen_keys.contains(key)) {
        throw YAMLDuplicateKeyError{key, next->line, next->column};
      }
      seen_keys.insert(key);

      auto colon{this->next_token()};
      if (!colon.has_value() || colon->type != TokenType::BlockMappingValue) {
        if (colon.has_value()) {
          this->pending_tokens_.push_back(colon.value());
        }
        break;
      }

      next = this->next_token();

      if (!next.has_value() || next->type == TokenType::Scalar) {
        if (next.has_value()) {
          auto after{this->next_token()};
          if (after.has_value()) {
            this->pending_tokens_.push_back(after.value());
          }
          auto value{this->parse_value(next.value(),
                                       JSON::ParseContext::Property, 0, key,
                                       key_line, key_column)};
          result.assign(std::string{key}, std::move(value));
          next = this->next_token();
        } else {
          result.assign(std::string{key}, JSON{nullptr});
        }
      } else if (next->type == TokenType::StreamEnd ||
                 next->type == TokenType::DocumentEnd ||
                 next->type == TokenType::DocumentStart) {
        result.assign(std::string{key}, JSON{nullptr});
        break;
      } else {
        auto value{this->parse_value(next.value(), JSON::ParseContext::Property,
                                     0, key, key_line, key_column)};
        result.assign(std::string{key}, std::move(value));
        next = this->next_token();
      }
    }

    if (next.has_value() && next->type != TokenType::StreamEnd &&
        next->type != TokenType::DocumentEnd) {
      this->pending_tokens_.push_back(next.value());
      if (next->type == TokenType::DocumentStart) {
        this->pending_token_position_ = next->position;
      }
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Object,
                          this->lexer_->line(), this->lexer_->column(),
                          JSON::ParseContext::Root, 0, std::string_view{});

    return result;
  }

  Lexer *lexer_;
  const JSON::ParseCallback *callback_;
  std::unordered_map<std::string, AnchoredValue> anchors_;
  bool recording_anchor_{false};
  std::vector<CallbackRecord> current_anchor_callbacks_;
  std::deque<Token> pending_tokens_;
  std::optional<std::size_t> pending_token_position_;
  std::unordered_map<std::string, std::string> tag_directives_;
  std::uint64_t document_start_line_{0};
};

} // namespace sourcemeta::core::yaml

#endif
