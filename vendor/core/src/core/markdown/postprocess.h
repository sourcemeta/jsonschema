#ifndef SOURCEMETA_CORE_MARKDOWN_POSTPROCESS_H_
#define SOURCEMETA_CORE_MARKDOWN_POSTPROCESS_H_

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include "characters.h"
#include "document.h"
#include "inlines.h"
#include "references.h"

#include <algorithm>     // std::sort
#include <array>         // std::array
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint32_t
#include <cstring>       // std::memchr
#include <functional>    // std::less
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace sourcemeta::core::markdown {

inline auto is_leaf_type(const NodeType type) noexcept -> bool {
  return type == NodeType::HTMLBlock || type == NodeType::ThematicBreak ||
         type == NodeType::CodeBlock || type == NodeType::Text ||
         type == NodeType::SoftBreak || type == NodeType::LineBreak ||
         type == NodeType::Code || type == NodeType::HTMLInline;
}

// The node after the given one in document order when skipping its
// descendants, without leaving the subtree of the given root, or no node at
// the end of that subtree
inline auto next_skipping_descendants(const Document &document,
                                      std::uint32_t index,
                                      const std::uint32_t root) noexcept
    -> std::uint32_t {
  while (index != root && index != NO_NODE) {
    const auto &current{document.nodes[index]};
    if (current.next != NO_NODE) {
      return current.next;
    }

    index = current.parent;
  }

  return NO_NODE;
}

// A character of the part of an email address before the at sign, which GFM
// section 6.9 makes of characters "which are alphanumeric, or ., -, _, or +"
inline auto is_email_local_character(const char character) noexcept -> bool {
  return sourcemeta::core::is_alphanum(character) || character == '.' ||
         character == '-' || character == '_' || character == '+';
}

// The passes that run on the block structure once it is complete
class PostProcessor {
public:
  explicit PostProcessor(Document &document) : document_{document} {}

  // Parse the inlines of every leaf block in document order, which is the
  // order in which references count towards their expansion limit. Blocks
  // without footnote references are finished right away, while their nodes
  // are still in the cache, and the rest wait for footnotes to resolve
  auto parse_inlines(InlineParser &parser) -> void {
    this->deferred_.clear();
    auto current{this->document_.nodes[ROOT_NODE].first_child};
    while (current != NO_NODE) {
      const auto type{this->document_.nodes[current].type};
      if (type == NodeType::Paragraph || type == NodeType::Heading ||
          type == NodeType::TableCell) {
        const auto references{this->document_.footnote_reference_nodes.size()};
        parser.parse(current);
        if (this->document_.footnote_reference_nodes.size() == references) {
          this->finish_inlines(current);
        } else {
          this->deferred_.push_back(current);
        }

        current =
            next_skipping_descendants(this->document_, current, ROOT_NODE);
      } else if (this->document_.nodes[current].first_child != NO_NODE) {
        current = this->document_.nodes[current].first_child;
      } else {
        current =
            next_skipping_descendants(this->document_, current, ROOT_NODE);
      }
    }
  }

  // Number footnote definitions in the order in which they are first
  // referenced and move them to the end of the document, dropping those that
  // are never referenced or that repeat a label
  auto process_footnotes() -> void {
    const auto &definitions{this->document_.footnote_definition_nodes};
    const auto &references{this->document_.footnote_reference_nodes};
    if (definitions.empty() && references.empty()) {
      return;
    }

    auto &nodes{this->document_.nodes};
    this->definitions_.clear();
    this->definition_nodes_.clear();
    // Without nesting, the order in which definitions were opened matches the
    // order in which they end, which is the order in which labels claim them
    if (this->document_.nested_footnote_definitions) {
      this->register_definitions_in_post_order();
    } else {
      for (const auto definition : definitions) {
        this->register_definition(definition);
      }
    }

    std::uint32_t last_index{0};
    for (const auto reference : references) {
      if (this->is_attached(reference)) {
        this->resolve_reference(reference, last_index);
      }
    }

    this->ordered_.clear();
    for (const auto &entry : this->definitions_) {
      this->ordered_.push_back(entry.second);
    }

    std::sort(this->ordered_.begin(), this->ordered_.end(),
              [&nodes](const std::uint32_t left, const std::uint32_t right) {
                return nodes[left].extra < nodes[right].extra;
              });
    for (const auto definition : this->ordered_) {
      this->document_.unlink(definition);
      if (nodes[definition].extra > 0) {
        set_flag(nodes[definition], FLAG_REFERENCED, true);
        this->document_.append_child(ROOT_NODE, definition);
      }
    }

    for (const auto definition : this->definition_nodes_) {
      if (!has_flag(nodes[definition], FLAG_REFERENCED)) {
        this->document_.unlink(definition);
      }
    }

    for (const auto block : this->deferred_) {
      this->finish_inlines(block);
    }
  }

private:
  // Merge the adjacent text nodes of a leaf block and turn the email
  // addresses of GFM section 6.9 that appear outside of links into links
  auto finish_inlines(const std::uint32_t block) -> void {
    auto current{this->document_.nodes[block].first_child};
    while (current != NO_NODE) {
      if (this->document_.nodes[current].type == NodeType::Text) {
        this->merge_following_text(current);
      }

      if (this->document_.nodes[current].first_child != NO_NODE) {
        current = this->document_.nodes[current].first_child;
      } else {
        current = next_skipping_descendants(this->document_, current, block);
      }
    }

    // Only text with an at sign, which a character reference may produce, can
    // have email addresses
    const auto content{
        this->document_.content_of(this->document_.nodes[block])};
    if (content.empty() ||
        (std::memchr(content.data(), '@', content.size()) == nullptr &&
         std::memchr(content.data(), '&', content.size()) == nullptr)) {
      return;
    }

    current = this->document_.nodes[block].first_child;
    while (current != NO_NODE) {
      const auto type{this->document_.nodes[current].type};
      if (type == NodeType::Text) {
        const auto next{
            next_skipping_descendants(this->document_, current, block)};
        this->link_emails_in_text(current);
        current = next;
      } else if (type != NodeType::Link &&
                 this->document_.nodes[current].first_child != NO_NODE) {
        current = this->document_.nodes[current].first_child;
      } else {
        current = next_skipping_descendants(this->document_, current, block);
      }
    }
  }

  auto register_definitions_in_post_order() -> void {
    const auto &nodes{this->document_.nodes};
    auto current{ROOT_NODE};
    bool entering{true};
    while (true) {
      if (entering && !is_leaf_type(nodes[current].type) &&
          nodes[current].first_child != NO_NODE) {
        current = nodes[current].first_child;
        continue;
      }

      if (nodes[current].type == NodeType::FootnoteDefinition) {
        this->register_definition(current);
      }

      if (current == ROOT_NODE) {
        break;
      }

      entering = nodes[current].next != NO_NODE;
      current = entering ? nodes[current].next : nodes[current].parent;
    }
  }

  auto register_definition(const std::uint32_t index) -> void {
    normalize_label(this->label_buffer_, this->document_.nodes[index].literal);
    if (this->label_buffer_.empty()) {
      return;
    }

    this->definition_nodes_.push_back(index);
    if (!this->definitions_.contains(std::string_view{this->label_buffer_})) {
      this->definitions_.emplace(
          this->document_.strings.store(this->label_buffer_), index);
    }
  }

  auto resolve_reference(const std::uint32_t index, std::uint32_t &last_index)
      -> void {
    auto &nodes{this->document_.nodes};
    const auto label{nodes[index].literal};
    auto definition{NO_NODE};
    if (sourcemeta::core::utf8_codepoint_within(label, 1,
                                                MAXIMUM_LINK_LABEL_LENGTH)) {
      normalize_label(this->label_buffer_, label);
      const auto match{
          this->definitions_.find(std::string_view{this->label_buffer_})};
      if (match != this->definitions_.end()) {
        definition = match->second;
      }
    }

    if (definition == NO_NODE) {
      this->buffer_.assign("[^");
      this->buffer_.append(label);
      this->buffer_.push_back(']');
      const auto text{this->document_.create(NodeType::Text)};
      nodes[text].literal = this->document_.strings.store(this->buffer_);
      this->document_.insert_after(index, text);
      this->document_.unlink(index);
      return;
    }

    if (nodes[definition].extra == 0) {
      ++last_index;
      nodes[definition].extra = last_index;
    }

    ++nodes[definition].data;
    nodes[index].data = definition;
    nodes[index].extra = nodes[definition].data;
    sourcemeta::core::DigitsBuffer digits;
    nodes[index].literal = this->document_.strings.store(
        sourcemeta::core::digits_view(nodes[definition].extra, digits));
  }

  [[nodiscard]] auto is_in_content(const std::string_view value) const noexcept
      -> bool {
    const std::less<const char *> before{};
    const auto *const begin{this->document_.content.data()};
    const auto *const end{begin + this->document_.content.size()};
    return !before(value.data(), begin) &&
           !before(end, value.data() + value.size());
  }

  auto merge_following_text(const std::uint32_t index) -> void {
    auto &nodes{this->document_.nodes};
    auto next{nodes[index].next};
    if (next == NO_NODE || nodes[next].type != NodeType::Text) {
      return;
    }

    auto literal{nodes[index].literal};
    bool buffered{false};
    while (next != NO_NODE && nodes[next].type == NodeType::Text) {
      const auto piece{nodes[next].literal};
      if (buffered) {
        this->buffer_.append(piece);
      } else if (literal.empty()) {
        literal = piece;
      } else if (!piece.empty()) {
        if (literal.data() + literal.size() == piece.data() &&
            this->is_in_content(literal) && this->is_in_content(piece)) {
          literal =
              std::string_view{literal.data(), literal.size() + piece.size()};
        } else {
          this->buffer_.assign(literal);
          this->buffer_.append(piece);
          buffered = true;
        }
      }

      const auto following{nodes[next].next};
      this->document_.unlink(next);
      next = following;
    }

    nodes[index].literal =
        buffered ? this->document_.strings.store(this->buffer_) : literal;
  }

  // Whether a node is still part of the document, remembering the answer for
  // every ancestor on the way, so that the references of a deeply nested block
  // do not walk the same ancestors over and over
  auto is_attached(const std::uint32_t index) -> bool {
    auto &nodes{this->document_.nodes};
    this->ancestors_.clear();
    auto current{index};
    while (current != ROOT_NODE && current != NO_NODE &&
           !has_flag(nodes[current], FLAG_ATTACHED) &&
           !has_flag(nodes[current], FLAG_DETACHED)) {
      this->ancestors_.push_back(current);
      current = nodes[current].parent;
    }

    const auto attached{
        current == ROOT_NODE ||
        (current != NO_NODE && has_flag(nodes[current], FLAG_ATTACHED))};
    for (const auto ancestor : this->ancestors_) {
      set_flag(nodes[ancestor], attached ? FLAG_ATTACHED : FLAG_DETACHED, true);
    }

    return attached;
  }

  // Whether an extended autolink may start at the beginning of a text node,
  // where GFM section 6.9 says that such autolinks "can only come at the
  // beginning of a line, after whitespace, or any of the delimiting characters
  // *, _, ~, and (", which are also the characters around emphasis, strong
  // emphasis, and strikethrough
  [[nodiscard]] auto may_start_autolink_at(std::uint32_t text) const noexcept
      -> bool {
    const auto &nodes{this->document_.nodes};
    while (true) {
      const auto previous{nodes[text].previous};
      if (previous == NO_NODE) {
        return nodes[nodes[text].parent].type != NodeType::Image;
      }

      const auto &node{nodes[previous]};
      switch (node.type) {
        case NodeType::Text:
          if (!node.literal.empty()) {
            return may_precede_extended_autolink(node.literal.back());
          }

          text = previous;
          break;
        case NodeType::SoftBreak:
        case NodeType::LineBreak:
        case NodeType::Emphasis:
        case NodeType::Strong:
        case NodeType::Strikethrough:
          return true;
        case NodeType::Document:
        case NodeType::BlockQuote:
        case NodeType::List:
        case NodeType::Item:
        case NodeType::CodeBlock:
        case NodeType::HTMLBlock:
        case NodeType::Paragraph:
        case NodeType::Heading:
        case NodeType::ThematicBreak:
        case NodeType::FootnoteDefinition:
        case NodeType::Table:
        case NodeType::TableRow:
        case NodeType::TableCell:
        case NodeType::Code:
        case NodeType::HTMLInline:
        case NodeType::Link:
        case NodeType::Image:
        case NodeType::FootnoteReference:
          return false;
      }
    }
  }

  // Turn the email addresses of GFM section 6.9 in a text node into links,
  // along with the mailto and xmpp protocols that may come before them
  auto link_emails_in_text(std::uint32_t text) -> void {
    auto &nodes{this->document_.nodes};
    const auto data{nodes[text].literal};
    const auto first_text{text};
    // The position of the data at which the current text node starts
    std::size_t consumed{0};
    std::size_t search{0};
    while (search < data.size()) {
      const auto *const found{static_cast<const char *>(
          std::memchr(data.data() + search, '@', data.size() - search))};
      if (found == nullptr) {
        break;
      }

      const auto separator{static_cast<std::size_t>(found - data.data())};
      search = separator + 1;
      auto link_start{separator};
      while (link_start > consumed &&
             is_email_local_character(data[link_start - 1])) {
        --link_start;
      }

      if (link_start == separator) {
        continue;
      }

      // GFM section 6.9: "One or more characters which are alphanumeric, or -
      // or _, separated by periods (.). There must be at least one period. The
      // last character must not be one of - or _", where "only . may occur at
      // the end of the email address, in which case it will not be considered
      // part of the address"
      auto end{separator + 1};
      std::size_t periods{0};
      while (end < data.size()) {
        if (is_domain_character(data[end])) {
          ++end;
        } else if (data[end] == '.' && end > separator + 1 &&
                   end + 1 < data.size() &&
                   is_domain_character(data[end + 1])) {
          ++periods;
          ++end;
        } else {
          break;
        }
      }

      if (periods == 0 || data[end - 1] == '-' || data[end - 1] == '_') {
        continue;
      }

      // GFM section 6.9: "An extended protocol autolink will be recognised
      // when a protocol is recognised within any text node", where the valid
      // protocols are mailto and xmpp
      bool has_protocol{false};
      bool xmpp{false};
      for (const auto protocol :
           std::array<std::string_view, 2>{{"mailto:", "xmpp:"}}) {
        if (link_start - consumed >= protocol.size() &&
            sourcemeta::core::starts_with_ignore_case(
                data.substr(link_start - protocol.size(), protocol.size()),
                protocol)) {
          link_start -= protocol.size();
          has_protocol = true;
          xmpp = protocol == "xmpp:";
          break;
        }
      }

      // GFM section 6.9: xmpp "offers an optional / followed by a resource.
      // The resource can contain all alphanumeric characters, as well as @ and
      // .", while "Further / characters are not considered part of the domain"
      if (xmpp && end + 1 < data.size() && data[end] == '/') {
        auto resource_end{end + 1};
        while (resource_end < data.size() &&
               (sourcemeta::core::is_alphanum(data[resource_end]) ||
                data[resource_end] == '@' || data[resource_end] == '.')) {
          ++resource_end;
        }

        while (resource_end > end + 1 && data[resource_end - 1] == '.') {
          --resource_end;
        }

        if (resource_end > end + 1) {
          end = resource_end;
        }
      }

      if (link_start == consumed
              ? consumed > 0 || !this->may_start_autolink_at(first_text)
              : !may_precede_extended_autolink(data[link_start - 1])) {
        continue;
      }

      const auto address{data.substr(link_start, end - link_start)};
      const auto link{this->document_.create(NodeType::Link)};
      if (has_protocol) {
        nodes[link].literal = address;
      } else {
        this->buffer_.assign("mailto:");
        this->buffer_.append(address);
        nodes[link].literal = this->document_.strings.store(this->buffer_);
      }

      const auto link_text{this->document_.create(NodeType::Text)};
      nodes[link_text].literal = address;
      this->document_.append_child(link, link_text);
      this->document_.insert_after(text, link);
      const auto after{this->document_.create(NodeType::Text)};
      nodes[after].literal = data.substr(end);
      this->document_.insert_after(link, after);
      nodes[text].literal = data.substr(consumed, link_start - consumed);
      text = after;
      consumed = end;
      search = end;
    }
  }

  Document &document_;
  std::unordered_map<std::string_view, std::uint32_t> definitions_;
  std::vector<std::uint32_t> definition_nodes_;
  std::vector<std::uint32_t> ordered_;
  std::vector<std::uint32_t> deferred_;
  std::vector<std::uint32_t> ancestors_;
  std::string label_buffer_;
  std::string buffer_;
};

} // namespace sourcemeta::core::markdown

#endif
