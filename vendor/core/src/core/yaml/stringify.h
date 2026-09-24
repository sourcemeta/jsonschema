#ifndef SOURCEMETA_CORE_YAML_STRINGIFY_H_
#define SOURCEMETA_CORE_YAML_STRINGIFY_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/yaml_roundtrip.h>

#include <algorithm>     // std::max
#include <array>         // std::array
#include <cassert>       // assert
#include <charconv>      // std::to_chars
#include <cmath>         // std::modf
#include <cstddef>       // std::size_t
#include <optional>      // std::optional
#include <ostream>       // std::basic_ostream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <utility>       // std::pair
#include <vector>        // std::vector

namespace sourcemeta::core::yaml {

using OutputStream = std::basic_ostream<JSON::Char, JSON::CharTraits>;

static constexpr std::size_t INDENT_WIDTH{2};
static constexpr std::size_t ONE_COLUMN{1};
static constexpr std::array<char, 16> HEX_DIGITS{{'0', '1', '2', '3', '4', '5',
                                                  '6', '7', '8', '9', 'a', 'b',
                                                  'c', 'd', 'e', 'f'}};

// A processor is free to write line breaks with whatever convention suits the
// document. See https://yaml.org/spec/1.2.2/#54-line-break-characters
inline auto write_break(OutputStream &stream, const YAMLRoundTrip *roundtrip)
    -> void {
  if ((roundtrip != nullptr) && roundtrip->carriage_returns) {
    stream.put('\r');
  }

  stream.put('\n');
}

inline auto write_indent(OutputStream &stream, const std::size_t columns)
    -> void {
  for (std::size_t index{0}; index < columns; ++index) {
    stream.put(' ');
  }
}

// The width of the "- " indicator that opens a block sequence entry
static constexpr std::size_t SEQUENCE_INDICATOR_WIDTH{2};

inline auto looks_like_number(const std::string &value) -> bool {
  std::size_t start{0};
  if (value[0] == '-' || value[0] == '+') {
    start = 1;
  }

  if (start >= value.size()) {
    return false;
  }

  if (value.size() > start + 1 && value[start] == '0') {
    const char second{value[start + 1]};
    if (second == 'x' || second == 'X' || second == 'o' || second == 'O') {
      return true;
    }
  }

  bool has_digit{false};
  bool has_dot{false};
  bool has_exponent{false};

  for (std::size_t index{start}; index < value.size(); ++index) {
    const char character{value[index]};
    if (character >= '0' && character <= '9') {
      has_digit = true;
    } else if (character == '.' && !has_dot && !has_exponent) {
      has_dot = true;
    } else if ((character == 'e' || character == 'E') && !has_exponent &&
               has_digit) {
      has_exponent = true;
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

inline auto needs_quoting(const std::string &value) -> bool {
  if (value.empty()) {
    return true;
  }

  if (value == "null" || value == "Null" || value == "NULL" || value == "~" ||
      value == "true" || value == "True" || value == "TRUE" ||
      value == "false" || value == "False" || value == "FALSE") {
    return true;
  }

  if (value == ".inf" || value == ".Inf" || value == ".INF" ||
      value == "+.inf" || value == "+.Inf" || value == "+.INF" ||
      value == "-.inf" || value == "-.Inf" || value == "-.INF" ||
      value == ".nan" || value == ".NaN" || value == ".NAN") {
    return true;
  }

  if (value.size() >= 3 &&
      ((value[0] == '-' && value[1] == '-' && value[2] == '-') ||
       (value[0] == '.' && value[1] == '.' && value[2] == '.')) &&
      (value.size() == 3 || value[3] == ' ' || value[3] == '\t')) {
    return true;
  }

  if (looks_like_number(value)) {
    return true;
  }

  const char first{value[0]};

  if (first == ',' || first == '[' || first == ']' || first == '{' ||
      first == '}' || first == '#' || first == '&' || first == '*' ||
      first == '!' || first == '|' || first == '>' || first == '\'' ||
      first == '"' || first == '%' || first == '@' || first == '`') {
    return true;
  }

  if (first == '-' || first == '?' || first == ':') {
    if (value.size() == 1 || value[1] == ' ') {
      return true;
    }
  }

  if (value.front() == ' ' || value.back() == ' ') {
    return true;
  }

  for (std::size_t index{0}; index < value.size(); ++index) {
    const char character{value[index]};
    if (character < ' ') {
      return true;
    }

    if (character == ':' &&
        (index + 1 >= value.size() || value[index + 1] == ' ')) {
      return true;
    }

    if (character == ' ' && index + 1 < value.size() &&
        value[index + 1] == '#') {
      return true;
    }
  }

  return false;
}

inline auto can_single_quote(const std::string &value) -> bool {
  for (const char character : value) {
    if (character < ' ' && character != '\t') {
      return false;
    }
  }

  return true;
}

inline auto write_double_quoted(OutputStream &stream, const std::string &value)
    -> void {
  stream.put('"');
  for (const char character : value) {
    switch (character) {
      case '"':
        stream.write("\\\"", 2);
        break;
      case '\\':
        stream.write("\\\\", 2);
        break;
      case '\n':
        stream.write("\\n", 2);
        break;
      case '\r':
        stream.write("\\r", 2);
        break;
      case '\t':
        stream.write("\\t", 2);
        break;
      case '\0':
        stream.write("\\0", 2);
        break;
      default:
        if (character >= '\x01' && character < '\x20') {
          const auto byte{static_cast<unsigned char>(character)};
          stream.write("\\x", 2);
          stream.put(HEX_DIGITS[byte >> 4U]);
          stream.put(HEX_DIGITS[byte & 0x0FU]);
        } else {
          stream.put(character);
        }
        break;
    }
  }
  stream.put('"');
}

inline auto write_single_quoted(OutputStream &stream, const std::string &value)
    -> void {
  stream.put('\'');
  for (const char character : value) {
    if (character == '\'') {
      stream.write("''", 2);
    } else {
      stream.put(character);
    }
  }
  stream.put('\'');
}

inline auto write_string(OutputStream &stream, const std::string &value)
    -> void {
  if (needs_quoting(value)) {
    write_double_quoted(stream, value);
  } else {
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
  }
}

inline auto write_block_scalar(
    OutputStream &stream, const std::string &value,
    const std::size_t content_columns, const YAMLRoundTrip::ScalarStyle style,
    const YAMLRoundTrip::Chomping chomping,
    const std::optional<std::string> &header_comment = std::nullopt,
    const std::size_t indicator = 0, const bool indent_before_chomping = false,
    const YAMLRoundTrip *roundtrip = nullptr) -> void {
  stream.put(style == YAMLRoundTrip::ScalarStyle::Literal ? '|' : '>');
  if (indent_before_chomping && indicator > 0) {
    stream.put(static_cast<char>('0' + indicator));
  }
  if (chomping == YAMLRoundTrip::Chomping::Strip) {
    stream.put('-');
  } else if (chomping == YAMLRoundTrip::Chomping::Keep) {
    stream.put('+');
  }
  if (!indent_before_chomping && indicator > 0) {
    stream.put(static_cast<char>('0' + indicator));
  }
  if (header_comment.has_value()) {
    stream.put(' ');
    const auto &comment{header_comment.value()};
    stream.write(comment.data(), static_cast<std::streamsize>(comment.size()));
  }
  write_break(stream, roundtrip);

  std::size_t position{0};
  while (position < value.size()) {
    auto line_end{value.find('\n', position)};
    if (line_end == std::string::npos) {
      write_indent(stream, content_columns);
      stream.write(value.data() + position,
                   static_cast<std::streamsize>(value.size() - position));
      write_break(stream, roundtrip);
      break;
    }

    if (line_end > position) {
      write_indent(stream, content_columns);
    }
    stream.write(value.data() + position,
                 static_cast<std::streamsize>(line_end - position));
    write_break(stream, roundtrip);
    position = line_end + 1;
  }
}

// The anchors emitted so far, paired with the value each one names, so that a
// reference can be checked against what it would resolve to when read back
using AnchorValues = std::vector<std::pair<std::string_view, const JSON *>>;

// Numbers of different types compare equal to each other, so the types have to
// agree too before the document can be said to still hold the same value
inline auto same_value(const JSON &left, const JSON &right) -> bool {
  return left.type() == right.type() && left == right;
}

inline auto matches_recorded_value(const YAMLRoundTrip::NodeStyle &style,
                                   const JSON &value) -> bool {
  return style.content_value.has_value() &&
         same_value(style.content_value.value(), value);
}

// The content indentation level of a block scalar that carries no indicator is
// read off its first non-empty line, so detection fails when that line begins
// with a space, and when the content holds no non-empty line at all.
// See https://yaml.org/spec/1.2.2/#8111-block-indentation-indicator
inline auto block_detection_fails(const std::string &value) -> bool {
  std::size_t position{0};
  while (position < value.size()) {
    const auto line_end{value.find('\n', position)};
    const auto length{line_end == std::string::npos ? value.size() - position
                                                    : line_end - position};
    if (length > 0) {
      return value[position] == ' ';
    }

    if (line_end == std::string::npos) {
      break;
    }

    position = line_end + 1;
  }

  return !value.empty();
}

// Every line a block scalar writes is closed with a line break, so the chomping
// indicator is what decides which of the trailing breaks of the value survive
// being read back. See
// https://yaml.org/spec/1.2.2/#8112-block-chomping-indicator
inline auto required_chomping(const std::string &value)
    -> YAMLRoundTrip::Chomping {
  const auto body{value.find_last_not_of('\n')};
  if (body == std::string::npos) {
    return value.empty() ? YAMLRoundTrip::Chomping::Clip
                         : YAMLRoundTrip::Chomping::Keep;
  }

  const auto breaks{value.size() - body - 1};
  if (breaks == 0) {
    return YAMLRoundTrip::Chomping::Strip;
  }

  return breaks == 1 ? YAMLRoundTrip::Chomping::Clip
                     : YAMLRoundTrip::Chomping::Keep;
}

// Clipping and keeping both leave a single trailing line break in place, so a
// recorded indicator that only differs in that way still says the same thing
// and is worth writing back as it was
inline auto block_chomping(const YAMLRoundTrip::NodeStyle &style,
                           const std::string &value)
    -> YAMLRoundTrip::Chomping {
  const auto required{required_chomping(value)};
  if (!style.chomping.has_value() || style.chomping.value() == required) {
    return required;
  }

  return (required == YAMLRoundTrip::Chomping::Clip && !value.empty() &&
          style.chomping.value() == YAMLRoundTrip::Chomping::Keep)
             ? YAMLRoundTrip::Chomping::Keep
             : required;
}

// The folded style joins the lines of its content, so it can only stand for a
// value that has no line break of its own to lose
inline auto folding_preserves(const std::string &value) -> bool {
  const auto body{value.find_last_not_of('\n')};
  return body == std::string::npos || value.find('\n') == std::string::npos ||
         value.find('\n') > body;
}

// The text a block scalar writes, which is the original one for as long as the
// document still holds the value it was read from
inline auto block_scalar_content(const YAMLRoundTrip::NodeStyle &style,
                                 const JSON &value) -> const std::string & {
  return style.block_content.has_value() && matches_recorded_value(style, value)
             ? style.block_content.value()
             : value.to_string();
}

inline auto find_anchor(const AnchorValues &anchors,
                        const std::string_view name) -> const JSON * {
  for (auto iterator{anchors.crbegin()}; iterator != anchors.crend();
       ++iterator) {
    if (iterator->first == name) {
      return iterator->second;
    }
  }

  return nullptr;
}

// A reference stands for whatever its anchor ends up naming, so it may only be
// emitted while the anchor still names the value the document holds here
inline auto matching_alias(const JSON &value, const YAMLRoundTrip *roundtrip,
                           const AnchorValues &anchors, const Pointer &pointer)
    -> const std::string * {
  if (roundtrip == nullptr) {
    return nullptr;
  }

  const auto match{roundtrip->aliases.find(pointer)};
  if (match == roundtrip->aliases.end()) {
    return nullptr;
  }

  const auto *anchored{find_anchor(anchors, match->second)};
  return (anchored != nullptr) && same_value(*anchored, value) ? &match->second
                                                               : nullptr;
}

inline auto write_alias(OutputStream &stream, const std::string &name) -> void {
  stream.put('*');
  stream.write(name.data(), static_cast<std::streamsize>(name.size()));
}

inline auto write_anchor(OutputStream &stream, const std::string &name,
                         const JSON &value, AnchorValues &anchors) -> void {
  stream.put('&');
  stream.write(name.data(), static_cast<std::streamsize>(name.size()));
  anchors.emplace_back(name, &value);
}

// A tag names the kind of value its node holds, so it may only be emitted
// while the document still holds that kind of value
inline auto matching_tag(const YAMLRoundTrip::NodeStyle *style,
                         const JSON &value) -> const std::string * {
  if ((style == nullptr) || !style->tag.has_value() ||
      !style->tag_type.has_value() || style->tag_type.value() != value.type()) {
    return nullptr;
  }

  return &style->tag.value();
}

// Comments and node properties are read off a position in a sequence, so they
// only stand for what is written there while the sequence still holds the same
// items it was read from
inline auto keeps_item_annotations(const YAMLRoundTrip::NodeStyle *style,
                                   const JSON &value) -> bool {
  return (style == nullptr) || !style->sequence_size.has_value() ||
         style->sequence_size.value() == value.size();
}

inline auto has_node_properties(const YAMLRoundTrip::NodeStyle *style,
                                const JSON &value) -> bool {
  return (matching_tag(style, value) != nullptr) ||
         ((style != nullptr) && style->anchor.has_value());
}

// Writes the tag and anchor that decorate a node, in the order they were
// written, and reports whether anything was written at all, as a node property
// has to be separated from the node it decorates
inline auto write_node_properties(OutputStream &stream, const JSON &value,
                                  const YAMLRoundTrip::NodeStyle *style,
                                  AnchorValues &anchors) -> bool {
  const auto *tag{matching_tag(style, value)};
  const bool anchor{(style != nullptr) && style->anchor.has_value()};
  if ((tag == nullptr) && !anchor) {
    return false;
  }

  if ((tag != nullptr) && style->tag_before_anchor) {
    stream.write(tag->data(), static_cast<std::streamsize>(tag->size()));
    if (anchor) {
      stream.put(' ');
    }
  }

  if (anchor) {
    write_anchor(stream, style->anchor.value(), value, anchors);
  }

  if ((tag != nullptr) && !style->tag_before_anchor) {
    if (anchor) {
      stream.put(' ');
    }
    stream.write(tag->data(), static_cast<std::streamsize>(tag->size()));
  }

  return true;
}

inline auto write_string_with_style(OutputStream &stream, const JSON &value,
                                    const YAMLRoundTrip *roundtrip,
                                    const Pointer &pointer) -> void {
  const auto &text{value.to_string()};
  if (roundtrip != nullptr) {
    const auto match{roundtrip->styles.find(pointer)};
    if (match != roundtrip->styles.end() && match->second.scalar.has_value()) {
      if (match->second.quoted_content.has_value() &&
          matches_recorded_value(match->second, value)) {
        const auto &raw{match->second.quoted_content.value()};
        const auto quote_char{match->second.scalar.value() ==
                                      YAMLRoundTrip::ScalarStyle::SingleQuoted
                                  ? '\''
                                  : '"'};
        stream.put(quote_char);
        stream.write(raw.data(), static_cast<std::streamsize>(raw.size()));
        stream.put(quote_char);
        return;
      }
      switch (match->second.scalar.value()) {
        case YAMLRoundTrip::ScalarStyle::SingleQuoted:
          if (can_single_quote(text)) {
            write_single_quoted(stream, text);
            return;
          }
          break;
        case YAMLRoundTrip::ScalarStyle::DoubleQuoted:
          write_double_quoted(stream, text);
          return;
        default:
          break;
      }
    }
  }

  write_string(stream, text);
}

inline auto write_key_string(OutputStream &stream, const std::string &key,
                             const YAMLRoundTrip *roundtrip,
                             const Pointer &pointer) -> void {
  if (roundtrip != nullptr) {
    const auto quoted_match{roundtrip->key_quoted_contents.find(pointer)};
    if (quoted_match != roundtrip->key_quoted_contents.end()) {
      const auto style_match{roundtrip->key_styles.find(pointer)};
      const auto quote_char{style_match != roundtrip->key_styles.end() &&
                                    style_match->second ==
                                        YAMLRoundTrip::ScalarStyle::SingleQuoted
                                ? '\''
                                : '"'};
      stream.put(quote_char);
      const auto &raw{quoted_match->second};
      stream.write(raw.data(), static_cast<std::streamsize>(raw.size()));
      stream.put(quote_char);
      return;
    }
    const auto match{roundtrip->key_styles.find(pointer)};
    if (match != roundtrip->key_styles.end()) {
      switch (match->second) {
        case YAMLRoundTrip::ScalarStyle::Plain:
          stream.write(key.data(), static_cast<std::streamsize>(key.size()));
          return;
        case YAMLRoundTrip::ScalarStyle::SingleQuoted:
          if (can_single_quote(key)) {
            write_single_quoted(stream, key);
            return;
          }
          break;
        case YAMLRoundTrip::ScalarStyle::DoubleQuoted:
          write_double_quoted(stream, key);
          return;
        default:
          break;
      }
    }
  }
  write_string(stream, key);
}

// Forward declarations for recursive flow collection writing
inline auto write_flow_mapping(OutputStream &stream, const JSON &value,
                               const YAMLRoundTrip *roundtrip,
                               AnchorValues &anchors, Pointer &pointer) -> void;
inline auto write_flow_sequence(OutputStream &stream, const JSON &value,
                                const YAMLRoundTrip *roundtrip,
                                AnchorValues &anchors, Pointer &pointer)
    -> void;

inline auto write_inline_value(OutputStream &stream, const JSON &value,
                               const YAMLRoundTrip *roundtrip,
                               AnchorValues &anchors, Pointer &pointer)
    -> void {
  const auto *alias{matching_alias(value, roundtrip, anchors, pointer)};
  if (alias != nullptr) {
    write_alias(stream, *alias);
    return;
  }

  if (roundtrip != nullptr) {
    const auto style_match{roundtrip->styles.find(pointer)};
    if (style_match != roundtrip->styles.end() &&
        style_match->second.scalar.has_value() &&
        style_match->second.scalar.value() ==
            YAMLRoundTrip::ScalarStyle::Plain &&
        style_match->second.plain_content.has_value() &&
        matches_recorded_value(style_match->second, value)) {
      const auto &content{style_match->second.plain_content.value()};
      stream.write(content.data(),
                   static_cast<std::streamsize>(content.size()));
      return;
    }
  }
  switch (value.type()) {
    case JSON::Type::Null:
      stream.write("null", 4);
      break;
    case JSON::Type::Boolean:
      if (value.to_boolean()) {
        stream.write("true", 4);
      } else {
        stream.write("false", 5);
      }
      break;
    case JSON::Type::Integer:
      digits_write(stream, value.to_integer());
      break;
    case JSON::Type::Real: {
      const auto real{value.to_real()};
      if (real == 0.0) {
        stream.write("0.0", 3);
      } else {
        // Format with to_chars so the decimal separator is independent of the
        // global locale and the full round-trip precision is preserved
        std::array<char, 344> buffer{};
        double integer_part;
        if (std::modf(real, &integer_part) == 0.0) {
          const auto result{std::to_chars(buffer.data(),
                                          buffer.data() + buffer.size(), real,
                                          std::chars_format::fixed)};
          assert(result.ec == std::errc{});
          stream.write(buffer.data(), result.ptr - buffer.data());
          stream.write(".0", 2);
        } else {
          const auto result{std::to_chars(buffer.data(),
                                          buffer.data() + buffer.size(), real)};
          assert(result.ec == std::errc{});
          stream.write(buffer.data(), result.ptr - buffer.data());
        }
      }
    } break;
    case JSON::Type::Decimal:
      stream << value.to_decimal().to_scientific_string();
      break;
    case JSON::Type::String:
      write_string_with_style(stream, value, roundtrip, pointer);
      break;
    case JSON::Type::Object:
      if (value.empty()) {
        stream.write("{}", 2);
      } else {
        write_flow_mapping(stream, value, roundtrip, anchors, pointer);
      }
      break;
    case JSON::Type::Array:
      if (value.empty()) {
        stream.write("[]", 2);
      } else {
        write_flow_sequence(stream, value, roundtrip, anchors, pointer);
      }
      break;
  }
}

inline auto is_implicit_null(const JSON &value, const YAMLRoundTrip *roundtrip,
                             const Pointer &pointer) -> bool {
  if ((roundtrip == nullptr) || !value.is_null()) {
    return false;
  }
  if (roundtrip->aliases.contains(pointer)) {
    return false;
  }
  const auto match{roundtrip->styles.find(pointer)};
  if (match == roundtrip->styles.end()) {
    return true;
  }
  return !match->second.scalar.has_value();
}

inline auto write_flow_properties(OutputStream &stream, const JSON &value,
                                  const YAMLRoundTrip *roundtrip,
                                  AnchorValues &anchors, const Pointer &pointer)
    -> void {
  if (roundtrip == nullptr) {
    return;
  }
  const auto match{roundtrip->styles.find(pointer)};
  if (match != roundtrip->styles.end() &&
      write_node_properties(stream, value, &match->second, anchors)) {
    stream.put(' ');
  }
}

// The same as writing the properties of a flow node, but for a node that is
// written with no value at all, so nothing follows to be separated from
inline auto write_flow_node_properties(OutputStream &stream, const JSON &value,
                                       const YAMLRoundTrip *roundtrip,
                                       AnchorValues &anchors,
                                       const Pointer &pointer) -> void {
  if (roundtrip == nullptr) {
    return;
  }

  const auto match{roundtrip->styles.find(pointer)};
  if (match != roundtrip->styles.end()) {
    write_node_properties(stream, value, &match->second, anchors);
  }
}

inline auto write_flow_mapping(OutputStream &stream, const JSON &value,
                               const YAMLRoundTrip *roundtrip,
                               AnchorValues &anchors, Pointer &pointer)
    -> void {
  bool compact{false};
  bool padded{false};
  if (roundtrip != nullptr) {
    const auto match{roundtrip->styles.find(pointer)};
    if (match != roundtrip->styles.end()) {
      compact = match->second.compact_flow;
      padded = match->second.padded_flow;
    }
  }
  stream.put('{');
  if (padded) {
    stream.put(' ');
  }
  bool first{true};
  for (const auto &entry : value.as_object()) {
    if (!first) {
      if (compact) {
        stream.put(',');
      } else {
        stream.write(", ", 2);
      }
    }
    first = false;
    pointer.push_back(entry.first);
    write_key_string(stream, entry.first, roundtrip, pointer);
    stream.write(": ", 2);
    if (is_implicit_null(entry.second, roundtrip, pointer)) {
      write_flow_node_properties(stream, entry.second, roundtrip, anchors,
                                 pointer);
    } else {
      write_flow_properties(stream, entry.second, roundtrip, anchors, pointer);
      write_inline_value(stream, entry.second, roundtrip, anchors, pointer);
    }
    pointer.pop_back();
  }
  if (padded) {
    stream.put(' ');
  }
  stream.put('}');
}

inline auto write_flow_sequence(OutputStream &stream, const JSON &value,
                                const YAMLRoundTrip *roundtrip,
                                AnchorValues &anchors, Pointer &pointer)
    -> void {
  bool compact{false};
  bool padded{false};
  bool annotations{true};
  if (roundtrip != nullptr) {
    const auto match{roundtrip->styles.find(pointer)};
    if (match != roundtrip->styles.end()) {
      compact = match->second.compact_flow;
      padded = match->second.padded_flow;
      annotations = keeps_item_annotations(&match->second, value);
    }
  }
  stream.put('[');
  if (padded) {
    stream.put(' ');
  }
  bool first{true};
  std::size_t item_index{0};
  for (const auto &item : value.as_array()) {
    if (!first) {
      if (compact) {
        stream.put(',');
      } else {
        stream.write(", ", 2);
      }
    }
    first = false;
    pointer.push_back(item_index);
    if (annotations) {
      write_flow_properties(stream, item, roundtrip, anchors, pointer);
    }
    write_inline_value(stream, item, roundtrip, anchors, pointer);
    pointer.pop_back();
    item_index++;
  }
  if (padded) {
    stream.put(' ');
  }
  stream.put(']');
}

inline auto write_block_mapping(OutputStream &stream, const JSON &value,
                                std::size_t columns, std::size_t width,
                                bool skip_first_indent,
                                const YAMLRoundTrip *roundtrip,
                                AnchorValues &anchors, Pointer &pointer)
    -> void;
inline auto write_block_sequence(OutputStream &stream, const JSON &value,
                                 std::size_t columns, std::size_t width,
                                 bool skip_first_indent,
                                 const YAMLRoundTrip *roundtrip,
                                 AnchorValues &anchors, Pointer &pointer)
    -> void;

inline auto emit_inline_comment(OutputStream &stream,
                                const YAMLRoundTrip::NodeStyle *style) -> void {
  if ((style != nullptr) && style->comment_inline.has_value()) {
    stream.put(' ');
    const auto &comment{style->comment_inline.value()};
    stream.write(comment.data(), static_cast<std::streamsize>(comment.size()));
  }
}

inline auto write_node(OutputStream &stream, const JSON &value,
                       const std::size_t columns, const std::size_t width,
                       const std::size_t block_indicator,
                       const bool skip_first_indent,
                       const YAMLRoundTrip *roundtrip, AnchorValues &anchors,
                       Pointer &pointer, const bool skip_properties = false,
                       const bool annotations = true) -> void {
  // Only the document root sits at the leftmost column, and a block scalar
  // there has no column of its own, so its content is pushed one nesting level
  // in to leave room for whatever follows it
  const auto block_columns{columns == 0 ? width : columns};
  const YAMLRoundTrip::NodeStyle *node_style{nullptr};
  if (roundtrip != nullptr) {
    const auto style_match{roundtrip->styles.find(pointer)};
    if (style_match != roundtrip->styles.end()) {
      node_style = &style_match->second;
    }
  }

  const YAMLRoundTrip::NodeStyle *annotation_style{annotations ? node_style
                                                               : nullptr};

  const auto *alias{matching_alias(value, roundtrip, anchors, pointer)};
  if (alias != nullptr) {
    write_alias(stream, *alias);
    emit_inline_comment(stream, annotation_style);
    write_break(stream, roundtrip);
    return;
  }

  const bool has_properties{
      skip_properties
          ? false
          : write_node_properties(stream, value, annotation_style, anchors)};

  const bool flow{
      (node_style != nullptr) && node_style->collection.has_value() &&
      node_style->collection.value() == YAMLRoundTrip::CollectionStyle::Flow};

  // An indicator is a single digit, so content that needs one but sits too far
  // in has to give up on block style and be quoted instead
  const bool block_style{
      (node_style != nullptr) && value.is_string() &&
      node_style->scalar.has_value() &&
      (node_style->scalar.value() == YAMLRoundTrip::ScalarStyle::Literal ||
       node_style->scalar.value() == YAMLRoundTrip::ScalarStyle::Folded) &&
      ((block_indicator >= 1 && block_indicator <= 9) ||
       !block_detection_fails(block_scalar_content(*node_style, value)))};

  if (value.is_object() && !value.empty()) {
    if (flow) {
      if (has_properties) {
        stream.put(' ');
      }
      write_flow_mapping(stream, value, roundtrip, anchors, pointer);
      emit_inline_comment(stream, annotation_style);
      write_break(stream, roundtrip);
    } else {
      if (has_properties) {
        emit_inline_comment(stream, annotation_style);
        write_break(stream, roundtrip);
      }
      write_block_mapping(stream, value, columns, width,
                          has_properties ? false : skip_first_indent, roundtrip,
                          anchors, pointer);
    }
  } else if (value.is_array() && !value.empty()) {
    if (flow) {
      if (has_properties) {
        stream.put(' ');
      }
      write_flow_sequence(stream, value, roundtrip, anchors, pointer);
      emit_inline_comment(stream, annotation_style);
      write_break(stream, roundtrip);
    } else {
      if (has_properties) {
        emit_inline_comment(stream, annotation_style);
        write_break(stream, roundtrip);
      }
      // A block sequence may sit at the indentation of the mapping key it
      // belongs to rather than one level further in
      const auto sequence_columns{(node_style != nullptr) &&
                                          node_style->unindented_sequence &&
                                          columns >= block_indicator
                                      ? columns - block_indicator
                                      : columns};
      write_block_sequence(stream, value, sequence_columns, width,
                           has_properties ? false : skip_first_indent,
                           roundtrip, anchors, pointer);
    }
  } else if (block_style) {
    if (has_properties) {
      stream.put(' ');
    }
    const auto &content{block_scalar_content(*node_style, value)};
    const auto &text{value.to_string()};
    // The recorded style only reproduces the text it was read from, so once the
    // document holds something else, a style that would lose a line break in
    // the process gives way to one that keeps every line as it is
    const bool original{node_style->block_content.has_value() &&
                        matches_recorded_value(*node_style, value)};
    const auto style{original || folding_preserves(text)
                         ? node_style->scalar.value()
                         : YAMLRoundTrip::ScalarStyle::Literal};
    const std::optional<std::string> header_comment{
        (annotation_style != nullptr) ? annotation_style->comment_inline
                                      : std::nullopt};
    const bool indicated{block_detection_fails(content) ||
                         node_style->explicit_indent > 0};
    write_block_scalar(stream, content, block_columns, style,
                       block_chomping(*node_style, text), header_comment,
                       indicated ? block_indicator : 0,
                       node_style->indent_before_chomping, roundtrip);
  } else {
    if (has_properties) {
      stream.put(' ');
    }
    write_inline_value(stream, value, roundtrip, anchors, pointer);
    emit_inline_comment(stream, annotation_style);
    write_break(stream, roundtrip);
  }
}

inline auto write_block_mapping(OutputStream &stream, const JSON &value,
                                const std::size_t columns,
                                const std::size_t width,
                                const bool skip_first_indent,
                                const YAMLRoundTrip *roundtrip,
                                AnchorValues &anchors, Pointer &pointer)
    -> void {
  assert(value.is_object() && !value.empty());
  bool first{true};
  for (const auto &entry : value.as_object()) {
    pointer.push_back(entry.first);

    const YAMLRoundTrip::NodeStyle *entry_style{nullptr};
    if (roundtrip != nullptr) {
      const auto style_match{roundtrip->styles.find(pointer)};
      if (style_match != roundtrip->styles.end()) {
        entry_style = &style_match->second;
      }
    }

    const bool entry_is_alias{
        matching_alias(entry.second, roundtrip, anchors, pointer) != nullptr};

    if (!first || !skip_first_indent) {
      if ((entry_style != nullptr) && !entry_style->comments_before.empty()) {
        for (const auto &comment : entry_style->comments_before) {
          if (comment.empty()) {
            write_break(stream, roundtrip);
          } else {
            write_indent(stream, columns);
            stream.write(comment.data(),
                         static_cast<std::streamsize>(comment.size()));
            write_break(stream, roundtrip);
          }
        }
      }
      write_indent(stream, columns);
    }
    first = false;

    write_key_string(stream, entry.first, roundtrip, pointer);
    stream.put(':');

    const bool implicit_null{
        (roundtrip != nullptr) && entry.second.is_null() && !entry_is_alias &&
        ((entry_style == nullptr) || !entry_style->scalar.has_value())};
    if (implicit_null) {
      if (has_node_properties(entry_style, entry.second)) {
        stream.put(' ');
        write_node_properties(stream, entry.second, entry_style, anchors);
      }
      emit_inline_comment(stream, entry_style);
      write_break(stream, roundtrip);
    } else {
      bool has_indicator_comment{false};
      if ((entry_style != nullptr) &&
          entry_style->comment_on_indicator.has_value()) {
        has_indicator_comment = true;
        stream.put(' ');
        const auto &comment{entry_style->comment_on_indicator.value()};
        stream.write(comment.data(),
                     static_cast<std::streamsize>(comment.size()));
        write_break(stream, roundtrip);
        write_indent(stream, columns + width);
      }
      if (!has_indicator_comment) {
        const bool has_prefix{entry_is_alias ||
                              has_node_properties(entry_style, entry.second)};
        const bool entry_flow{(entry_style != nullptr) &&
                              entry_style->collection.has_value() &&
                              entry_style->collection.value() ==
                                  YAMLRoundTrip::CollectionStyle::Flow};
        const bool nested{
            (entry.second.is_object() || entry.second.is_array()) &&
            !entry.second.empty() && !entry_flow && !has_prefix};
        if (nested) {
          emit_inline_comment(stream, entry_style);
          write_break(stream, roundtrip);
        } else {
          stream.put(' ');
        }
      }
      write_node(stream, entry.second, columns + width, width, width,
                 has_indicator_comment ? true : false, roundtrip, anchors,
                 pointer);
    }

    pointer.pop_back();
  }
}

inline auto write_block_sequence(OutputStream &stream, const JSON &value,
                                 const std::size_t columns,
                                 const std::size_t width,
                                 const bool skip_first_indent,
                                 const YAMLRoundTrip *roundtrip,
                                 AnchorValues &anchors, Pointer &pointer)
    -> void {
  assert(value.is_array() && !value.empty());
  const YAMLRoundTrip::NodeStyle *sequence_style{nullptr};
  if (roundtrip != nullptr) {
    const auto style_match{roundtrip->styles.find(pointer)};
    if (style_match != roundtrip->styles.end()) {
      sequence_style = &style_match->second;
    }
  }

  const bool annotations{keeps_item_annotations(sequence_style, value)};
  bool first{true};
  std::size_t item_index{0};
  for (const auto &item : value.as_array()) {
    pointer.push_back(item_index);

    const YAMLRoundTrip::NodeStyle *item_style{nullptr};
    if (annotations && (roundtrip != nullptr)) {
      const auto style_match{roundtrip->styles.find(pointer)};
      if (style_match != roundtrip->styles.end()) {
        item_style = &style_match->second;
      }
    }

    const bool item_is_alias{
        matching_alias(item, roundtrip, anchors, pointer) != nullptr};

    if (!first || !skip_first_indent) {
      if ((item_style != nullptr) && !item_style->comments_before.empty()) {
        for (const auto &comment : item_style->comments_before) {
          if (comment.empty()) {
            write_break(stream, roundtrip);
          } else {
            write_indent(stream, columns);
            stream.write(comment.data(),
                         static_cast<std::streamsize>(comment.size()));
            write_break(stream, roundtrip);
          }
        }
      }
      write_indent(stream, columns);
    }
    first = false;

    const bool implicit_null{
        (roundtrip != nullptr) && item.is_null() && !item_is_alias &&
        ((item_style == nullptr) || !item_style->scalar.has_value())};
    if (implicit_null) {
      stream.put('-');
      if (item_style != nullptr) {
        if (has_node_properties(item_style, item)) {
          stream.put(' ');
          write_node_properties(stream, item, item_style, anchors);
        }
        if (item_style->comment_on_indicator.has_value() &&
            !item_style->comment_on_indicator.value().empty()) {
          stream.put(' ');
          const auto &comment{item_style->comment_on_indicator.value()};
          stream.write(comment.data(),
                       static_cast<std::streamsize>(comment.size()));
        }
      }
      emit_inline_comment(stream, item_style);
      write_break(stream, roundtrip);
    } else {
      bool has_indicator{false};
      if ((item_style != nullptr) &&
          item_style->comment_on_indicator.has_value()) {
        has_indicator = true;
        const auto &comment{item_style->comment_on_indicator.value()};
        if (comment.empty()) {
          stream.put('-');
        } else {
          stream.write("- ", 2);
          stream.write(comment.data(),
                       static_cast<std::streamsize>(comment.size()));
        }
        write_break(stream, roundtrip);
        write_indent(stream, columns + SEQUENCE_INDICATOR_WIDTH);
      }
      if (!has_indicator) {
        stream.write("- ", 2);
      }
      write_node(stream, item, columns + SEQUENCE_INDICATOR_WIDTH, width,
                 SEQUENCE_INDICATOR_WIDTH, true, roundtrip, anchors, pointer,
                 false, annotations);
    }

    pointer.pop_back();
    item_index++;
  }
}

// An anchor only names something when an alias that refers to it is written
// later, as an alias may not stand before the anchor it names. A caller that
// rearranges the document can move an anchor past its aliases, which expands
// them and leaves the anchor naming nothing.
// See https://yaml.org/spec/1.2.2/#71-alias-nodes
struct AnchorDefinition {
  Pointer pointer;
  std::string name;
  std::size_t position;
};

inline auto scan_anchor_uses(
    const JSON &value, const YAMLRoundTrip &roundtrip, Pointer &pointer,
    std::size_t &position, std::vector<AnchorDefinition> &definitions,
    std::unordered_map<std::string, std::size_t> &aliases) -> void {
  const auto alias{roundtrip.aliases.find(pointer)};
  if (alias != roundtrip.aliases.cend()) {
    aliases[alias->second] = position;
  } else {
    const auto style{roundtrip.styles.find(pointer)};
    if (style != roundtrip.styles.cend() && style->second.anchor.has_value()) {
      definitions.emplace_back(pointer, style->second.anchor.value(), position);
    }
  }

  position += 1;

  if (value.is_object()) {
    for (const auto &entry : value.as_object()) {
      pointer.push_back(entry.first);
      scan_anchor_uses(entry.second, roundtrip, pointer, position, definitions,
                       aliases);
      pointer.pop_back();
    }
  } else if (value.is_array()) {
    std::size_t index{0};
    for (const auto &item : value.as_array()) {
      pointer.push_back(index);
      scan_anchor_uses(item, roundtrip, pointer, position, definitions,
                       aliases);
      pointer.pop_back();
      index += 1;
    }
  }
}

// An anchor that never had an alias is markup the document was written with, so
// only one whose aliases have all moved ahead of it is dropped
inline auto collect_dead_anchors(const JSON &document,
                                 const YAMLRoundTrip &roundtrip)
    -> std::vector<Pointer> {
  std::vector<Pointer> dead;
  if (roundtrip.aliases.empty()) {
    return dead;
  }

  Pointer pointer;
  std::size_t position{0};
  std::vector<AnchorDefinition> definitions;
  std::unordered_map<std::string, std::size_t> aliases;
  scan_anchor_uses(document, roundtrip, pointer, position, definitions,
                   aliases);

  for (const auto &definition : definitions) {
    const auto match{aliases.find(definition.name)};
    if (match != aliases.cend() && match->second < definition.position) {
      dead.push_back(definition.pointer);
    }
  }

  return dead;
}

template <template <typename T> typename Allocator>
auto stringify_yaml(const JSON &document, OutputStream &stream,
                    const YAMLRoundTrip *roundtrip = nullptr,
                    const std::size_t indentation = INDENT_WIDTH) -> void {
  std::optional<YAMLRoundTrip> pruned;
  if (roundtrip != nullptr) {
    const auto dead{collect_dead_anchors(document, *roundtrip)};
    if (!dead.empty()) {
      pruned = *roundtrip;
      for (const auto &entry : dead) {
        pruned.value().styles[entry].anchor.reset();
      }

      roundtrip = &pruned.value();
    }
  }

  if ((roundtrip != nullptr) && roundtrip->byte_order_mark) {
    stream.write("\xEF\xBB\xBF", 3);
  }

  Pointer pointer;
  AnchorValues anchors;
  const YAMLRoundTrip::NodeStyle *root_style{nullptr};
  if (roundtrip != nullptr) {
    const auto style_match{roundtrip->styles.find(pointer)};
    if (style_match != roundtrip->styles.end()) {
      root_style = &style_match->second;
    }
  }

  // A node property on the root node belongs on the document start marker,
  // which is the only line that precedes the node itself
  bool root_properties{false};

  bool directed{false};
  if (roundtrip != nullptr) {
    for (const auto &line : roundtrip->document_prefix) {
      directed = directed || line.starts_with('%');
      stream.write(line.data(), static_cast<std::streamsize>(line.size()));
      write_break(stream, roundtrip);
    }

    for (const auto &comment : roundtrip->leading_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      write_break(stream, roundtrip);
    }
  }

  // A document that carries directives is closed off from them by an explicit
  // start marker. See https://yaml.org/spec/1.2.2/#912-document-markers
  if (roundtrip && (roundtrip->explicit_document_start || directed)) {
    stream.write("---", 3);
    if (has_node_properties(root_style, document)) {
      stream.put(' ');
      root_properties =
          write_node_properties(stream, document, root_style, anchors);
    }
    if (roundtrip->document_start_comment.has_value()) {
      stream.put(' ');
      const auto &comment{roundtrip->document_start_comment.value()};
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
    }
    write_break(stream, roundtrip);
    for (const auto &comment : roundtrip->post_start_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      write_break(stream, roundtrip);
    }
  }

  if (!is_implicit_null(document, roundtrip, pointer)) {
    // A nesting width of zero would run a nested collection into the one that
    // holds it, so the narrowest width that still nests is used instead
    const auto width{std::max(indentation, ONE_COLUMN)};
    write_node(stream, document, 0, width, width + 1, false, roundtrip, anchors,
               pointer, root_properties);
  }

  if (roundtrip) {
    for (const auto &comment : roundtrip->pre_end_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      write_break(stream, roundtrip);
    }
  }

  if (roundtrip && roundtrip->explicit_document_end) {
    stream.write("...", 3);
    if (roundtrip->document_end_comment.has_value()) {
      stream.put(' ');
      const auto &comment{roundtrip->document_end_comment.value()};
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
    }
    write_break(stream, roundtrip);
  }

  if (roundtrip) {
    for (const auto &comment : roundtrip->trailing_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      write_break(stream, roundtrip);
    }
  }
}

} // namespace sourcemeta::core::yaml

#endif
