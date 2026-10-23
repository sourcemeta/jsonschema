#ifndef SOURCEMETA_CORE_YAML_PARSER_H_
#define SOURCEMETA_CORE_YAML_PARSER_H_

#include "lexer.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/yaml_error.h>
#include <sourcemeta/core/yaml_roundtrip.h>

#include <algorithm>     // std::max, std::find_if
#include <cassert>       // assert
#include <cstdint>       // std::uint64_t, std::int64_t
#include <optional>      // std::optional
#include <sstream>       // std::ostringstream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move, std::unreachable
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
  std::size_t expanded_weight;
};

inline auto to_roundtrip_chomping(const BlockChomping chomping)
    -> YAMLRoundTrip::Chomping {
  switch (chomping) {
    case BlockChomping::Clip:
      return YAMLRoundTrip::Chomping::Clip;
    case BlockChomping::Strip:
      return YAMLRoundTrip::Chomping::Strip;
    case BlockChomping::Keep:
      return YAMLRoundTrip::Chomping::Keep;
  }

    // Clang settles that no other value arrives, so only GCC and MSVC
    // need to be told that the end of this function is never reached
#if !defined(__clang__)
  std::unreachable();
#endif
}

class Parser {
public:
  Parser(Lexer *lexer, const JSON::ParseCallback *callback,
         YAMLRoundTrip *roundtrip = nullptr)
      : lexer_{lexer}, callback_{callback}, roundtrip_{roundtrip} {}

  auto parse() -> JSON {
    bool empty_document{false};
    // One parser reads one document, so nothing has been handed back by the
    // time the first token is asked for, and the lexer opens every stream with
    // the token that says so
    assert(this->pending_tokens_.empty());
    auto token{this->lexer_->next()};
    assert(token.type == TokenType::StreamStart);
    token = this->lexer_->next();

    if (token.type == TokenType::StreamEnd) [[unlikely]] {
      throw YAMLParseError{1, 1, "Empty YAML document"};
    }

    if (token.type == TokenType::DirectiveYAML ||
        token.type == TokenType::DirectiveTag ||
        token.type == TokenType::DirectiveReserved) {
      this->process_directives(token, true);
    }

    if (token.type == TokenType::DocumentStart) {
      if (this->roundtrip_ != nullptr) {
        this->roundtrip_->leading_comments =
            this->lexer_->take_preceding_comments();
        this->roundtrip_->explicit_document_start = true;
      }
      this->document_start_line_ = token.line;
      this->lexer_->skip_line_break();
      const auto pos_before_next{this->lexer_->position()};
      token = this->lexer_->next();
      if (this->roundtrip_ != nullptr) {
        this->roundtrip_->document_start_comment =
            this->lexer_->take_inline_comment();
      }

      if (token.type == TokenType::StreamEnd ||
          token.type == TokenType::DocumentEnd ||
          token.type == TokenType::DocumentStart) {
        if (token.type == TokenType::DocumentStart) {
          this->pending_tokens_.push_back(token);
          this->pending_token_position_ = pos_before_next;
          return JSON{nullptr};
        }

        if (this->roundtrip_ != nullptr) {
          this->roundtrip_->post_start_comments =
              this->lexer_->take_preceding_comments();
        }

        // A document with no node of its own still ends the way any other
        // does, so what follows it is read the same way
        empty_document = true;
      }
    } else if (token.type == TokenType::StreamEnd) [[unlikely]] {
      throw YAMLParseError{1, 1, "Empty YAML document"};
    } else if (token.type == TokenType::DocumentEnd) {
      auto pos_before_pending{this->lexer_->position()};
      while (token.type == TokenType::DocumentEnd) {
        this->document_ended_ = true;
        pos_before_pending = this->lexer_->position();
        token = this->lexer_->next();
      }
      if (token.type == TokenType::StreamEnd) [[unlikely]] {
        throw YAMLParseError{1, 1, "Empty YAML document"};
      }
      // A token handed back has not been read as far as the caller is
      // concerned, so where the stream stands is where that token begins
      // rather than where reading it left off. Without that the caller resumes
      // part way into it and loses the text in between. Only a document marker
      // carries where it began, so anything else is placed by what the stream
      // stood at before it was read, which is how the other handback does it
      this->pending_tokens_.push_back(token);
      this->pending_token_position_ = token.type == TokenType::DocumentStart
                                          ? token.position
                                          : pos_before_pending;
      return JSON{nullptr};
    }

    JSON result{nullptr};
    auto pos_before_token{this->lexer_->position()};

    if (!empty_document) {
      if (this->roundtrip_ != nullptr) {
        auto comments{this->lexer_->take_preceding_comments()};
        this->lexer_->take_inline_comment();
        if (this->roundtrip_->explicit_document_start) {
          this->roundtrip_->post_start_comments = std::move(comments);
        } else {
          this->roundtrip_->leading_comments = std::move(comments);
        }
      }

      result =
          this->parse_value(token, JSON::ParseContext::Root, 0, EMPTY_PROPERTY);

      this->attach_leading_comments_to_first_key(result);

      pos_before_token = this->lexer_->position();
      token = this->next_token();
      if (this->roundtrip_ != nullptr) {
        auto root_inline{this->lexer_->take_inline_comment()};
        if (root_inline.has_value()) {
          this->roundtrip_->styles[this->pointer_stack_].comment_inline =
              std::move(root_inline);
        }
      }
    }
    while (token.type == TokenType::DocumentEnd) {
      this->document_ended_ = true;
      if (this->roundtrip_ != nullptr) {
        this->roundtrip_->pre_end_comments =
            this->lexer_->take_preceding_comments();
        this->roundtrip_->explicit_document_end = true;
      }
      this->lexer_->skip_line_break();
      pos_before_token = this->lexer_->position();
      token = this->next_token();
      if (this->roundtrip_ != nullptr) {
        this->roundtrip_->document_end_comment =
            this->lexer_->take_inline_comment();
      }
    }

    if (this->roundtrip_ != nullptr) {
      auto trailing{this->lexer_->take_preceding_comments()};
      const bool ends_the_stream{token.type == TokenType::StreamEnd};
      if (!trailing.empty() && ends_the_stream) {
        this->roundtrip_->trailing_comments = std::move(trailing);
      }
    }

    if (token.type != TokenType::StreamEnd) {
      this->pending_tokens_.push_back(token);
      if (token.type == TokenType::DocumentStart) {
        this->pending_token_position_ = token.position;
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

  // Metadata collected for a round-trip describes the one document it was read
  // from, so a stream that carries more than that cannot be written back
  auto validate_single_document() -> void {
    auto token{this->next_token()};
    while (token.type == TokenType::DocumentEnd) {
      token = this->next_token();
    }

    if (token.type != TokenType::StreamEnd) [[unlikely]] {
      throw YAMLParseError{token.line, token.column,
                           "Unexpected content after document"};
    }
  }

  // YAML 1.2.2 Section 6.8.2 makes tag directives local to one document, so
  // every end marker crossed here leaves the next document to declare its own
  auto skip_document_ends(Token &token, bool &saw_document_end) -> void {
    while (token.type == TokenType::DocumentEnd) {
      saw_document_end = true;
      this->tag_directives_.clear();
      token = this->next_token();
    }
  }

  auto validate_end_of_stream() -> void {
    // Whatever stands after the document that was read is checked rather than
    // handed over, so nothing of it is announced to whoever is listening for
    // the one document the caller gets
    this->callback_ = nullptr;
    auto token{this->next_token()};
    // The preceding parse already consumed a document, so its end marker, if
    // any, is not among the tokens seen here. YAML 1.2.2 Section 6.8.2: tag
    // directives are local to one document, so crossing that boundary begins a
    // fresh directive scope.
    // The document read before this point is complete, so what applied to it
    // goes out of scope here. Section 6.8.2 makes directives local to one
    // document, and Section 9.1 has each document be "completely independent
    // from the rest", which puts the anchors it declared out of reach of
    // whatever follows. A fresh scope opens whether an end marker closed the
    // last document or a directives end marker opens the next
    this->tag_directives_.clear();
    this->anchors_.clear();
    bool saw_document_end{this->document_ended_};
    this->skip_document_ends(token, saw_document_end);
    if (token.type == TokenType::StreamEnd) {
      return;
    }
    while (token.type != TokenType::StreamEnd) {
      // Section 9.2: a document that is not terminated by a document end
      // marker is followed by one that begins with a directives end marker, so
      // crossing that marker opens a document just as the end marker closes
      // one. Production 211 admits an explicit document directly after any
      // other, with no suffix between them
      bool opened_document{false};
      if (token.type == TokenType::DocumentStart) {
        opened_document = true;
        token = this->next_token();
        if (token.type == TokenType::StreamEnd) {
          return;
        }
        // Section 8.2.3 admits an empty node, so an explicit document may
        // carry nothing of its own and be closed by its end marker directly
        if (token.type == TokenType::DocumentEnd) {
          this->skip_document_ends(token, saw_document_end);
          continue;
        }

        if (token.type == TokenType::DocumentStart) {
          continue;
        }
      }
      if (token.type == TokenType::DirectiveYAML ||
          token.type == TokenType::DirectiveTag ||
          token.type == TokenType::DirectiveReserved) {
        if (!saw_document_end) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Directive not allowed without preceding "
                               "document end marker"};
        }
        this->process_directives(token);
        continue;
      }
      // A directives end marker is read above, which is what opens a document
      assert(opened_document || token.type != TokenType::DocumentStart);
      if (!saw_document_end && !opened_document) [[unlikely]] {
        throw YAMLParseError{token.line, token.column,
                             "Unexpected content after document"};
      }
      this->parse_value(token, JSON::ParseContext::Root, 0, EMPTY_PROPERTY);
      saw_document_end = false;
      // That document is complete, so anything it declared stops applying
      // before the next one reads its own
      this->tag_directives_.clear();
      this->anchors_.clear();
      token = this->next_token();
      this->skip_document_ends(token, saw_document_end);
    }
  }

private:
  // Cap how much alias expansion may materialise, so that a document
  // cannot expand into a far larger one on attacker-controlled input. The
  // allowance grows with the text that stands ahead of the alias drawing on
  // it, as an expansion that outgrows the text calling for it by orders of
  // magnitude is the shape of the attack, whereas a long document that reuses
  // anchors in earnest grows in step with its own size. Measuring the text
  // ahead rather than the whole of what was handed over keeps everything the
  // parser has not reached, from a comment sitting behind the alias to the
  // documents that follow this one in a stream, from paying for an expansion it
  // takes no part in. The floor keeps short documents workable and the ceiling
  // keeps long ones from claiming an unbounded allowance
  static constexpr std::size_t MAXIMUM_EXPANDED_WEIGHT{20000000};
  static constexpr std::size_t MINIMUM_EXPANDED_WEIGHT{20000};
  static constexpr std::size_t EXPANDED_WEIGHT_PER_INPUT_BYTE{200};

  [[nodiscard]] static auto expansion_budget(const std::size_t input_read)
      -> std::size_t {
    if (input_read > MAXIMUM_EXPANDED_WEIGHT / EXPANDED_WEIGHT_PER_INPUT_BYTE)
        [[unlikely]] {
      return MAXIMUM_EXPANDED_WEIGHT;
    }

    return std::max(MINIMUM_EXPANDED_WEIGHT,
                    input_read * EXPANDED_WEIGHT_PER_INPUT_BYTE);
  }

  // Cap the recursion depth of the value parser so that a deeply nested
  // document cannot overflow the stack on attacker-controlled input. The bound
  // is conservative so that it holds on platforms with a small default stack
  // such as Windows, and is still far above any realistic nesting
  static constexpr std::size_t MAXIMUM_DEPTH{100};
  std::size_t depth_{0};

  struct DepthScope {
    std::size_t &counter;
    explicit DepthScope(std::size_t &value) : counter{value} {
      this->counter += 1;
    }
    ~DepthScope() { this->counter -= 1; }
    DepthScope(const DepthScope &) = delete;
    auto operator=(const DepthScope &) -> DepthScope & = delete;
    DepthScope(DepthScope &&) = delete;
    auto operator=(DepthScope &&) -> DepthScope & = delete;
  };

  // What an expansion costs is what materialising it takes, not how many
  // places it fills. Charging a place alone lets one holding a long run of
  // text be copied for the price of an empty one, which turns the allowance
  // into a multiplier on that length. So a place costs one for itself and one
  // more for each byte of text it carries, keeping what an expansion is
  // charged in step with what it occupies
  auto count_expanded_weight(const JSON &value) -> std::size_t {
    std::size_t total{1};
    if (value.is_array()) {
      for (const auto &element : value.as_array()) {
        total += this->count_expanded_weight(element);
      }
    } else if (value.is_object()) {
      for (const auto &entry : value.as_object()) {
        total += entry.first.size() + this->count_expanded_weight(entry.second);
      }
    } else if (value.is_string()) {
      // What a copy occupies is counted in bytes, where the logical length
      // counts the characters those bytes spell, so a character written in
      // four of them would be charged as one
      total += value.to_string().size();
    } else if (value.is_decimal()) {
      // A coefficient runs as wide as the digits it was written with, so one
      // of them is no more a single unit than a run of text is
      total += value.to_decimal().to_string().size();
    }

    return total;
  }

  // A comment block that runs straight into the first key of the document reads
  // as belonging to that key, so it is recorded there and travels with the key
  // if the document is later rearranged. A blank line in between instead marks
  // the block as a header for the document as a whole
  auto attach_leading_comments_to_first_key(const JSON &result) -> void {
    if ((this->roundtrip_ == nullptr) || !result.is_object() ||
        result.empty()) {
      return;
    }

    auto &comments{this->roundtrip_->explicit_document_start
                       ? this->roundtrip_->post_start_comments
                       : this->roundtrip_->leading_comments};
    const auto blank{
        std::find_if(comments.crbegin(), comments.crend(),
                     [](const auto &comment) { return comment.empty(); })};
    if (blank == comments.crbegin()) {
      return;
    }

    const auto first{blank.base()};
    Pointer pointer{result.as_object().cbegin()->first};
    auto &attached{this->roundtrip_->styles[pointer].comments_before};
    attached.insert(attached.cbegin(), first, comments.cend());
    comments.erase(first, comments.cend());
  }

  // A flow collection may be written with a space just inside its delimiters,
  // which the first token after the opening one gives away
  auto record_flow_padding(const Token &start_token, const Token &first)
      -> void {
    if ((this->roundtrip_ == nullptr) || first.line != start_token.line ||
        first.column <= start_token.column + 1) {
      return;
    }

    this->roundtrip_->styles[this->pointer_stack_].padded_flow = true;
  }

  auto process_directives(Token &token, const bool record = false) -> void {
    bool seen_yaml_directive{false};
    while (token.type == TokenType::DirectiveYAML ||
           token.type == TokenType::DirectiveTag ||
           token.type == TokenType::DirectiveReserved) {
      if (record && this->roundtrip_ != nullptr) {
        auto &prefix{this->roundtrip_->document_prefix};
        for (auto &comment : this->lexer_->take_preceding_comments()) {
          prefix.push_back(std::move(comment));
        }

        prefix.emplace_back(token.value);
      }
      if (token.type == TokenType::DirectiveYAML) {
        if (seen_yaml_directive) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Duplicate %YAML directive"};
        }
        seen_yaml_directive = true;
        const auto content{token.value};
        auto cursor{5UZ};
        while (cursor < content.size() &&
               (content[cursor] == ' ' || content[cursor] == '\t')) {
          cursor++;
        }
        const auto version_start{cursor};
        while (cursor < content.size() && content[cursor] != ' ' &&
               content[cursor] != '\t') {
          cursor++;
        }
        const auto version{
            content.substr(version_start, cursor - version_start)};
        // YAML 1.2.2 Section 6.8.1: the version is a major and a minor decimal
        // number joined by a dot, so any other character (such as a comment
        // sign without a preceding separation space) is invalid
        const auto version_dot{version.find('.')};
        if (version_dot == std::string_view::npos || version_dot == 0 ||
            version_dot + 1 == version.size() ||
            version.find_first_not_of("0123456789") != version_dot ||
            version.find_first_not_of("0123456789", version_dot + 1) !=
                std::string_view::npos) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Invalid version in %YAML directive"};
        }
        // YAML 1.2.2 Section 6.8.1: a document that names a higher major
        // version than this processor supports must be rejected
        const auto major{version.substr(0, version_dot)};
        if (major.size() > 1 || major.front() > '1') [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Unsupported major version in %YAML directive"};
        }
        while (cursor < content.size() &&
               (content[cursor] == ' ' || content[cursor] == '\t')) {
          cursor++;
        }
        if (cursor < content.size() && content[cursor] != '#') [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Invalid content in %YAML directive"};
        }
      } else if (token.type == TokenType::DirectiveTag) {
        const auto content{token.value};
        auto cursor{4UZ};
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
               content[cursor] != '\t') {
          cursor++;
        }
        const auto prefix{
            std::string{content.substr(prefix_start, cursor - prefix_start)}};
        // YAML 1.2.2 Section 6.8.2 spells the directive with a handle and a
        // prefix, so one that names neither is not a directive at all
        if (handle.empty() || prefix.empty()) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Incomplete %TAG directive"};
        }

        {
          // YAML 1.2.2 Section 6.8.2: a handle may carry at most one tag
          // directive in a document, even when both give the same prefix
          if (this->tag_directives_.contains(handle)) [[unlikely]] {
            throw YAMLParseError{
                token.line, token.column,
                "Duplicate %TAG directive for the same handle"};
          }
          this->tag_directives_.insert_or_assign(handle, prefix);
        }
      }
      token = this->lexer_->next();
    }
  }

  auto resolve_tag(const std::string_view raw_tag) -> std::string {
    // A tag token begins at the exclamation mark the lexer scanned it from,
    // and YAML 1.2.2 Section 6.9.1 has that lexer turn down a verbatim tag
    // left unclosed, so one that opens with a bracket closes with one
    assert(raw_tag.starts_with('!'));
    assert(!raw_tag.starts_with("!<") || raw_tag.back() == '>');
    if (raw_tag.size() > 2 && raw_tag[1] == '<') {
      return std::string{raw_tag.substr(2, raw_tag.size() - 3)};
    }

    if (raw_tag.starts_with("!!")) {
      // The shorthand form takes one or more tag characters after its handle,
      // and Example 6.27 of the specification turns one down for want of them
      if (raw_tag.size() == 2) [[unlikely]] {
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Tag shorthand with no suffix"};
      }

      const auto iterator{this->tag_directives_.find("!!")};
      if (iterator != this->tag_directives_.end()) {
        return iterator->second + std::string{raw_tag.substr(2)};
      }
      return "tag:yaml.org,2002:" + std::string{raw_tag.substr(2)};
    }

    if (raw_tag.size() > 1) {
      const auto second_bang{raw_tag.find('!', 1)};
      if (second_bang == raw_tag.size() - 1) [[unlikely]] {
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Tag shorthand with no suffix"};
      }

      if (second_bang != std::string_view::npos) {
        const auto handle{std::string{raw_tag.substr(0, second_bang + 1)}};
        const auto iterator{this->tag_directives_.find(handle)};
        if (iterator != this->tag_directives_.end()) {
          return iterator->second +
                 std::string{raw_tag.substr(second_bang + 1)};
        }
        // YAML 1.2.2 Section 6.8.2.1: a named tag handle must be associated
        // with a prefix by a tag directive, so an undefined handle is an error
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Undefined tag handle"};
      }
    }

    return std::string{raw_tag};
  }

  auto invoke_callback(const JSON::ParsePhase phase, const JSON::Type type,
                       const std::uint64_t line, const std::uint64_t column,
                       const JSON::ParseContext context,
                       const std::size_t index, const std::string &property)
      -> void {
    if ((this->callback_ == nullptr) || !*this->callback_) {
      return;
    }

    (*this->callback_)(phase, type, line, column, context, index, property);

    if (this->recording_anchor_) {
      this->current_anchor_callbacks_.push_back(
          {.phase = phase,
           .type = type,
           .line = line,
           .column = column,
           .context = context,
           .index = index,
           .property = std::string{property}});
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

  // Columns are counted from one, so the indentation a token establishes is
  // one less. Only the token that ends the stream carries no column, and no
  // node property or block collection opens on one
  [[nodiscard]] static auto indentation_of(const Token &token) -> std::size_t {
    assert(token.column > 0);
    return static_cast<std::size_t>(token.column - 1);
  }

  [[nodiscard]] auto json_to_key_string(const JSON &value,
                                        const std::uint64_t key_line,
                                        const std::uint64_t key_column) const
      -> std::string {
    // RFC 8259 Section 4: an object member name is a string, so a mapping key
    // that resolves to a collection cannot be represented as JSON. PyYAML
    // raises on the unhashable key and js-yaml rejects the complex key, so a
    // collection key is rejected rather than silently stringified
    if (value.is_array() || value.is_object()) [[unlikely]] {
      throw YAMLParseError{key_line, key_column,
                           "Mapping key cannot be a collection"};
    }
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

  // The node a scalar key denotes, which is what an anchor on that key has to
  // name so that aliasing it yields the very same key
  auto resolve_scalar_node(const Token &token,
                           const std::optional<std::string> &tag = std::nullopt)
      -> JSON {
    // Round-trip mode preserves the original key text, so it is not resolved
    if (this->roundtrip_ != nullptr) {
      return JSON{std::string{token.value}};
    }

    // Resolve a scalar key to its typed value, so that keys such as 0x1 and 1
    // collapse to the same member name, keeping key handling consistent with
    // values and alias keys. An explicit tag is honored the same way it would
    // be for a scalar value, so a string-tagged key keeps its literal text
    return this->interpret_scalar(token.value, token.scalar_style, tag);
  }

  auto resolve_scalar_key(const Token &token,
                          const std::optional<std::string> &tag = std::nullopt)
      -> std::string {
    return this->json_to_key_string(this->resolve_scalar_node(token, tag),
                                    token.line, token.column);
  }

  auto parse_value(const Token &token, const JSON::ParseContext context,
                   const std::size_t index, const std::string &property,
                   const std::uint64_t key_line = 0,
                   const std::uint64_t key_column = 0) -> JSON {
    const DepthScope scope{this->depth_};
    if (this->depth_ > MAXIMUM_DEPTH) {
      throw YAMLParseError{token.line, token.column,
                           "Maximum nesting depth exceeded"};
    }

    if (this->roundtrip_ != nullptr) {
      if (context == JSON::ParseContext::Property) {
        this->pointer_stack_.push_back(std::string{property});
      } else if (context == JSON::ParseContext::Index) {
        this->pointer_stack_.push_back(index);
      }
    }

    std::optional<std::string_view> anchor_name;
    std::uint64_t anchor_line{0};
    std::optional<std::string> tag;
    std::optional<std::string> raw_tag;
    bool tag_before_anchor{false};
    std::size_t anchor_count{0};
    std::optional<std::string> anchor_inline_comment;
    Token current_token{token};
    std::uint64_t node_start_column{token.column};
    std::uint64_t prefix_line{token.line};

    while (current_token.type == TokenType::Anchor ||
           current_token.type == TokenType::Tag) {
      if (this->lexer_->flow_level() == 0 &&
          context == JSON::ParseContext::Property && key_line > 0 &&
          current_token.line != key_line) {
        const auto value_indent{indentation_of(current_token)};
        const auto parent_indent{this->lexer_->block_indent()};
        if (parent_indent != SIZE_MAX && value_indent <= parent_indent)
            [[unlikely]] {
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
        if (this->roundtrip_ != nullptr) {
          raw_tag = std::string{current_token.value};
          tag_before_anchor = anchor_count == 0;
        }
      }

      auto next{this->lexer_->next()};
      if ((this->roundtrip_ != nullptr) && anchor_name.has_value()) {
        anchor_inline_comment = this->lexer_->take_inline_comment();
      } else if ((this->roundtrip_ != nullptr) &&
                 context == JSON::ParseContext::Root &&
                 this->document_start_line_ > 0 &&
                 current_token.line == this->document_start_line_ &&
                 !this->roundtrip_->document_start_comment.has_value()) {
        // A comment that trails the node properties of the root node still
        // sits on the document start marker line, which is where it is written
        this->roundtrip_->document_start_comment =
            this->lexer_->take_inline_comment();
      }
      if (next.type == TokenType::StreamEnd ||
          next.type == TokenType::DocumentEnd ||
          next.type == TokenType::DocumentStart) {
        JSON empty_value{nullptr};
        if (tag.has_value()) {
          if (tag.value() == "tag:yaml.org,2002:str") {
            empty_value = JSON{std::string{}};
          }
        }
        this->pending_tokens_.push_back(next);
        // An empty node that carries an anchor is still a node, so it is
        // announced and filed the same way as one ending any other place
        // does. Doing only the part that records how it was written left it
        // unannounced and left the anchor unresolvable further on
        if (anchor_name.has_value()) {
          this->register_anchored_empty(anchor_name.value(), empty_value, token,
                                        context, index, property,
                                        anchor_inline_comment);
        }

        this->record_tag(raw_tag, tag_before_anchor, empty_value);
        if ((this->roundtrip_ != nullptr) &&
            context != JSON::ParseContext::Root) {
          this->pointer_stack_.pop_back();
        }
        return empty_value;
      }
      current_token = next;

      if (current_token.type == TokenType::Scalar &&
          current_token.column <= key_column && key_column > 0) {
        auto after{this->lexer_->next()};
        if (after.type == TokenType::BlockMappingValue) {
          this->pending_tokens_.push_back(current_token);
          this->pending_tokens_.push_back(after);
          JSON empty_value{nullptr};
          if (tag.has_value() && tag.value() == "tag:yaml.org,2002:str") {
            empty_value = JSON{std::string{}};
          }
          if (anchor_name.has_value()) {
            this->register_anchored_empty(anchor_name.value(), empty_value,
                                          token, context, index, property,
                                          anchor_inline_comment);
          }
          this->record_tag(raw_tag, tag_before_anchor, empty_value);
          if ((this->roundtrip_ != nullptr) &&
              context != JSON::ParseContext::Root) {
            this->pointer_stack_.pop_back();
          }
          return empty_value;
        }
        this->pending_tokens_.push_back(after);
      }

      if ((anchor_name.has_value() || tag.has_value()) &&
          context == JSON::ParseContext::Index &&
          current_token.type == TokenType::BlockSequenceEntry) {
        const auto block_indent{this->lexer_->block_indent()};
        const auto entry_indent{indentation_of(current_token)};
        if (block_indent != SIZE_MAX && entry_indent <= block_indent) {
          this->pending_tokens_.push_back(current_token);
          JSON empty_value{nullptr};
          if (tag.has_value() && tag.value() == "tag:yaml.org,2002:str") {
            empty_value = JSON{std::string{}};
          }
          if (anchor_name.has_value()) {
            this->register_anchored_empty(anchor_name.value(), empty_value,
                                          token, context, index, property,
                                          anchor_inline_comment);
          }
          this->record_tag(raw_tag, tag_before_anchor, empty_value);
          if ((this->roundtrip_ != nullptr) &&
              context != JSON::ParseContext::Root) {
            this->pointer_stack_.pop_back();
          }
          return empty_value;
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
      this->record_tag(raw_tag, tag_before_anchor, empty_value);
      if ((this->roundtrip_ != nullptr) &&
          context != JSON::ParseContext::Root) {
        this->pointer_stack_.pop_back();
      }
      return empty_value;
    }

    if (current_token.line != prefix_line) {
      node_start_column = 0;
    }

    if ((anchor_name.has_value() || tag.has_value()) &&
        this->lexer_->flow_level() == 0 && current_token.line == prefix_line &&
        current_token.type == TokenType::BlockSequenceEntry) [[unlikely]] {
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
        // YAML 1.2.2 Section 7.4.2: the value of an entry of a flow collection
        // is a single node, and a pair carrying no brackets of its own is a
        // node only where a flow sequence takes its entries. Leaving the
        // indicator unread in that position hands the scalar back on its own,
        // which is what lets the caller report the separator the entry is
        // really missing
        const auto pair_without_brackets_allowed{
            this->lexer_->flow_level() == 0 ||
            context != JSON::ParseContext::Property};
        // YAML 1.2.2 Section 6.7: "Implicit keys are restricted to a single
        // line", so an indicator standing on a later line belongs to a mapping
        // further out rather than making this scalar a key of its own. Flow
        // context is told the same by the error below, which reports where the
        // indicator was read instead of handing it back
        const bool indicator_shares_the_key_line{(
            this->lexer_->flow_level() > 0 || next.line == current_token.line)};
        if (next.type == TokenType::BlockMappingValue &&
            pair_without_brackets_allowed && indicator_shares_the_key_line) {
          if (current_token.multiline) [[unlikely]] {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Multi-line implicit mapping key"};
          }
          if (this->lexer_->flow_level() > 0 && next.line != current_token.line)
              [[unlikely]] {
            throw YAMLParseError{next.line, next.column,
                                 "Implicit key and value indicator on "
                                 "different lines in flow context"};
          }
          if (this->lexer_->flow_level() == 0 &&
              context == JSON::ParseContext::Property && key_line > 0 &&
              current_token.line == key_line) [[unlikely]] {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Implicit mapping key in block value on "
                                 "same line as parent key"};
          }
          if (this->lexer_->flow_level() == 0 &&
              (anchor_name.has_value() || tag.has_value()) &&
              this->document_start_line_ > 0 &&
              current_token.line == this->document_start_line_) [[unlikely]] {
            throw YAMLParseError{
                current_token.line, current_token.column,
                "Node properties before implicit mapping key on "
                "document start line"};
          }
          if (anchor_name.has_value() && anchor_line == current_token.line) {
            // The anchor names the resolved key node, so that aliasing it
            // yields the very same member name rather than the raw text
            JSON key_value{this->resolve_scalar_node(current_token, tag)};
            this->recording_anchor_ = false;
            this->anchors_.insert_or_assign(
                std::string{anchor_name.value()},
                AnchoredValue{
                    .value = key_value,
                    .callbacks = std::move(this->current_anchor_callbacks_),
                    .expanded_weight = this->count_expanded_weight(key_value)});
            this->current_anchor_callbacks_.clear();
            anchor_name.reset();
          }
          result = this->parse_block_mapping_from_first_key(
              current_token, context, index, property, key_line, key_column,
              node_start_column, tag);
        } else {
          if (anchor_count > 1) [[unlikely]] {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Multiple anchors on a scalar node"};
          }
          result = this->parse_scalar(current_token, tag, context, index,
                                      property, key_line, key_column);
          this->pending_tokens_.push_back(next);
        }
        break;
      }
      case TokenType::MappingStart:
        result = this->parse_flow_mapping(current_token, context, index,
                                          property, key_line, key_column);
        this->record_collection_style(YAMLRoundTrip::CollectionStyle::Flow);
        break;
      case TokenType::SequenceStart:
        result = this->parse_flow_sequence(current_token, context, index,
                                           property, key_line, key_column);
        this->record_collection_style(YAMLRoundTrip::CollectionStyle::Flow);
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
        // Reading the token that follows the alias skips over any comment
        // between them, so how far the document had been read is settled here
        const auto input_read{this->position()};
        auto next{this->next_token()};
        if (next.type == TokenType::BlockMappingValue) {
          const std::string alias_name{current_token.value};
          const auto iterator{this->anchors_.find(alias_name)};
          if (iterator == this->anchors_.end()) [[unlikely]] {
            throw YAMLUnknownAnchorError{alias_name, current_token.line,
                                         current_token.column};
          }
          const auto key_string{this->json_to_key_string(iterator->second.value,
                                                         current_token.line,
                                                         current_token.column)};
          Token key_token{current_token};
          key_token.type = TokenType::Scalar;
          key_token.value = key_string;
          result = this->parse_block_mapping_from_first_key(
              key_token, context, index, property, key_line, key_column,
              node_start_column, std::nullopt, true);
        } else {
          if (anchor_name.has_value()) [[unlikely]] {
            throw YAMLParseError{current_token.line, current_token.column,
                                 "Cannot anchor an alias node"};
          }
          result = this->resolve_alias(current_token, context, index, property,
                                       input_read, key_line, key_column);
          if (this->roundtrip_ != nullptr) {
            this->roundtrip_->aliases[this->pointer_stack_] =
                std::string{current_token.value};
          }
          this->pending_tokens_.push_back(next);
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
                        .callbacks = std::move(this->current_anchor_callbacks_),
                        .expanded_weight =
                            this->count_expanded_weight(result)});
      this->current_anchor_callbacks_.clear();

      if (this->roundtrip_ != nullptr) {
        auto &style{this->roundtrip_->styles[this->pointer_stack_]};
        style.anchor = std::string{anchor_name.value()};
        if (anchor_inline_comment.has_value()) {
          style.comment_inline = std::move(anchor_inline_comment);
        }
      }
    }

    this->record_tag(raw_tag, tag_before_anchor, result);

    if ((this->roundtrip_ != nullptr) && result.is_array()) {
      this->roundtrip_->styles[this->pointer_stack_].sequence_size =
          result.size();
    }

    if ((this->roundtrip_ != nullptr) && context != JSON::ParseContext::Root) {
      this->pointer_stack_.pop_back();
    }

    return result;
  }

  auto parse_scalar(const Token &token, const std::optional<std::string> &tag,
                    const JSON::ParseContext context, const std::size_t index,
                    const std::string &property,
                    const std::uint64_t key_line = 0,
                    const std::uint64_t key_column = 0) -> JSON {
    JSON result{this->interpret_scalar(token.value, token.scalar_style, tag)};
    this->record_scalar_style(token, result);

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
                          EMPTY_PROPERTY);

    return result;
  }

  [[nodiscard]] static auto is_special_float(const std::string_view value)
      -> bool {
    return value == ".inf" || value == ".Inf" || value == ".INF" ||
           value == "+.inf" || value == "+.Inf" || value == "+.INF" ||
           value == "-.inf" || value == "-.Inf" || value == "-.INF" ||
           value == ".nan" || value == ".NaN" || value == ".NAN";
  }

  auto interpret_scalar(const std::string_view value, const ScalarStyle style,
                        const std::optional<std::string> &tag) -> JSON {
    if (tag.has_value()) {
      const auto &tag_value{tag.value()};
      if (tag_value == "!" || tag_value == "tag:yaml.org,2002:str") {
        return JSON{value};
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
        // RFC 8259 Section 6: Infinity and NaN are not permitted in JSON, so an
        // explicitly floated infinity or not-a-number has no JSON
        // representation. Round-trip mode preserves the original text instead
        if (is_special_float(value)) [[unlikely]] {
          if (this->roundtrip_ == nullptr) {
            throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                                 "Infinity and NaN are not permitted"};
          }
          return JSON{value};
        }
        return this->parse_float(value);
      }
      return JSON{value};
    }

    if (style != ScalarStyle::Plain) {
      return JSON{value};
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

    if (is_special_float(value)) {
      // RFC 8259 Section 6: numeric values that cannot be represented in the
      // JSON grammar, such as Infinity and NaN, are not permitted, so a YAML
      // infinity or not-a-number float has no JSON representation. Round-trip
      // mode preserves the original text instead, so it keeps the string
      if (this->roundtrip_ == nullptr) [[unlikely]] {
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Infinity and NaN are not permitted"};
      }
      return JSON{value};
    }

    if (this->looks_like_number(value)) {
      return this->parse_number(value);
    }

    return JSON{value};
  }

  [[nodiscard]] auto looks_like_number(const std::string_view value) const
      -> bool {
    // An empty scalar is read as a null before anything asks what it looks like
    assert(!value.empty());
    std::size_t start{0};
    if (value[0] == '-' || value[0] == '+') {
      start = 1;
      if (start >= value.size()) {
        return false;
      }
    }

    // YAML 1.2.2 Section 10.3.2: the hexadecimal and octal integer forms carry
    // no sign and use a lowercase indicator, so a preceding sign or an
    // uppercase indicator makes the value a plain string rather than an integer
    if (start == 0 && value.size() > 1 && value[0] == '0') {
      if (value[1] == 'x' || value[1] == 'o') {
        return true;
      }
    }

    bool has_digit{false};
    bool has_dot{false};
    bool has_exp{false};
    bool has_exp_digit{false};

    for (std::size_t index = start; index < value.size(); ++index) {
      const char current{value[index]};
      if (current >= '0' && current <= '9') {
        has_digit = true;
        if (has_exp) {
          has_exp_digit = true;
        }
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

    // YAML 1.2.2 core schema: an exponent must carry at least one digit, so
    // "1e" is a plain string rather than a malformed number that would throw
    return has_digit && (!has_exp || has_exp_digit);
  }

  auto parse_number(const std::string_view value) -> JSON {
    const std::size_t prefix{(value[0] == '-' || value[0] == '+') ? 1U : 0U};
    // YAML 1.2.2 Section 10.3.2: the hexadecimal and octal integer forms carry
    // no sign and use a lowercase indicator
    if (prefix == 0 && value.size() > 1 && value[0] == '0') {
      const char indicator{value[1]};
      if (indicator == 'x') {
        return this->parse_base_integer(value, 16);
      }
      if (indicator == 'o') {
        return this->parse_base_integer(value, 8);
      }
    }

    // The YAML core schema permits a leading plus sign on numbers, but the
    // strict numeric parsers (like RFC 8259 JSON) do not, so it must be
    // stripped at this layer
    const auto number{value.front() == '+' ? value.substr(1) : value};

    bool has_dot{false};
    bool has_exp{false};
    for (const char character : number) {
      if (character == '.') {
        has_dot = true;
      }
      if (character == 'e' || character == 'E') {
        has_exp = true;
      }
    }

    if (has_exp) {
      return JSON{Decimal{number}};
    }

    if (has_dot) {
      return this->parse_float(number);
    }

    return this->parse_integer(number);
  }

  auto parse_integer(const std::string_view value) -> JSON {
    const auto result{to_int64_t(std::string{value})};
    return result.has_value() ? JSON{result.value()} : JSON{Decimal{value}};
  }

  auto parse_base_integer(const std::string_view value, const int base)
      -> JSON {
    // YAML 1.2.2 Section 10.3.2: the base indicator is two characters and no
    // sign precedes it, so parsing starts at the third character
    const auto result{to_int64_t(std::string{value.substr(2)}, base)};
    if (result.has_value()) {
      return JSON{result.value()};
    }
    return JSON{value};
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

    constexpr std::size_t DOUBLE_PRECISION_LIMIT{15};
    if (significant_digits > DOUBLE_PRECISION_LIMIT) {
      return JSON{Decimal{value}};
    }

    const auto result{to_double_exact(value)};
    if (!result.has_value()) {
      return JSON{Decimal{value}};
    }

    // YAML 1.2.2 Section 10.3.2 tags a dotted or explicitly floated value as a
    // float, so an integral-valued float stays a real rather than collapsing to
    // an integer, matching the JSON parser where a dotted literal that the
    // format holds exactly is a real
    return JSON{result.value()};
  }

  auto parse_flow_mapping(const Token &start_token,
                          const JSON::ParseContext context,
                          const std::size_t index, const std::string &property,
                          const std::uint64_t key_line = 0,
                          const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Object,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_object()};
    std::unordered_set<std::string> seen_keys;
    bool found_compact_separator{false};

    auto token{this->next_token()};
    this->record_flow_padding(start_token, token);

    while (token.type != TokenType::MappingEnd) {
      if (token.type == TokenType::FlowEntry) {
        if (token.compact_separator) {
          found_compact_separator = true;
        }
        token = this->next_token();
        continue;
      }

      auto key_token{token};

      std::optional<std::string> key_tag;
      while (key_token.type == TokenType::Anchor ||
             key_token.type == TokenType::Tag) {
        if (key_token.type == TokenType::Tag) {
          key_tag = this->resolve_tag(key_token.value);
        }
        token = this->next_token();
        if (token.type == TokenType::StreamEnd) [[unlikely]] {
          throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                               "Unexpected end of input in flow mapping"};
        }
        key_token = token;
      }

      std::string key;
      if (key_token.type == TokenType::BlockMappingValue) {
        if (key_tag.has_value() && key_tag.value() == "tag:yaml.org,2002:str") {
          key = "";
        }
      } else if (key_token.type == TokenType::Scalar) {
        key = this->resolve_scalar_key(key_token, key_tag);
        this->record_key_scalar_style(key, key_token.scalar_style,
                                      key_token.quoted_original);
      } else [[unlikely]] {
        throw YAMLParseError{key_token.line, key_token.column,
                             "Expected scalar key in mapping"};
      }

      if (seen_keys.contains(key)) [[unlikely]] {
        throw YAMLDuplicateKeyError{key, key_token.line, key_token.column};
      }
      seen_keys.insert(key);

      if (key_token.type != TokenType::BlockMappingValue) {
        token = this->next_token();

        if (token.type == TokenType::StreamEnd) [[unlikely]] {
          throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                               "Unexpected end of input in flow mapping"};
        }

        if (token.type == TokenType::FlowEntry ||
            token.type == TokenType::MappingEnd) {
          if (token.type == TokenType::FlowEntry && token.compact_separator) {
            found_compact_separator = true;
          }
          result.assign(key, JSON{nullptr});
          continue;
        }

        if (token.type != TokenType::BlockMappingValue) [[unlikely]] {
          const auto colon_column{key_token.column +
                                  static_cast<std::uint64_t>(key.size())};
          throw YAMLParseError{key_token.line, colon_column,
                               "Expected ':' after mapping key"};
        }
      }

      token = this->next_token();
      if (token.type == TokenType::StreamEnd) [[unlikely]] {
        throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                             "Expected value after ':'"};
      }

      if (token.type == TokenType::FlowEntry ||
          token.type == TokenType::MappingEnd) {
        if (token.type == TokenType::FlowEntry && token.compact_separator) {
          found_compact_separator = true;
        }
        result.assign(key, JSON{nullptr});
      } else {
        auto value{this->parse_value(token, JSON::ParseContext::Property, 0,
                                     key, key_token.line, key_token.column)};
        result.assign(key, std::move(value));
      }

      if (token.type != TokenType::FlowEntry &&
          token.type != TokenType::MappingEnd) {
        token = this->next_token();
        if (token.type == TokenType::FlowEntry && token.compact_separator) {
          found_compact_separator = true;
        }
        if (token.type != TokenType::FlowEntry &&
            token.type != TokenType::MappingEnd) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Missing comma between flow mapping entries"};
        }
      }
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Object,
                          token.line, token.column, JSON::ParseContext::Root, 0,
                          EMPTY_PROPERTY);

    if ((this->roundtrip_ != nullptr) && found_compact_separator) {
      this->roundtrip_->styles[this->pointer_stack_].compact_flow = true;
    }

    return result;
  }

  auto parse_flow_sequence(const Token &start_token,
                           const JSON::ParseContext context,
                           const std::size_t index, const std::string &property,
                           const std::uint64_t key_line = 0,
                           const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Array,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_array()};
    const auto parent_block_indent{this->lexer_->block_indent()};
    bool found_compact_separator{false};

    auto token{this->next_token()};
    this->record_flow_padding(start_token, token);
    std::size_t element_index{0};

    while (token.type != TokenType::SequenceEnd) {
      if (parent_block_indent != SIZE_MAX && token.line != start_token.line) {
        const auto token_indent{token.column > 0
                                    ? static_cast<std::size_t>(token.column - 1)
                                    : 0UZ};
        if (token_indent <= parent_block_indent) [[unlikely]] {
          throw YAMLParseError{
              token.line, token.column,
              "Flow content indented less than or equal to parent block level"};
        }
      }
      if (token.type == TokenType::FlowEntry) {
        if (element_index == 0) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Leading comma in flow sequence"};
        }
        if (token.compact_separator) {
          found_compact_separator = true;
        }
        token = this->next_token();
        if (token.type == TokenType::FlowEntry) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Empty entry in flow sequence"};
        }
        continue;
      }

      if (token.type == TokenType::BlockMappingKey) {
        auto mapping{JSON::make_object()};
        token = this->next_token();
        if (token.type == TokenType::StreamEnd) [[unlikely]] {
          throw YAMLParseError{this->lexer_->line(), this->lexer_->column(),
                               "Unexpected end after explicit key in flow"};
        }

        std::string key_string;
        if (token.type == TokenType::Scalar) {
          key_string = this->resolve_scalar_key(token);
          token = this->next_token();
        } else {
          const auto explicit_key_line{token.line};
          const auto explicit_key_column{token.column};
          auto key_value{this->parse_value(token, JSON::ParseContext::Index,
                                           element_index, EMPTY_PROPERTY)};
          key_string = this->json_to_key_string(key_value, explicit_key_line,
                                                explicit_key_column);
          token = this->next_token();
        }

        if (token.type == TokenType::BlockMappingValue) {
          token = this->next_token();
          if (token.type != TokenType::SequenceEnd &&
              token.type != TokenType::FlowEntry) {
            auto value{this->parse_value(token, JSON::ParseContext::Property, 0,
                                         key_string)};
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

      auto value{this->parse_value(token, JSON::ParseContext::Index,
                                   element_index, EMPTY_PROPERTY)};
      result.push_back(std::move(value));
      element_index++;

      token = this->next_token();
      if (token.type == TokenType::FlowEntry && token.compact_separator) {
        found_compact_separator = true;
      }
      if (token.type != TokenType::FlowEntry &&
          token.type != TokenType::SequenceEnd) [[unlikely]] {
        throw YAMLParseError{token.line, token.column,
                             "Missing comma in flow sequence"};
      }
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Array, token.line,
                          token.column, JSON::ParseContext::Root, 0,
                          EMPTY_PROPERTY);

    if ((this->roundtrip_ != nullptr) && found_compact_separator) {
      this->roundtrip_->styles[this->pointer_stack_].compact_flow = true;
    }

    return result;
  }

  auto parse_block_sequence(const Token &start_token,
                            const JSON::ParseContext context,
                            const std::size_t index,
                            const std::string &property,
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
    const auto sequence_indent{indentation_of(start_token)};
    this->detect_indent_width(key_column, base_column);
    if ((this->roundtrip_ != nullptr) &&
        context == JSON::ParseContext::Property && key_column > 0 &&
        base_column == key_column) {
      this->roundtrip_->styles[this->pointer_stack_].unindented_sequence = true;
    }
    this->lexer_->set_block_indent(sequence_indent);
    this->record_preceding_comments_for_index(0);

    auto token{this->next_token()};
    if (token.line != start_token.line) {
      this->record_indicator_comment_for_index(element_index);
    }

    if (token.type != TokenType::BlockSequenceEntry &&
        token.type != TokenType::StreamEnd &&
        token.type != TokenType::DocumentEnd &&
        token.type != TokenType::DocumentStart) {
      auto value{this->parse_value(token, JSON::ParseContext::Index,
                                   element_index, EMPTY_PROPERTY)};
      result.push_back(std::move(value));
      element_index++;
      token = this->next_token();
    } else if ((token.type == TokenType::BlockSequenceEntry &&
                token.column == base_column) ||
               token.type == TokenType::StreamEnd ||
               token.type == TokenType::DocumentEnd ||
               token.type == TokenType::DocumentStart) {
      // The single entry has no value on this line (a sibling entry follows or
      // the stream or document ends), so it is a null entry rather than dropped
      result.push_back(JSON{nullptr});
      element_index++;
    }

    while (token.type == TokenType::BlockSequenceEntry &&
           token.column >= base_column) {
      if (element_index > 0) {
        this->record_inline_comment_for_index(element_index - 1);
      }
      this->record_preceding_comments_for_index(element_index);
      this->lexer_->set_block_indent(sequence_indent);

      if (token.column > base_column) {
        if (token.column < base_column + 2) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Wrong indentation for sequence entry"};
        }
        auto value{this->parse_value(token, JSON::ParseContext::Index,
                                     element_index, EMPTY_PROPERTY)};
        result.push_back(std::move(value));
        element_index++;
        token = this->next_token();
        continue;
      }

      const auto dash_line{token.line};
      token = this->next_token();
      if (token.line != dash_line) {
        this->record_indicator_comment_for_index(element_index);
      }

      if ((token.type == TokenType::BlockSequenceEntry &&
           token.column == base_column) ||
          token.type == TokenType::StreamEnd ||
          token.type == TokenType::DocumentEnd ||
          token.type == TokenType::DocumentStart) {
        result.push_back(JSON{nullptr});
      } else {
        auto value{this->parse_value(token, JSON::ParseContext::Index,
                                     element_index, EMPTY_PROPERTY)};
        result.push_back(std::move(value));
        token = this->next_token();
      }

      element_index++;
    }

    if (element_index > 0) {
      this->record_inline_comment_for_index(element_index - 1);
    }

    this->pending_tokens_.push_back(token);

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Array, token.line,
                          0, JSON::ParseContext::Root, 0, EMPTY_PROPERTY);

    return result;
  }

  auto parse_block_mapping(const Token &start_token,
                           const JSON::ParseContext context,
                           const std::size_t index, const std::string &property,
                           const std::uint64_t key_line = 0,
                           const std::uint64_t key_column = 0) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Object,
        this->effective_line(start_token, context, key_line),
        this->effective_column(start_token, context, key_column), context,
        index, property);

    JSON result{JSON::make_object()};
    std::unordered_set<std::string> seen_keys;

    auto token{start_token};
    const auto mapping_indent{indentation_of(start_token)};

    while (true) {
      this->lexer_->set_block_indent(mapping_indent);

      if (token.type == TokenType::BlockMappingKey) {
        auto next{this->next_token()};
        token = next;
      }

      std::optional<std::string> key_tag;
      std::optional<std::string> key_anchor;
      while (token.type == TokenType::Tag || token.type == TokenType::Anchor) {
        if (token.type == TokenType::Tag) {
          key_tag = this->resolve_tag(token.value);
        } else {
          key_anchor = std::string{token.value};
        }
        auto next{this->next_token()};
        token = next;
      }

      if (token.type != TokenType::Scalar &&
          token.type != TokenType::BlockMappingValue &&
          token.type != TokenType::Alias) {
        // RFC 8259 Section 4: an object member name is a string, so an explicit
        // mapping key that is itself a collection cannot be represented as
        // JSON. PyYAML raises on the unhashable key and js-yaml rejects the
        // complex key, so such a key is rejected rather than silently dropped
        if (token.type == TokenType::SequenceStart ||
            token.type == TokenType::MappingStart ||
            token.type == TokenType::BlockSequenceEntry) [[unlikely]] {
          throw YAMLParseError{token.line, token.column,
                               "Mapping key cannot be a collection"};
        }
        if (token.type == TokenType::DocumentEnd ||
            token.type == TokenType::DocumentStart) {
          this->pending_tokens_.push_back(token);
        }
        break;
      }

      std::string key;
      bool key_present{false};
      std::uint64_t current_key_line{0};
      std::uint64_t current_key_column{0};

      if (token.type == TokenType::Scalar) {
        key = this->resolve_scalar_key(token, key_tag);
        key_present = true;
        current_key_line = token.line;
        current_key_column = token.column;
        // How a key was written is recorded here as it is anywhere else. An
        // explicit key read on this path recorded nothing at all, so one
        // written as a block scalar, which only an explicit key can be, came
        // back out as a quoted scalar on one line
        this->record_key_scalar_style(key, token.scalar_style,
                                      token.quoted_original);

        // YAML 1.2.2 Section 7.1: an anchor on an explicit key names that key
        // for later aliases, exactly as it would on any other node
        if (key_anchor.has_value()) {
          JSON key_value{this->resolve_scalar_node(token, key_tag)};
          const auto key_expanded_weight{
              this->count_expanded_weight(key_value)};
          this->anchors_.insert_or_assign(
              key_anchor.value(),
              AnchoredValue{.value = std::move(key_value),
                            .callbacks = {},
                            .expanded_weight = key_expanded_weight});
        }

        if (seen_keys.contains(key)) [[unlikely]] {
          throw YAMLDuplicateKeyError{key, token.line, token.column};
        }
        seen_keys.insert(key);

        auto next{this->next_token()};
        if (next.type != TokenType::BlockMappingValue) {
          result.assign(key, JSON{nullptr});
          token = next;
          continue;
        }
        token = next;
      }

      // YAML 1.2.2 Section 7.1: an alias node in key position stands for the
      // value of the anchor it names, which is already resolved and so must not
      // be resolved a second time
      if (token.type == TokenType::Alias) {
        const std::string alias_name{token.value};
        const auto iterator{this->anchors_.find(alias_name)};
        if (iterator == this->anchors_.end()) [[unlikely]] {
          throw YAMLUnknownAnchorError{alias_name, token.line, token.column};
        }

        key = this->json_to_key_string(iterator->second.value, token.line,
                                       token.column);
        key_present = true;
        current_key_line = token.line;
        current_key_column = token.column;

        if (seen_keys.contains(key)) [[unlikely]] {
          throw YAMLDuplicateKeyError{key, token.line, token.column};
        }
        seen_keys.insert(key);

        auto next{this->next_token()};
        if (next.type != TokenType::BlockMappingValue) {
          result.assign(key, JSON{nullptr});
          token = next;
          continue;
        }
        token = next;
      }

      if (token.type == TokenType::BlockMappingValue) {
        auto next{this->next_token()};

        if (next.type == TokenType::StreamEnd ||
            next.type == TokenType::DocumentEnd ||
            next.type == TokenType::DocumentStart) {
          if (!key_present) {
            note_implicit_key(seen_keys, key, token);
          }

          result.assign(key, JSON{nullptr});
          token = next;
          continue;
        }

        // In round-trip mode the key text is kept raw, so a null-like key such
        // as ~ stays non-empty and the historical empty-string sentinel still
        // marks the absence of a key. In conversion mode a null-like key
        // resolves to an empty string, so a dedicated flag marks its presence
        const bool key_absent{(this->roundtrip_ != nullptr) ? key.empty()
                                                            : !key_present};
        if (next.type == TokenType::BlockMappingValue ||
            next.type == TokenType::BlockMappingKey) {
          if (key_absent && next.type == TokenType::BlockMappingKey) {
            token = next;
            continue;
          }

          if (!key_present) {
            note_implicit_key(seen_keys, key, token);
          }

          result.assign(key, JSON{nullptr});
          token = next;
          continue;
        }

        // A value indicator with nothing before it carries an empty key, which
        // Section 10.3.2 resolves to null and this module spells as the empty
        // string. What follows the indicator is that key's value, not a key of
        // its own, as Example 7.3 shows by reading a leading indicator into a
        // null key holding the scalar beside it
        auto value{this->parse_value(next, JSON::ParseContext::Property, 0, key,
                                     current_key_line, current_key_column)};
        if (!key_present) {
          note_implicit_key(seen_keys, key, token);
        }

        result.assign(key, std::move(value));

        auto after{this->next_token()};
        token = after;
      }
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Object,
                          this->lexer_->line(), this->lexer_->column(),
                          JSON::ParseContext::Root, 0, EMPTY_PROPERTY);

    return result;
  }

  auto resolve_alias(const Token &token, const JSON::ParseContext context,
                     const std::size_t index, const std::string &property,
                     const std::size_t input_read,
                     const std::uint64_t key_line = 0,
                     const std::uint64_t key_column = 0) -> JSON {
    const std::string anchor_name{token.value};
    const auto iterator{this->anchors_.find(anchor_name)};

    if (iterator == this->anchors_.end()) [[unlikely]] {
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
      std::string callback_property{record.property};

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
        callback_property.clear();
      }

      this->invoke_callback(record.phase, record.type, callback_line,
                            callback_column, callback_context, callback_idx,
                            callback_property);
      callback_index++;
    }

    this->expanded_weight_ += anchored.expanded_weight;
    if (this->expanded_weight_ > expansion_budget(input_read)) [[unlikely]] {
      throw YAMLParseError{token.line, token.column,
                           "Maximum YAML alias expansion exceeded"};
    }

    return anchored.value;
  }

  // A node that stands on a later line than the key it belongs to has to be
  // indented past the mapping for that mapping to own it. A block sequence is
  // the one exception, as it may sit at the very indentation of its key.
  // See https://yaml.org/spec/1.2.2/#821-block-sequences
  [[nodiscard]] auto starts_mapping_value(const Token &token,
                                          const std::uint64_t key_line,
                                          const std::uint64_t base_column) const
      -> bool {
    if (token.line == key_line) {
      return true;
    }

    return token.type == TokenType::BlockSequenceEntry
               ? token.column >= base_column
               : token.column > base_column;
  }

  // A node property that decorates a block node rather than a key of the
  // mapping it sits in has to be indented past that mapping, so one that opens
  // a block sequence from the mapping's own indentation has nowhere to belong.
  // See https://yaml.org/spec/1.2.2/#822-block-mappings
  auto reject_misplaced_property(const Token &property, const Token &node) const
      -> void {
    if (node.type == TokenType::BlockSequenceEntry) [[unlikely]] {
      throw YAMLParseError{property.line, property.column,
                           "Node property at wrong indentation level"};
    }
  }

  auto next_token() -> Token {
    if (this->pending_tokens_.empty()) {
      return this->lexer_->next();
    }

    auto result{this->pending_tokens_.front()};
    this->pending_tokens_.pop_front();
    if (this->pending_tokens_.empty()) {
      this->pending_token_position_.reset();
    }
    return result;
  }

  // Read the value of an entry whose indicator has just been consumed, and
  // report whether the mapping carries on after it. YAML 1.2.2 Section 8.2.2
  // leaves the value optional where the indicator is not, so a token that can
  // open no value leaves the entry empty
  auto read_block_mapping_value(JSON &result, Token &next,
                                const std::string &key,
                                const std::uint64_t key_line,
                                const std::uint64_t key_column,
                                const std::uint64_t base_column) -> bool {
    next = this->next_token();

    if (next.type == TokenType::Scalar) {
      if (this->starts_mapping_value(next, key_line, base_column)) {
        this->record_inline_comment_for_key(key, next.line != key_line);
        auto after{this->next_token()};
        this->pending_tokens_.push_back(after);
        auto value{this->parse_value(next, JSON::ParseContext::Property, 0, key,
                                     key_line, key_column)};
        result.assign(key, std::move(value));
        next = this->next_token();
      } else {
        this->record_inline_comment_for_key(key);
        result.assign(key, JSON{nullptr});
      }
    } else if (next.type == TokenType::StreamEnd ||
               next.type == TokenType::DocumentEnd ||
               next.type == TokenType::DocumentStart) {
      result.assign(key, JSON{nullptr});
      return false;
    } else if (this->starts_mapping_value(next, key_line, base_column)) {
      this->record_inline_comment_for_key(key, next.line != key_line);
      auto value{this->parse_value(next, JSON::ParseContext::Property, 0, key,
                                   key_line, key_column)};
      result.assign(key, std::move(value));
      next = this->next_token();
    } else {
      result.assign(key, JSON{nullptr});
    }

    return true;
  }

  auto parse_block_mapping_from_first_key(
      const Token &key_token, const JSON::ParseContext context,
      const std::size_t index, const std::string &property,
      const std::uint64_t parent_key_line = 0,
      const std::uint64_t parent_key_column = 0,
      const std::uint64_t node_start_column = 0,
      const std::optional<std::string> &key_tag = std::nullopt,
      const bool key_pre_resolved = false) -> JSON {
    this->invoke_callback(
        JSON::ParsePhase::Pre, JSON::Type::Object,
        this->effective_line(key_token, context, parent_key_line),
        this->effective_column(key_token, context, parent_key_column), context,
        index, property);

    JSON result{JSON::make_object()};
    std::unordered_set<std::string> seen_keys;
    const auto base_column{node_start_column > 0 ? node_start_column
                                                 : key_token.column};

    this->detect_indent_width(parent_key_column, base_column);

    // An alias key arrives already stringified from its anchor value, so it
    // must not be resolved a second time
    std::string key{key_pre_resolved
                        ? std::string{key_token.value}
                        : this->resolve_scalar_key(key_token, key_tag)};
    std::uint64_t key_line{key_token.line};
    std::uint64_t key_column{key_token.column};
    const auto first_key_line{key_token.line};
    seen_keys.insert(key);
    this->record_key_scalar_style(key, key_token.scalar_style,
                                  key_token.quoted_original);
    this->record_preceding_comments_for_key(key);

    this->lexer_->set_block_indent(static_cast<std::size_t>(base_column - 1));
    auto next{this->next_token()};

    if (next.type == TokenType::Scalar || next.type == TokenType::StreamEnd ||
        next.type == TokenType::DocumentEnd) {
      if (next.type == TokenType::Scalar &&
          this->starts_mapping_value(next, key_line, base_column)) {
        this->record_inline_comment_for_key(key, next.line != key_line);
        auto value{this->parse_value(next, JSON::ParseContext::Property, 0, key,
                                     key_line, key_column)};
        result.assign(key, std::move(value));
        this->record_inline_comment_for_key(key);
        next = this->next_token();
      } else if (next.type == TokenType::Scalar) {
        this->record_inline_comment_for_key(key);
        result.assign(key, JSON{nullptr});
      } else {
        this->invoke_callback(JSON::ParsePhase::Pre, JSON::Type::Null, key_line,
                              key_column, JSON::ParseContext::Property, 0, key);
        const auto null_post_column{key_column +
                                    static_cast<std::uint64_t>(key.size())};
        this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Null,
                              key_line, null_post_column,
                              JSON::ParseContext::Root, 0, EMPTY_PROPERTY);
        result.assign(key, JSON{nullptr});
      }
    } else if (next.type == TokenType::MappingStart ||
               next.type == TokenType::SequenceStart ||
               next.type == TokenType::BlockSequenceEntry ||
               next.type == TokenType::BlockMappingKey ||
               next.type == TokenType::Anchor || next.type == TokenType::Tag ||
               next.type == TokenType::Alias) {
      if (!this->starts_mapping_value(next, key_line, base_column)) {
        result.assign(key, JSON{nullptr});
      } else {
        if (next.type == TokenType::BlockSequenceEntry && next.line == key_line)
            [[unlikely]] {
          throw YAMLParseError{
              next.line, next.column,
              "Block sequence entry on same line as mapping key"};
        }
        // YAML 1.2.2 Section 8.2.2 reaches the block collection of an entry
        // through `s-l-comments`, which ends on a line break, so a nested
        // mapping opens on a line of its own
        if (next.type == TokenType::BlockMappingKey && next.line == key_line)
            [[unlikely]] {
          throw YAMLParseError{
              next.line, next.column,
              "Explicit mapping key in block value on same line as parent key"};
        }
        this->record_inline_comment_for_key(key, next.line != key_line);
        auto value{this->parse_value(next, JSON::ParseContext::Property, 0, key,
                                     key_line, key_column)};
        result.assign(key, std::move(value));
        next = this->next_token();
        this->record_inline_comment_for_key(key);
      }
    } else {
      result.assign(key, JSON{nullptr});
    }

    while ((next.type == TokenType::Scalar ||
            next.type == TokenType::BlockMappingKey ||
            next.type == TokenType::BlockMappingValue ||
            next.type == TokenType::Anchor || next.type == TokenType::Tag ||
            next.type == TokenType::Alias)) {
      if (this->document_start_line_ > 0 &&
          first_key_line == this->document_start_line_ &&
          next.line != this->document_start_line_) [[unlikely]] {
        throw YAMLParseError{next.line, next.column,
                             "Block mapping continuation after document "
                             "start line"};
      }
      this->lexer_->set_block_indent(static_cast<std::size_t>(base_column - 1));

      if (next.type == TokenType::BlockMappingKey) {
        if (next.column != base_column) {
          break;
        }
        next = this->next_token();

        // YAML 1.2.2 Section 7.1: an anchor on an explicit key names that key
        // for later aliases, exactly as it would on any other node
        std::optional<std::string> explicit_key_anchor;
        if (next.type == TokenType::Anchor) {
          explicit_key_anchor = std::string{next.value};
          next = this->next_token();
        }

        if (next.type == TokenType::Alias) {
          // YAML 1.2.2 Section 7.1: an alias node in key position stands for
          // the value of the anchor it names, which is already resolved and so
          // must not be resolved a second time
          const std::string alias_name{next.value};
          const auto iterator{this->anchors_.find(alias_name)};
          if (iterator == this->anchors_.end()) [[unlikely]] {
            throw YAMLUnknownAnchorError{alias_name, next.line, next.column};
          }

          key = this->json_to_key_string(iterator->second.value, next.line,
                                         next.column);
        } else if (next.type != TokenType::Scalar) {
          result.assign("", JSON{nullptr});
          next = this->next_token();
          continue;
        } else {
          key = this->resolve_scalar_key(next);
          this->record_key_scalar_style(key, next.scalar_style,
                                        next.quoted_original);
          if (explicit_key_anchor.has_value()) {
            JSON key_value{this->resolve_scalar_node(next)};
            const auto key_expanded_weight{
                this->count_expanded_weight(key_value)};
            this->anchors_.insert_or_assign(
                explicit_key_anchor.value(),
                AnchoredValue{.value = std::move(key_value),
                              .callbacks = {},
                              .expanded_weight = key_expanded_weight});
          }
        }

        key_line = next.line;
        key_column = next.column;

        if (seen_keys.contains(key)) [[unlikely]] {
          throw YAMLDuplicateKeyError{key, next.line, next.column};
        }
        seen_keys.insert(key);

        auto colon{this->next_token()};
        if (colon.type != TokenType::BlockMappingValue) {
          result.assign(key, JSON{nullptr});
          this->pending_tokens_.push_back(colon);
          next = this->next_token();
          continue;
        }

        next = this->next_token();
        if (next.type == TokenType::StreamEnd ||
            next.type == TokenType::DocumentEnd ||
            next.type == TokenType::DocumentStart) {
          result.assign(key, JSON{nullptr});
          this->pending_tokens_.push_back(next);
          break;
        }
        if (next.type == TokenType::BlockMappingValue ||
            next.type == TokenType::BlockMappingKey) {
          result.assign(key, JSON{nullptr});
        } else {
          this->record_inline_comment_for_key(key, next.line != key_line);
          this->lexer_->set_block_indent(
              static_cast<std::size_t>(base_column - 1));
          auto value{this->parse_value(next, JSON::ParseContext::Property, 0,
                                       key, key_line, key_column)};
          result.assign(key, std::move(value));
          next = this->next_token();
        }
        continue;
      }

      auto effective_column{next.column};
      std::optional<std::string> subsequent_key_tag;

      // A node property introduces the key it decorates, so a mapping that
      // does not reach that key must leave the property alone as well
      if ((next.type == TokenType::Anchor || next.type == TokenType::Tag) &&
          effective_column != base_column) {
        break;
      }

      if (next.type == TokenType::Anchor) {
        const auto property_token{next};
        next = this->next_token();
        this->reject_misplaced_property(property_token, next);
        if (next.type != TokenType::Scalar) {
          continue;
        }
      }

      if (next.type == TokenType::Tag) {
        const auto property_token{next};
        subsequent_key_tag = this->resolve_tag(next.value);
        next = this->next_token();
        this->reject_misplaced_property(property_token, next);
        if (next.type != TokenType::Scalar) {
          continue;
        }
      }

      if (next.type == TokenType::Alias) {
        if (effective_column != base_column) {
          break;
        }
        const std::string alias_name{next.value};
        const auto iterator{this->anchors_.find(alias_name)};
        if (iterator == this->anchors_.end()) [[unlikely]] {
          throw YAMLUnknownAnchorError{alias_name, next.line, next.column};
        }
        key = this->json_to_key_string(iterator->second.value, next.line,
                                       next.column);
        key_line = next.line;
        key_column = next.column;

        if (seen_keys.contains(key)) [[unlikely]] {
          throw YAMLDuplicateKeyError{key, next.line, next.column};
        }
        seen_keys.insert(key);

        // YAML 1.2.2 production 193 has the value of an implicit block
        // mapping entry follow a literal colon, so an alias standing without
        // one names no entry, exactly as a plain scalar standing without one
        // names none. An explicit key is the one that may leave its value out
        auto colon{this->next_token()};
        if (colon.type != TokenType::BlockMappingValue) {
          this->pending_tokens_.push_back(colon);
          break;
        }

        next = this->next_token();

        if (next.type == TokenType::Scalar) {
          if (this->starts_mapping_value(next, key_line, base_column)) {
            auto value{this->parse_value(next, JSON::ParseContext::Property, 0,
                                         key, key_line, key_column)};
            result.assign(key, std::move(value));
            next = this->next_token();
          } else {
            result.assign(key, JSON{nullptr});
          }
        } else if (next.type == TokenType::StreamEnd ||
                   next.type == TokenType::DocumentEnd ||
                   next.type == TokenType::DocumentStart) {
          result.assign(key, JSON{nullptr});
          break;
        } else if (this->starts_mapping_value(next, key_line, base_column)) {
          auto value{this->parse_value(next, JSON::ParseContext::Property, 0,
                                       key, key_line, key_column)};
          result.assign(key, std::move(value));
          next = this->next_token();
        } else {
          result.assign(key, JSON{nullptr});
        }
        continue;
      }

      // YAML 1.2.2 production 192 writes an entry as
      // `( ns-s-block-map-implicit-key | e-node ) c-l-block-map-implicit-value`
      // so the key of any entry may be empty, and Section 8.2.2 settles what
      // makes it one: "while both the implicit key and the value following it
      // may be empty, the ':' indicator is mandatory"
      if (next.type == TokenType::BlockMappingValue) {
        if (next.column != base_column) {
          break;
        }

        this->record_inline_comment_for_key(key);
        key.clear();
        key_line = next.line;
        key_column = next.column;

        if (seen_keys.contains(key)) [[unlikely]] {
          throw YAMLDuplicateKeyError{key, next.line, next.column};
        }
        seen_keys.insert(key);

        if (!this->read_block_mapping_value(result, next, key, key_line,
                                            key_column, base_column)) {
          break;
        }

        continue;
      }

      if (effective_column != base_column) {
        break;
      }

      this->record_inline_comment_for_key(key);
      key = this->resolve_scalar_key(next, subsequent_key_tag);
      key_line = next.line;
      key_column = next.column;
      this->record_key_scalar_style(key, next.scalar_style,
                                    next.quoted_original);
      this->record_preceding_comments_for_key(key);

      if (next.multiline) [[unlikely]] {
        throw YAMLParseError{next.line, next.column,
                             "Multi-line implicit mapping key"};
      }

      if (seen_keys.contains(key)) [[unlikely]] {
        throw YAMLDuplicateKeyError{key, next.line, next.column};
      }
      seen_keys.insert(key);

      auto colon{this->next_token()};
      if (colon.type != TokenType::BlockMappingValue) {
        this->pending_tokens_.push_back(colon);
        break;
      }

      if (!this->read_block_mapping_value(result, next, key, key_line,
                                          key_column, base_column)) {
        break;
      }
    }

    this->record_inline_comment_for_key(key);

    if (next.type != TokenType::StreamEnd) {
      this->pending_tokens_.push_back(next);
      if (next.type == TokenType::DocumentStart) {
        this->pending_token_position_ = next.position;
      }
    }

    this->invoke_callback(JSON::ParsePhase::Post, JSON::Type::Object,
                          this->lexer_->line(), this->lexer_->column(),
                          JSON::ParseContext::Root, 0, EMPTY_PROPERTY);

    return result;
  }

  auto record_preceding_comments_for_key(const std::string &key) -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }
    auto comments{this->lexer_->take_preceding_comments()};
    if (comments.empty()) {
      return;
    }
    this->pointer_stack_.push_back(key);
    this->roundtrip_->styles[this->pointer_stack_].comments_before =
        std::move(comments);
    this->pointer_stack_.pop_back();
  }

  auto record_inline_comment_for_key(const std::string &key,
                                     const bool on_indicator = false) -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }
    auto comment{this->lexer_->take_inline_comment()};
    if (!comment.has_value()) {
      return;
    }
    this->pointer_stack_.push_back(key);
    if (on_indicator) {
      this->roundtrip_->styles[this->pointer_stack_].comment_on_indicator =
          std::move(comment);
    } else {
      this->roundtrip_->styles[this->pointer_stack_].comment_inline =
          std::move(comment);
    }
    this->pointer_stack_.pop_back();
  }

  auto record_preceding_comments_for_index(const std::size_t index) -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }
    auto comments{this->lexer_->take_preceding_comments()};
    if (comments.empty()) {
      return;
    }
    this->pointer_stack_.push_back(index);
    this->roundtrip_->styles[this->pointer_stack_].comments_before =
        std::move(comments);
    this->pointer_stack_.pop_back();
  }

  auto record_inline_comment_for_index(const std::size_t index) -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }
    auto comment{this->lexer_->take_inline_comment()};
    if (!comment.has_value()) {
      return;
    }
    this->pointer_stack_.push_back(index);
    this->roundtrip_->styles[this->pointer_stack_].comment_inline =
        std::move(comment);
    this->pointer_stack_.pop_back();
  }

  auto record_indicator_comment_for_index(const std::size_t index) -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }
    this->pointer_stack_.push_back(index);
    auto indicator_comment{this->lexer_->take_inline_comment()};
    this->roundtrip_->styles[this->pointer_stack_].comment_on_indicator =
        std::move(indicator_comment).value_or(std::string{});
    this->pointer_stack_.pop_back();
  }

  auto
  // A tag can make an empty node stand for something other than null, so what
  // is announced and what is filed are both taken from the value itself.
  // Announcing null regardless told whoever is listening a different type from
  // the one the node came back as
  register_anchored_empty(const std::string_view anchor_name, const JSON &value,
                          const Token &token, const JSON::ParseContext context,
                          const std::size_t index, const std::string &property,
                          std::optional<std::string> &inline_comment) -> void {
    this->recording_anchor_ = true;
    this->current_anchor_callbacks_.clear();
    const auto type{value.type()};
    this->invoke_callback(JSON::ParsePhase::Pre, type, token.line, token.column,
                          context, index, property);
    this->invoke_callback(JSON::ParsePhase::Post, type, token.line,
                          token.column, JSON::ParseContext::Root, 0,
                          EMPTY_PROPERTY);
    this->recording_anchor_ = false;
    this->anchors_.insert_or_assign(
        std::string{anchor_name},
        AnchoredValue{.value = value,
                      .callbacks = std::move(this->current_anchor_callbacks_),
                      .expanded_weight = this->count_expanded_weight(value)});
    this->current_anchor_callbacks_.clear();
    if (this->roundtrip_ != nullptr) {
      auto &style{this->roundtrip_->styles[this->pointer_stack_]};
      style.anchor = std::string{anchor_name};
      if (inline_comment.has_value()) {
        style.comment_inline = std::move(inline_comment);
      }
    }
  }

  auto record_collection_style(const YAMLRoundTrip::CollectionStyle style)
      -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }

    this->roundtrip_->styles[this->pointer_stack_].collection = style;
  }

  auto record_tag(const std::optional<std::string> &raw_tag,
                  const bool tag_before_anchor, const JSON &value) -> void {
    if ((this->roundtrip_ == nullptr) || !raw_tag.has_value()) {
      return;
    }

    auto &node_style{this->roundtrip_->styles[this->pointer_stack_]};
    node_style.tag = raw_tag.value();
    node_style.tag_type = value.type();
    node_style.tag_before_anchor = tag_before_anchor;
  }

  auto record_scalar_style(const Token &token, const JSON &value) -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }

    auto &node_style{this->roundtrip_->styles[this->pointer_stack_]};

    switch (token.scalar_style) {
      case ScalarStyle::Plain:
        node_style.scalar = YAMLRoundTrip::ScalarStyle::Plain;
        if (token.multiline && !token.block_original.empty()) {
          node_style.plain_content = std::string{token.block_original};
        } else {
          node_style.plain_content = std::string{token.value};
        }
        node_style.content_value = value;
        break;
      case ScalarStyle::SingleQuoted:
        node_style.scalar = YAMLRoundTrip::ScalarStyle::SingleQuoted;
        if (!token.quoted_original.empty()) {
          node_style.quoted_content = std::string{token.quoted_original};
          node_style.content_value = value;
        }
        break;
      case ScalarStyle::DoubleQuoted:
        node_style.scalar = YAMLRoundTrip::ScalarStyle::DoubleQuoted;
        if (!token.quoted_original.empty()) {
          node_style.quoted_content = std::string{token.quoted_original};
          node_style.content_value = value;
        }
        break;
      case ScalarStyle::Literal:
        node_style.scalar = YAMLRoundTrip::ScalarStyle::Literal;
        break;
      case ScalarStyle::Folded:
        node_style.scalar = YAMLRoundTrip::ScalarStyle::Folded;
        break;
    }

    if (token.scalar_style == ScalarStyle::Literal ||
        token.scalar_style == ScalarStyle::Folded) {
      node_style.chomping = to_roundtrip_chomping(token.chomping);

      node_style.explicit_indent = token.explicit_indent;
      node_style.indent_before_chomping = token.indent_before_chomping;

      if (!token.block_original.empty()) {
        node_style.block_content = std::string{token.block_original};
        node_style.content_value = value;
      }

      auto block_comment{this->lexer_->take_block_scalar_comment()};
      if (block_comment.has_value()) {
        node_style.comment_inline = std::move(block_comment);
      }
    }
  }

  // An empty key is a key like any other, so naming it twice names the same
  // key twice. It is recorded where the entry is committed rather than where
  // the indicator is read, since an indicator that turns out to open an
  // explicit key commits nothing and would otherwise mark a key the mapping
  // never holds as one it has already seen
  static auto note_implicit_key(std::unordered_set<std::string> &seen_keys,
                                const std::string &key, const Token &token)
      -> void {
    if (seen_keys.contains(key)) [[unlikely]] {
      throw YAMLDuplicateKeyError{key, token.line, token.column};
    }

    seen_keys.insert(key);
  }

  auto record_key_scalar_style(const std::string &key, const ScalarStyle style,
                               const std::string_view quoted_original = {})
      -> void {
    if (this->roundtrip_ == nullptr) {
      return;
    }
    this->pointer_stack_.push_back(key);
    switch (style) {
      case ScalarStyle::Plain:
        this->roundtrip_->key_styles[this->pointer_stack_] =
            YAMLRoundTrip::ScalarStyle::Plain;
        break;
      case ScalarStyle::SingleQuoted:
        this->roundtrip_->key_styles[this->pointer_stack_] =
            YAMLRoundTrip::ScalarStyle::SingleQuoted;
        break;
      case ScalarStyle::DoubleQuoted:
        this->roundtrip_->key_styles[this->pointer_stack_] =
            YAMLRoundTrip::ScalarStyle::DoubleQuoted;
        break;
      // A block scalar key is left to be written as a quoted scalar. What one
      // stands for rests on the indicator, the chomping and the folding it was
      // written with together, and writing it back without all three carries a
      // different key than the one that was read
      case ScalarStyle::Literal:
      case ScalarStyle::Folded:
        break;
    }
    // A key is written back without the indicator that can open an explicit
    // one, so text spanning lines cannot be replayed as it was read. Keeping
    // it would put a line break inside a key that nothing marks as explicit,
    // which is not a document that can be read again, where writing the key
    // out afresh keeps it to one line
    if (!quoted_original.empty() &&
        quoted_original.find('\n') == std::string_view::npos) {
      this->roundtrip_->key_quoted_contents[this->pointer_stack_] =
          std::string{quoted_original};
    }
    this->pointer_stack_.pop_back();
  }

  auto detect_indent_width(const std::uint64_t parent_column,
                           const std::uint64_t child_column) -> void {
    if ((this->roundtrip_ == nullptr) || this->indent_width_detected_) {
      return;
    }
    if (parent_column > 0 && child_column > parent_column) {
      this->roundtrip_->indent_width =
          static_cast<std::size_t>(child_column - parent_column);
      this->indent_width_detected_ = true;
    }
  }

  inline static const std::string EMPTY_PROPERTY{};
  Lexer *lexer_;
  const JSON::ParseCallback *callback_;
  YAMLRoundTrip *roundtrip_{nullptr};
  Pointer pointer_stack_;
  std::unordered_map<std::string, AnchoredValue> anchors_;
  bool recording_anchor_{false};
  bool indent_width_detected_{false};
  std::size_t expanded_weight_{0};
  std::vector<CallbackRecord> current_anchor_callbacks_;
  std::deque<Token> pending_tokens_;
  std::optional<std::size_t> pending_token_position_;
  std::unordered_map<std::string, std::string> tag_directives_;
  std::uint64_t document_start_line_{0};
  // Whether the document parsed by the most recent call ended with an explicit
  // document end marker, so a subsequent stream validation knows a document
  // boundary was already crossed
  bool document_ended_{false};
};

} // namespace sourcemeta::core::yaml

#endif
