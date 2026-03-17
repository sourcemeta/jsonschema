#ifndef SOURCEMETA_CORE_YAML_STRINGIFY_H_
#define SOURCEMETA_CORE_YAML_STRINGIFY_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/yaml_roundtrip.h>

#include <array>   // std::array
#include <cassert> // assert
#include <cmath>   // std::modf
#include <cstddef> // std::size_t
#include <iomanip> // std::setprecision
#include <ios>     // std::noshowpoint, std::fixed
#include <ostream> // std::basic_ostream
#include <string>  // std::to_string

namespace sourcemeta::core::yaml {

using OutputStream = std::basic_ostream<JSON::Char, JSON::CharTraits>;

static constexpr std::size_t INDENT_WIDTH{2};
static constexpr std::array<char, 16> HEX_DIGITS{{'0', '1', '2', '3', '4', '5',
                                                  '6', '7', '8', '9', 'a', 'b',
                                                  'c', 'd', 'e', 'f'}};

inline auto write_indent(OutputStream &stream, const std::size_t indent,
                         const std::size_t width = INDENT_WIDTH) -> void {
  for (std::size_t index{0}; index < indent * width; ++index) {
    stream.put(' ');
  }
}

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
          stream.put(HEX_DIGITS[byte >> 4u]);
          stream.put(HEX_DIGITS[byte & 0x0Fu]);
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
    OutputStream &stream, const std::string &value, const std::size_t indent,
    const YAMLRoundTrip::ScalarStyle style,
    const YAMLRoundTrip::Chomping chomping,
    const std::optional<std::string> &header_comment = std::nullopt,
    const std::size_t indent_width = INDENT_WIDTH,
    const std::size_t explicit_indent = 0,
    const bool indent_before_chomping = false) -> void {
  stream.put(style == YAMLRoundTrip::ScalarStyle::Literal ? '|' : '>');
  if (indent_before_chomping && explicit_indent > 0) {
    stream.put(static_cast<char>('0' + explicit_indent));
  }
  if (chomping == YAMLRoundTrip::Chomping::Strip) {
    stream.put('-');
  } else if (chomping == YAMLRoundTrip::Chomping::Keep) {
    stream.put('+');
  }
  if (!indent_before_chomping && explicit_indent > 0) {
    stream.put(static_cast<char>('0' + explicit_indent));
  }
  if (header_comment.has_value()) {
    stream.put(' ');
    const auto &comment{header_comment.value()};
    stream.write(comment.data(), static_cast<std::streamsize>(comment.size()));
  }
  stream.put('\n');

  std::size_t position{0};
  while (position < value.size()) {
    auto line_end{value.find('\n', position)};
    if (line_end == std::string::npos) {
      write_indent(stream, indent, indent_width);
      stream.write(value.data() + position,
                   static_cast<std::streamsize>(value.size() - position));
      stream.put('\n');
      break;
    }

    if (line_end > position) {
      write_indent(stream, indent, indent_width);
    }
    stream.write(value.data() + position,
                 static_cast<std::streamsize>(line_end - position));
    stream.put('\n');
    position = line_end + 1;
  }
}

inline auto write_string_with_style(OutputStream &stream,
                                    const std::string &value,
                                    const YAMLRoundTrip *roundtrip,
                                    const Pointer &pointer) -> void {
  if (roundtrip) {
    const auto match{roundtrip->styles.find(pointer)};
    if (match != roundtrip->styles.end() && match->second.scalar.has_value()) {
      if (match->second.quoted_content.has_value()) {
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
          if (can_single_quote(value)) {
            write_single_quoted(stream, value);
            return;
          }
          break;
        case YAMLRoundTrip::ScalarStyle::DoubleQuoted:
          write_double_quoted(stream, value);
          return;
        default:
          break;
      }
    }
  }

  write_string(stream, value);
}

inline auto write_key_string(OutputStream &stream, const std::string &key,
                             const YAMLRoundTrip *roundtrip,
                             const Pointer &pointer) -> void {
  if (roundtrip) {
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
                               const YAMLRoundTrip *roundtrip, Pointer &pointer)
    -> void;
inline auto write_flow_sequence(OutputStream &stream, const JSON &value,
                                const YAMLRoundTrip *roundtrip,
                                Pointer &pointer) -> void;

inline auto write_inline_value(OutputStream &stream, const JSON &value,
                               const YAMLRoundTrip *roundtrip, Pointer &pointer)
    -> void {
  if (roundtrip) {
    const auto alias_match{roundtrip->aliases.find(pointer)};
    if (alias_match != roundtrip->aliases.end()) {
      stream.put('*');
      const auto &name{alias_match->second};
      stream.write(name.data(), static_cast<std::streamsize>(name.size()));
      return;
    }

    const auto style_match{roundtrip->styles.find(pointer)};
    if (style_match != roundtrip->styles.end() &&
        style_match->second.scalar.has_value() &&
        style_match->second.scalar.value() ==
            YAMLRoundTrip::ScalarStyle::Plain &&
        style_match->second.plain_content.has_value()) {
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
    case JSON::Type::Integer: {
      const auto string{std::to_string(value.to_integer())};
      stream.write(string.c_str(), static_cast<std::streamsize>(string.size()));
    } break;
    case JSON::Type::Real: {
      const auto real{value.to_real()};
      if (real == 0.0) {
        stream.write("0.0", 3);
      } else {
        const auto flags{stream.flags()};
        const auto precision{stream.precision()};
        double integer_part;
        if (std::modf(real, &integer_part) == 0.0) {
          stream << std::fixed << std::setprecision(1) << real;
        } else {
          stream << std::noshowpoint << real;
        }
        stream.flags(flags);
        stream.precision(precision);
      }
    } break;
    case JSON::Type::Decimal:
      stream << value.to_decimal().to_scientific_string();
      break;
    case JSON::Type::String:
      write_string_with_style(stream, value.to_string(), roundtrip, pointer);
      break;
    case JSON::Type::Object:
      if (value.empty()) {
        stream.write("{}", 2);
      } else {
        write_flow_mapping(stream, value, roundtrip, pointer);
      }
      break;
    case JSON::Type::Array:
      if (value.empty()) {
        stream.write("[]", 2);
      } else {
        write_flow_sequence(stream, value, roundtrip, pointer);
      }
      break;
  }
}

inline auto is_implicit_null(const JSON &value, const YAMLRoundTrip *roundtrip,
                             const Pointer &pointer) -> bool {
  if (!roundtrip || !value.is_null()) {
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

inline auto write_flow_anchor(OutputStream &stream,
                              const YAMLRoundTrip *roundtrip,
                              const Pointer &pointer) -> void {
  if (!roundtrip) {
    return;
  }
  const auto match{roundtrip->styles.find(pointer)};
  if (match != roundtrip->styles.end() && match->second.anchor.has_value()) {
    stream.put('&');
    const auto &anchor_name{match->second.anchor.value()};
    stream.write(anchor_name.data(),
                 static_cast<std::streamsize>(anchor_name.size()));
    stream.put(' ');
  }
}

inline auto write_flow_mapping(OutputStream &stream, const JSON &value,
                               const YAMLRoundTrip *roundtrip, Pointer &pointer)
    -> void {
  bool compact{false};
  if (roundtrip) {
    const auto match{roundtrip->styles.find(pointer)};
    if (match != roundtrip->styles.end()) {
      compact = match->second.compact_flow;
    }
  }
  stream.put('{');
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
    if (!is_implicit_null(entry.second, roundtrip, pointer)) {
      write_flow_anchor(stream, roundtrip, pointer);
      write_inline_value(stream, entry.second, roundtrip, pointer);
    }
    pointer.pop_back();
  }
  stream.put('}');
}

inline auto write_flow_sequence(OutputStream &stream, const JSON &value,
                                const YAMLRoundTrip *roundtrip,
                                Pointer &pointer) -> void {
  bool compact{false};
  if (roundtrip) {
    const auto match{roundtrip->styles.find(pointer)};
    if (match != roundtrip->styles.end()) {
      compact = match->second.compact_flow;
    }
  }
  stream.put('[');
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
    write_flow_anchor(stream, roundtrip, pointer);
    write_inline_value(stream, item, roundtrip, pointer);
    pointer.pop_back();
    item_index++;
  }
  stream.put(']');
}

inline auto write_block_mapping(OutputStream &stream, const JSON &value,
                                std::size_t indent, bool skip_first_indent,
                                const YAMLRoundTrip *roundtrip,
                                Pointer &pointer) -> void;
inline auto write_block_sequence(OutputStream &stream, const JSON &value,
                                 std::size_t indent, bool skip_first_indent,
                                 const YAMLRoundTrip *roundtrip,
                                 Pointer &pointer) -> void;

inline auto emit_inline_comment(OutputStream &stream,
                                const YAMLRoundTrip::NodeStyle *style) -> void {
  if (style && style->comment_inline.has_value()) {
    stream.put(' ');
    const auto &comment{style->comment_inline.value()};
    stream.write(comment.data(), static_cast<std::streamsize>(comment.size()));
  }
}

inline auto write_node(OutputStream &stream, const JSON &value,
                       const std::size_t indent, const bool skip_first_indent,
                       const YAMLRoundTrip *roundtrip, Pointer &pointer)
    -> void {
  const YAMLRoundTrip::NodeStyle *node_style{nullptr};
  if (roundtrip) {
    const auto style_match{roundtrip->styles.find(pointer)};
    if (style_match != roundtrip->styles.end()) {
      node_style = &style_match->second;
    }
    const auto alias_match{roundtrip->aliases.find(pointer)};
    if (alias_match != roundtrip->aliases.end()) {
      stream.put('*');
      const auto &name{alias_match->second};
      stream.write(name.data(), static_cast<std::streamsize>(name.size()));
      emit_inline_comment(stream, node_style);
      stream.put('\n');
      return;
    }
  }

  bool has_anchor{false};
  if (node_style && node_style->anchor.has_value()) {
    stream.put('&');
    const auto &name{node_style->anchor.value()};
    stream.write(name.data(), static_cast<std::streamsize>(name.size()));
    has_anchor = true;
  }

  const bool flow{node_style && node_style->collection.has_value() &&
                  node_style->collection.value() ==
                      YAMLRoundTrip::CollectionStyle::Flow};

  if (value.is_object() && !value.empty()) {
    if (flow) {
      if (has_anchor) {
        stream.put(' ');
      }
      write_flow_mapping(stream, value, roundtrip, pointer);
      emit_inline_comment(stream, node_style);
      stream.put('\n');
    } else {
      if (has_anchor) {
        emit_inline_comment(stream, node_style);
        stream.put('\n');
      }
      write_block_mapping(stream, value, indent,
                          has_anchor ? false : skip_first_indent, roundtrip,
                          pointer);
    }
  } else if (value.is_array() && !value.empty()) {
    if (flow) {
      if (has_anchor) {
        stream.put(' ');
      }
      write_flow_sequence(stream, value, roundtrip, pointer);
      emit_inline_comment(stream, node_style);
      stream.put('\n');
    } else {
      if (has_anchor) {
        emit_inline_comment(stream, node_style);
        stream.put('\n');
      }
      write_block_sequence(stream, value, indent,
                           has_anchor ? false : skip_first_indent, roundtrip,
                           pointer);
    }
  } else if (node_style && value.is_string() &&
             node_style->scalar.has_value() &&
             (node_style->scalar.value() ==
                  YAMLRoundTrip::ScalarStyle::Literal ||
              node_style->scalar.value() ==
                  YAMLRoundTrip::ScalarStyle::Folded)) {
    if (has_anchor) {
      stream.put(' ');
    }
    const auto chomping{
        node_style->chomping.value_or(YAMLRoundTrip::Chomping::Clip)};
    const auto &content{node_style->block_content.has_value()
                            ? node_style->block_content.value()
                            : value.to_string()};
    write_block_scalar(stream, content, indent, node_style->scalar.value(),
                       chomping, node_style->comment_inline,
                       roundtrip->indent_width, node_style->explicit_indent,
                       node_style->indent_before_chomping);
  } else {
    if (has_anchor) {
      stream.put(' ');
    }
    write_inline_value(stream, value, roundtrip, pointer);
    emit_inline_comment(stream, node_style);
    stream.put('\n');
  }
}

inline auto write_block_mapping(OutputStream &stream, const JSON &value,
                                const std::size_t indent,
                                const bool skip_first_indent,
                                const YAMLRoundTrip *roundtrip,
                                Pointer &pointer) -> void {
  assert(value.is_object() && !value.empty());
  const auto width{roundtrip ? roundtrip->indent_width : INDENT_WIDTH};
  bool first{true};
  for (const auto &entry : value.as_object()) {
    pointer.push_back(entry.first);

    const YAMLRoundTrip::NodeStyle *entry_style{nullptr};
    bool entry_is_alias{false};
    if (roundtrip) {
      entry_is_alias = roundtrip->aliases.contains(pointer);
      const auto style_match{roundtrip->styles.find(pointer)};
      if (style_match != roundtrip->styles.end()) {
        entry_style = &style_match->second;
      }
    }

    if (!first || !skip_first_indent) {
      if (entry_style && !entry_style->comments_before.empty()) {
        for (const auto &comment : entry_style->comments_before) {
          if (comment.empty()) {
            stream.put('\n');
          } else {
            write_indent(stream, indent, width);
            stream.write(comment.data(),
                         static_cast<std::streamsize>(comment.size()));
            stream.put('\n');
          }
        }
      }
      write_indent(stream, indent, width);
    }
    first = false;

    write_key_string(stream, entry.first, roundtrip, pointer);
    stream.put(':');

    const bool implicit_null{
        roundtrip && entry.second.is_null() && !entry_is_alias &&
        (!entry_style || !entry_style->scalar.has_value())};
    if (implicit_null) {
      if (entry_style && entry_style->anchor.has_value()) {
        stream.put(' ');
        stream.put('&');
        const auto &name{entry_style->anchor.value()};
        stream.write(name.data(), static_cast<std::streamsize>(name.size()));
      }
      emit_inline_comment(stream, entry_style);
      stream.put('\n');
    } else {
      bool has_indicator_comment{false};
      if (entry_style && entry_style->comment_on_indicator.has_value()) {
        has_indicator_comment = true;
        stream.put(' ');
        const auto &comment{entry_style->comment_on_indicator.value()};
        stream.write(comment.data(),
                     static_cast<std::streamsize>(comment.size()));
        stream.put('\n');
        write_indent(stream, indent + 1, width);
      }
      if (!has_indicator_comment) {
        const bool has_prefix{entry_is_alias ||
                              (entry_style && entry_style->anchor.has_value())};
        const bool entry_flow{entry_style &&
                              entry_style->collection.has_value() &&
                              entry_style->collection.value() ==
                                  YAMLRoundTrip::CollectionStyle::Flow};
        const bool nested{
            (entry.second.is_object() || entry.second.is_array()) &&
            !entry.second.empty() && !entry_flow && !has_prefix};
        if (nested) {
          emit_inline_comment(stream, entry_style);
          stream.put('\n');
        } else {
          stream.put(' ');
        }
      }
      write_node(stream, entry.second, indent + 1,
                 has_indicator_comment ? true : false, roundtrip, pointer);
    }

    pointer.pop_back();
  }
}

inline auto write_block_sequence(OutputStream &stream, const JSON &value,
                                 const std::size_t indent,
                                 const bool skip_first_indent,
                                 const YAMLRoundTrip *roundtrip,
                                 Pointer &pointer) -> void {
  assert(value.is_array() && !value.empty());
  const auto width{roundtrip ? roundtrip->indent_width : INDENT_WIDTH};
  bool first{true};
  std::size_t item_index{0};
  for (const auto &item : value.as_array()) {
    pointer.push_back(item_index);

    // Single lookup for alias and style per item
    const YAMLRoundTrip::NodeStyle *item_style{nullptr};
    bool item_is_alias{false};
    if (roundtrip) {
      item_is_alias = roundtrip->aliases.contains(pointer);
      const auto style_match{roundtrip->styles.find(pointer)};
      if (style_match != roundtrip->styles.end()) {
        item_style = &style_match->second;
      }
    }

    if (!first || !skip_first_indent) {
      if (item_style && !item_style->comments_before.empty()) {
        for (const auto &comment : item_style->comments_before) {
          if (comment.empty()) {
            stream.put('\n');
          } else {
            write_indent(stream, indent, width);
            stream.write(comment.data(),
                         static_cast<std::streamsize>(comment.size()));
            stream.put('\n');
          }
        }
      }
      write_indent(stream, indent, width);
    }
    first = false;

    const bool implicit_null{roundtrip && item.is_null() && !item_is_alias &&
                             (!item_style || !item_style->scalar.has_value())};
    if (implicit_null) {
      stream.put('-');
      if (item_style) {
        if (item_style->anchor.has_value()) {
          stream.put(' ');
          stream.put('&');
          const auto &name{item_style->anchor.value()};
          stream.write(name.data(), static_cast<std::streamsize>(name.size()));
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
      stream.put('\n');
    } else {
      bool has_indicator{false};
      if (item_style && item_style->comment_on_indicator.has_value()) {
        has_indicator = true;
        const auto &comment{item_style->comment_on_indicator.value()};
        if (comment.empty()) {
          stream.put('-');
        } else {
          stream.write("- ", 2);
          stream.write(comment.data(),
                       static_cast<std::streamsize>(comment.size()));
        }
        stream.put('\n');
        write_indent(stream, indent + 1, width);
      }
      if (!has_indicator) {
        stream.write("- ", 2);
      }
      write_node(stream, item, indent + 1, true, roundtrip, pointer);
    }

    pointer.pop_back();
    item_index++;
  }
}

template <template <typename T> typename Allocator>
auto stringify_yaml(const JSON &document, OutputStream &stream,
                    const YAMLRoundTrip *roundtrip = nullptr) -> void {
  if (roundtrip) {
    for (const auto &comment : roundtrip->leading_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      stream.put('\n');
    }
  }

  if (roundtrip && roundtrip->explicit_document_start) {
    stream.write("---", 3);
    if (roundtrip->document_start_comment.has_value()) {
      stream.put(' ');
      const auto &comment{roundtrip->document_start_comment.value()};
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
    }
    stream.put('\n');
    for (const auto &comment : roundtrip->post_start_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      stream.put('\n');
    }
  }

  Pointer pointer;
  if (!is_implicit_null(document, roundtrip, pointer)) {
    write_node(stream, document, 0, false, roundtrip, pointer);
  }

  if (roundtrip) {
    for (const auto &comment : roundtrip->pre_end_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      stream.put('\n');
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
    stream.put('\n');
  }

  if (roundtrip) {
    for (const auto &comment : roundtrip->trailing_comments) {
      stream.write(comment.data(),
                   static_cast<std::streamsize>(comment.size()));
      stream.put('\n');
    }
  }
}

} // namespace sourcemeta::core::yaml

#endif
