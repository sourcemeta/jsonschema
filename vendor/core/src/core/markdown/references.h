#ifndef SOURCEMETA_CORE_MARKDOWN_REFERENCES_H_
#define SOURCEMETA_CORE_MARKDOWN_REFERENCES_H_

#include <sourcemeta/core/unicode.h>

#include "characters.h"
#include "document.h"
#include "scanners.h"

#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core::markdown {

// GFM section 6.6: "A link label can have at most 999 characters inside the
// square brackets", where GFM section 2.1 says that "A character is a Unicode
// code point"
constexpr std::size_t MAXIMUM_LINK_LABEL_LENGTH{999};

// GFM section 6.6: "Implementations may impose limits on parentheses nesting to
// avoid performance issues, but at least three levels of nesting should be
// supported"
constexpr std::size_t MAXIMUM_DESTINATION_PARENTHESES{32};

// A link label of GFM section 4.7, moving the position past it on success
inline auto scan_link_label(const std::string_view input, std::size_t &position,
                            std::string_view &label) noexcept -> bool {
  const auto start{position};
  if (character_at(input, start) != '[') {
    return false;
  }

  auto cursor{start + 1};
  std::size_t length{0};
  while (cursor < input.size()) {
    const auto character{input[cursor]};
    if (character == '[' || character == ']') {
      break;
    }

    ++cursor;
    if (!sourcemeta::core::is_utf8_continuation(
            static_cast<unsigned char>(character))) {
      ++length;
    }

    if (character == '\\' &&
        sourcemeta::core::is_punctuation(character_at(input, cursor))) {
      ++cursor;
      ++length;
    }

    if (length > MAXIMUM_LINK_LABEL_LENGTH) {
      return false;
    }
  }

  if (character_at(input, cursor) != ']') {
    return false;
  }

  label = sourcemeta::core::trim(input.substr(start + 1, cursor - start - 1),
                                 is_space);
  position = cursor + 1;
  return true;
}

// A link destination of GFM section 6.3, returning the length it takes
// including any angle brackets, or minus one if there is none
inline auto scan_link_destination(const std::string_view input,
                                  const std::size_t offset,
                                  std::string_view &destination) noexcept
    -> std::ptrdiff_t {
  const auto size{input.size()};
  auto index{offset};
  if (index < size && input[index] == '<') {
    ++index;
    while (index < size) {
      const auto character{input[index]};
      if (character == '>') {
        ++index;
        break;
      }

      // GFM section 6.3: a link destination can be "a sequence of zero or
      // more characters between an opening < and a closing > that contains
      // no line breaks or unescaped < or > characters", where GFM section 2.4
      // only lets "Any ASCII punctuation character" be backslash-escaped
      if (character == '\\' && index + 1 < size &&
          sourcemeta::core::is_punctuation(input[index + 1])) {
        index += 2;
      } else if (character == '\n' || character == '\r' || character == '<') {
        return -1;
      } else {
        ++index;
      }
    }

    if (index >= size) {
      return -1;
    }

    destination = input.substr(offset + 1, index - offset - 2);
    return static_cast<std::ptrdiff_t>(index - offset);
  }

  std::size_t parentheses{0};
  while (index < size) {
    const auto character{input[index]};
    if (character == '\\' && index + 1 < size &&
        sourcemeta::core::is_punctuation(input[index + 1])) {
      index += 2;
    } else if (character == '(') {
      ++parentheses;
      ++index;
      if (parentheses > MAXIMUM_DESTINATION_PARENTHESES) {
        return -1;
      }
    } else if (character == ')') {
      if (parentheses == 0) {
        break;
      }

      --parentheses;
      ++index;
    } else if (is_space_or_control(character)) {
      if (index == offset) {
        return -1;
      }

      break;
    } else {
      ++index;
    }
  }

  if (index >= size) {
    return -1;
  }

  destination = input.substr(offset, index - offset);
  return static_cast<std::ptrdiff_t>(index - offset);
}

// Resolve the character references and backslash escapes of a link
// destination, where a destination that needs neither and lives in stable
// storage is returned as it is
inline auto clean_url(Document &document, const std::string_view url,
                      std::string &buffer, const bool stable)
    -> std::string_view {
  if (url.empty()) {
    return {};
  }

  if (stable && url.find_first_of("&\\") == std::string_view::npos) {
    return url;
  }

  buffer.clear();
  decode_escapes_and_references(buffer, url);
  return document.strings.store(buffer);
}

// Resolve the character references and backslash escapes of a link title,
// dropping its surrounding delimiters
inline auto clean_title(Document &document, const std::string_view title,
                        std::string &buffer, const bool stable)
    -> std::string_view {
  if (title.empty()) {
    return {};
  }

  const auto first{title.front()};
  const auto last{title.back()};
  auto inner{title};
  if ((first == '\'' && last == '\'') || (first == '(' && last == ')') ||
      (first == '"' && last == '"')) {
    inner = title.size() < 2 ? std::string_view{}
                             : title.substr(1, title.size() - 2);
  }

  if (stable && inner.find_first_of("&\\") == std::string_view::npos) {
    return inner;
  }

  buffer.clear();
  decode_escapes_and_references(buffer, inner);
  return document.strings.store(buffer);
}

inline auto skip_spaces(const std::string_view input,
                        std::size_t &position) noexcept -> void {
  while (position < input.size() &&
         (input[position] == ' ' || input[position] == '\t')) {
    ++position;
  }
}

inline auto skip_line_end(const std::string_view input,
                          std::size_t &position) noexcept -> bool {
  bool seen{false};
  if (character_at(input, position) == '\r') {
    ++position;
    seen = true;
  }

  if (character_at(input, position) == '\n') {
    ++position;
    seen = true;
  }

  return seen || position >= input.size();
}

// Spaces or tabs, including up to one line ending
inline auto skip_spaces_and_line_end(const std::string_view input,
                                     std::size_t &position) noexcept -> void {
  skip_spaces(input, position);
  if (skip_line_end(input, position)) {
    skip_spaces(input, position);
  }
}

// A link reference definition of GFM section 4.7 at the start of the input,
// which is registered unless a previous definition has the same label,
// returning the length it takes or zero if there is none
inline auto
parse_reference_definition(Document &document, const std::string_view input,
                           std::string &label_buffer, std::string &value_buffer)
    -> std::size_t {
  std::size_t position{0};
  std::string_view label;
  if (!scan_link_label(input, position, label) || label.empty() ||
      character_at(input, position) != ':') {
    return 0;
  }

  ++position;
  skip_spaces_and_line_end(input, position);
  std::string_view url;
  const auto url_length{scan_link_destination(input, position, url)};
  if (url_length < 0) {
    return 0;
  }

  position += static_cast<std::size_t>(url_length);
  const auto before_title{position};
  skip_spaces_and_line_end(input, position);
  const auto title_length{
      position == before_title ? 0 : scan_link_title(input, position)};
  std::string_view title;
  if (title_length > 0) {
    title = input.substr(position, title_length);
    position += title_length;
  } else {
    position = before_title;
  }

  skip_spaces(input, position);
  if (!skip_line_end(input, position)) {
    if (title_length == 0) {
      return 0;
    }

    position = before_title;
    skip_spaces(input, position);
    if (!skip_line_end(input, position)) {
      return 0;
    }

    // GFM section 4.7, example 179: a title followed by other text on its line
    // leaves "a link reference definition, but it has no title"
    title = {};
  }

  normalize_label(label_buffer, label);
  if (!label_buffer.empty() &&
      !document.references.contains(std::string_view{label_buffer})) {
    const auto key{document.strings.store(label_buffer)};
    const auto cleaned_url{clean_url(document, url, value_buffer, false)};
    const auto cleaned_title{clean_title(document, title, value_buffer, false)};
    document.references.emplace(
        key, Reference{.url = cleaned_url, .title = cleaned_title});
  }

  return position;
}

// Find the definition of a link label
inline auto find_reference(const Document &document,
                           const std::string_view label, std::string &buffer)
    -> const Reference * {
  if (document.references.empty() || !sourcemeta::core::utf8_codepoint_within(
                                         label, 1, MAXIMUM_LINK_LABEL_LENGTH)) {
    return nullptr;
  }

  normalize_label(buffer, label);
  if (buffer.empty()) {
    return nullptr;
  }

  const auto match{document.references.find(std::string_view{buffer})};
  return match == document.references.end() ? nullptr : &match->second;
}

} // namespace sourcemeta::core::markdown

#endif
