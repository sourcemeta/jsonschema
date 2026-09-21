#ifndef SOURCEMETA_CORE_MARKDOWN_SCANNERS_H_
#define SOURCEMETA_CORE_MARKDOWN_SCANNERS_H_

#include <sourcemeta/core/email.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include "characters.h"

#include <algorithm>   // std::binary_search
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

// Every scanner matches at the given offset of the input, never looks past
// its end, and returns the length of the match, where zero means no match

namespace sourcemeta::core::markdown {

inline auto character_at(const std::string_view input,
                         const std::size_t index) noexcept -> char {
  return index < input.size() ? input[index] : '\0';
}

// The whitespace that raw HTML allows between its parts
inline auto is_html_space(const char character) noexcept -> bool {
  return character == ' ' || character == '\t' || character == '\v' ||
         character == '\f' || character == '\r' || character == '\n';
}

// The whitespace that the table and task list extensions allow within a line
inline auto is_line_space(const char character) noexcept -> bool {
  return character == ' ' || character == '\t' || character == '\v' ||
         character == '\f';
}

inline auto scan_spacechars(const std::string_view input,
                            const std::size_t offset) noexcept -> std::size_t {
  auto position{offset};
  while (position < input.size() && is_html_space(input[position])) {
    ++position;
  }

  return position - offset;
}

// An absolute URI of GFM section 6.9, right after the opening angle bracket
inline auto scan_autolink_uri(const std::string_view input,
                              const std::size_t offset) noexcept
    -> std::size_t {
  auto position{offset};
  if (!sourcemeta::core::is_alpha(character_at(input, position))) {
    return 0;
  }

  ++position;
  std::size_t scheme_length{0};
  while (scheme_length < 31) {
    const auto character{character_at(input, position)};
    if (!sourcemeta::core::is_alphanum(character) && character != '.' &&
        character != '+' && character != '-') {
      break;
    }

    ++position;
    ++scheme_length;
  }

  if (scheme_length == 0 || character_at(input, position) != ':') {
    return 0;
  }

  ++position;
  while (position < input.size()) {
    const auto character{static_cast<unsigned char>(input[position])};
    if (character <= 0x20 || character == '<' || character == '>') {
      break;
    }

    ++position;
  }

  if (character_at(input, position) != '>') {
    return 0;
  }

  return position + 1 - offset;
}

// An email address of GFM section 6.9, right after the opening angle bracket,
// which is a valid email address of the HTML Standard up to the closing angle
// bracket
inline auto scan_autolink_email(const std::string_view input,
                                const std::size_t offset) -> std::size_t {
  auto position{offset};
  while (position < input.size()) {
    const auto character{static_cast<unsigned char>(input[position])};
    if (character <= 0x20 || character == '<' || character == '>') {
      break;
    }

    ++position;
  }

  if (character_at(input, position) != '>' ||
      !sourcemeta::core::is_html_email(
          input.substr(offset, position - offset))) {
    return 0;
  }

  return position + 1 - offset;
}

inline auto scan_tag_name(const std::string_view input,
                          const std::size_t offset) noexcept -> std::size_t {
  if (!sourcemeta::core::is_alpha(character_at(input, offset))) {
    return 0;
  }

  auto position{offset + 1};
  while (position < input.size() &&
         (sourcemeta::core::is_alphanum(input[position]) ||
          input[position] == '-')) {
    ++position;
  }

  return position - offset;
}

inline auto is_attribute_name_start(const char character) noexcept -> bool {
  return sourcemeta::core::is_alpha(character) || character == '_' ||
         character == ':';
}

inline auto is_attribute_name_character(const char character) noexcept -> bool {
  return sourcemeta::core::is_alphanum(character) || character == '_' ||
         character == ':' || character == '.' || character == '-';
}

// The value of an attribute, at the equals sign or the whitespace before it
inline auto
scan_attribute_value_specification(const std::string_view input,
                                   const std::size_t offset) noexcept
    -> std::size_t {
  auto position{offset + scan_spacechars(input, offset)};
  if (character_at(input, position) != '=') {
    return 0;
  }

  ++position;
  position += scan_spacechars(input, position);
  const auto quote{character_at(input, position)};
  if (quote == '"' || quote == '\'') {
    const auto closing{input.find(quote, position + 1)};
    if (closing == std::string_view::npos) {
      return 0;
    }

    return closing + 1 - offset;
  }

  const auto value_start{position};
  while (position < input.size()) {
    const auto character{input[position]};
    if (character == ' ' || character == '\t' || character == '\r' ||
        character == '\n' || character == '\v' || character == '\f' ||
        character == '"' || character == '\'' || character == '=' ||
        character == '<' || character == '>' || character == '`') {
      break;
    }

    ++position;
  }

  if (position == value_start) {
    return 0;
  }

  return position - offset;
}

// An open tag or a closing tag of GFM section 6.6, right after the opening
// angle bracket
inline auto scan_html_tag(const std::string_view input,
                          const std::size_t offset) noexcept -> std::size_t {
  auto position{offset};
  if (character_at(input, position) == '/') {
    ++position;
    const auto name_length{scan_tag_name(input, position)};
    if (name_length == 0) {
      return 0;
    }

    position += name_length;
    position += scan_spacechars(input, position);
    if (character_at(input, position) != '>') {
      return 0;
    }

    return position + 1 - offset;
  }

  const auto name_length{scan_tag_name(input, position)};
  if (name_length == 0) {
    return 0;
  }

  position += name_length;
  while (true) {
    const auto spaces{scan_spacechars(input, position)};
    if (spaces == 0 ||
        !is_attribute_name_start(character_at(input, position + spaces))) {
      break;
    }

    auto name_end{position + spaces + 1};
    while (name_end < input.size() &&
           is_attribute_name_character(input[name_end])) {
      ++name_end;
    }

    position = name_end + scan_attribute_value_specification(input, name_end);
  }

  position += scan_spacechars(input, position);
  if (character_at(input, position) == '/') {
    ++position;
  }

  if (character_at(input, position) != '>') {
    return 0;
  }

  return position + 1 - offset;
}

// An HTML comment of GFM section 6.10, at the two dashes that follow the
// exclamation mark: "An HTML comment consists of <!-- + text + -->, where text
// does not start with > or ->, does not end with -, and does not contain --"
inline auto scan_html_comment(const std::string_view input,
                              const std::size_t offset) noexcept
    -> std::size_t {
  if (character_at(input, offset) != '-' ||
      character_at(input, offset + 1) != '-') {
    return 0;
  }

  const auto text{offset + 2};
  if (character_at(input, text) == '>' ||
      (character_at(input, text) == '-' &&
       character_at(input, text + 1) == '>')) {
    return 0;
  }

  // As the text can neither contain two dashes nor end with one, the first two
  // dashes after the opening have to be the start of the closing
  const auto closing{input.find("--", text)};
  if (closing == std::string_view::npos ||
      character_at(input, closing + 2) != '>') {
    return 0;
  }

  return closing + 3 - offset;
}

// The content of a processing instruction, right after its question mark,
// without the closing question mark and angle bracket
inline auto scan_html_processing_instruction(const std::string_view input,
                                             const std::size_t offset) noexcept
    -> std::size_t {
  auto position{offset};
  while (position < input.size()) {
    if (input[position] == '?') {
      if (position + 1 < input.size() && input[position + 1] != '>') {
        ++position;
        continue;
      }

      break;
    }

    ++position;
  }

  return position - offset;
}

// A declaration, right after its exclamation mark, without the closing angle
// bracket
inline auto scan_html_declaration(const std::string_view input,
                                  const std::size_t offset) noexcept
    -> std::size_t {
  auto position{offset};
  while (position < input.size() && input[position] >= 'A' &&
         input[position] <= 'Z') {
    ++position;
  }

  if (position == offset) {
    return 0;
  }

  const auto spaces{scan_spacechars(input, position)};
  if (spaces == 0) {
    return 0;
  }

  const auto closing{input.find('>', position + spaces)};
  return (closing == std::string_view::npos ? input.size() : closing) - offset;
}

// A CDATA section, right after its opening bracket, without the closing
// brackets and angle bracket
inline auto scan_html_cdata(const std::string_view input,
                            const std::size_t offset) noexcept -> std::size_t {
  if (offset > input.size() ||
      input.substr(offset, 6) != std::string_view{"CDATA["}) {
    return 0;
  }

  auto boundary{offset + 6};
  std::size_t brackets{0};
  for (auto position{offset + 6}; position < input.size(); ++position) {
    const auto character{input[position]};
    if (brackets == 2) {
      if (character == '>') {
        break;
      }

      brackets = 0;
      boundary = position + 1;
    } else if (character == ']') {
      ++brackets;
    } else {
      brackets = 0;
      boundary = position + 1;
    }
  }

  return boundary - offset;
}

// The lowercase names of the HTML block condition 6 of GFM section 4.6
constexpr std::array<std::string_view, 62> BLOCK_TAG_NAMES{
    {"address",  "article",    "aside",  "base",     "basefont", "blockquote",
     "body",     "caption",    "center", "col",      "colgroup", "dd",
     "details",  "dialog",     "dir",    "div",      "dl",       "dt",
     "fieldset", "figcaption", "figure", "footer",   "form",     "frame",
     "frameset", "h1",         "h2",     "h3",       "h4",       "h5",
     "h6",       "head",       "header", "hr",       "html",     "iframe",
     "legend",   "li",         "link",   "main",     "menu",     "menuitem",
     "nav",      "noframes",   "ol",     "optgroup", "option",   "p",
     "param",    "section",    "source", "summary",  "table",    "tbody",
     "td",       "tfoot",      "th",     "thead",    "title",    "tr",
     "track",    "ul"}};

// Returns the lowercase alphanumeric run at the given offset, if it is short
// enough to be a tag name that the HTML block conditions care about
inline auto lowercase_tag_name(const std::string_view input,
                               const std::size_t offset,
                               std::array<char, 16> &buffer) noexcept
    -> std::string_view {
  std::size_t length{0};
  while (offset + length < input.size() &&
         sourcemeta::core::is_alphanum(input[offset + length])) {
    if (length == buffer.size()) {
      return {};
    }

    buffer[length] = sourcemeta::core::to_lowercase(input[offset + length]);
    ++length;
  }

  return {buffer.data(), length};
}

// The start condition of an HTML block of GFM section 4.6, returning the
// number of the condition from one to six or zero
inline auto scan_html_block_start(const std::string_view input,
                                  const std::size_t offset) noexcept
    -> std::size_t {
  if (character_at(input, offset) != '<') {
    return 0;
  }

  const auto next{character_at(input, offset + 1)};
  if (next == '!') {
    const auto after{character_at(input, offset + 2)};
    if (after == '-' && character_at(input, offset + 3) == '-') {
      return 2;
    }

    if (after == '[' && sourcemeta::core::starts_with_ignore_case(
                            input.substr(offset + 3), "cdata[")) {
      return 5;
    }

    return after >= 'A' && after <= 'Z' ? 4 : 0;
  }

  if (next == '?') {
    return 3;
  }

  std::array<char, 16> buffer{};
  const auto name{lowercase_tag_name(input, offset + 1, buffer)};
  if (name == "script" || name == "pre" || name == "textarea" ||
      name == "style") {
    const auto after{character_at(input, offset + 1 + name.size())};
    return is_html_space(after) || after == '>' ? 1 : 0;
  }

  auto position{offset + 1};
  if (next == '/') {
    ++position;
  }

  const auto block_name{lowercase_tag_name(input, position, buffer)};
  if (block_name.empty() ||
      !std::binary_search(BLOCK_TAG_NAMES.cbegin(), BLOCK_TAG_NAMES.cend(),
                          block_name)) {
    return 0;
  }

  position += block_name.size();
  const auto after{character_at(input, position)};
  if (is_html_space(after) || after == '>') {
    return 6;
  }

  return after == '/' && character_at(input, position + 1) == '>' ? 6 : 0;
}

// The start condition 7 of an HTML block of GFM section 4.6
inline auto scan_html_block_start_7(const std::string_view input,
                                    const std::size_t offset) noexcept -> bool {
  if (character_at(input, offset) != '<') {
    return false;
  }

  const auto length{scan_html_tag(input, offset + 1)};
  if (length == 0) {
    return false;
  }

  auto position{offset + 1 + length};
  while (position < input.size() &&
         (input[position] == '\t' || input[position] == '\f' ||
          input[position] == ' ')) {
    ++position;
  }

  const auto character{character_at(input, position)};
  return character == '\n' || character == '\r';
}

// The end conditions of the HTML blocks of GFM section 4.6
inline auto scan_html_block_end(const std::string_view input,
                                const std::size_t offset,
                                const std::size_t condition) noexcept -> bool {
  if (offset > input.size()) {
    return false;
  }

  const auto line{input.substr(offset)};
  switch (condition) {
    case 1: {
      auto position{line.find("</")};
      while (position != std::string_view::npos) {
        std::array<char, 16> buffer{};
        const auto name{lowercase_tag_name(line, position + 2, buffer)};
        if ((name == "script" || name == "pre" || name == "textarea" ||
             name == "style") &&
            character_at(line, position + 2 + name.size()) == '>') {
          return true;
        }

        position = line.find("</", position + 1);
      }

      return false;
    }

    case 2:
      return line.find("-->") != std::string_view::npos;
    case 3:
      return line.find("?>") != std::string_view::npos;
    case 4:
      return line.find('>') != std::string_view::npos;
    case 5:
      return line.find("]]>") != std::string_view::npos;
    default:
      return false;
  }
}

// A link title of GFM section 6.3, which "consists of either a sequence of zero
// or more characters between straight double-quote characters ("), including a
// " character only if it is backslash-escaped, or a sequence of zero or more
// characters between straight single-quote characters ('), including a '
// character only if it is backslash-escaped, or a sequence of zero or more
// characters between matching parentheses ((...)), including a ( or )
// character only if it is backslash-escaped"
inline auto scan_link_title(const std::string_view input,
                            const std::size_t offset) noexcept -> std::size_t {
  const auto opening{character_at(input, offset)};
  if (opening != '"' && opening != '\'' && opening != '(') {
    return 0;
  }

  const auto closing{opening == '(' ? ')' : opening};
  for (auto position{offset + 1}; position < input.size(); ++position) {
    const auto character{input[position]};
    // GFM section 2.4: "Any ASCII punctuation character may be
    // backslash-escaped"
    if (character == '\\' && position + 1 < input.size() &&
        sourcemeta::core::is_punctuation(input[position + 1])) {
      ++position;
      continue;
    }

    if (character == closing) {
      return position + 1 - offset;
    }

    if (opening == '(' && character == '(') {
      return 0;
    }
  }

  return 0;
}

// The start of an ATX heading of GFM section 4.2, including the whitespace
// after its opening sequence
inline auto scan_atx_heading_start(const std::string_view input,
                                   const std::size_t offset) noexcept
    -> std::size_t {
  auto position{offset};
  while (position < input.size() && input[position] == '#') {
    ++position;
  }

  const auto hashes{position - offset};
  if (hashes == 0 || hashes > 6) {
    return 0;
  }

  const auto character{character_at(input, position)};
  if (character == ' ' || character == '\t') {
    while (position < input.size() &&
           (input[position] == ' ' || input[position] == '\t')) {
      ++position;
    }

    return position - offset;
  }

  return character == '\n' || character == '\r' ? position + 1 - offset : 0;
}

// The underline of a setext heading of GFM section 4.3, returning the level
// of the heading or zero
inline auto scan_setext_heading_line(const std::string_view input,
                                     const std::size_t offset) noexcept
    -> std::uint8_t {
  const auto marker{character_at(input, offset)};
  if (marker != '=' && marker != '-') {
    return 0;
  }

  auto position{offset};
  while (position < input.size() && input[position] == marker) {
    ++position;
  }

  while (position < input.size() &&
         (input[position] == ' ' || input[position] == '\t')) {
    ++position;
  }

  const auto character{character_at(input, position)};
  if (character != '\n' && character != '\r') {
    return 0;
  }

  return marker == '=' ? 1 : 2;
}

// The opening fence of a fenced code block of GFM section 4.5, returning the
// length of the fence
inline auto scan_open_code_fence(const std::string_view input,
                                 const std::size_t offset) noexcept
    -> std::size_t {
  const auto marker{character_at(input, offset)};
  if (marker != '`' && marker != '~') {
    return 0;
  }

  auto position{offset};
  while (position < input.size() && input[position] == marker) {
    ++position;
  }

  const auto length{position - offset};
  if (length < 3) {
    return 0;
  }

  while (position < input.size()) {
    const auto character{input[position]};
    if (character == '\n' || character == '\r') {
      return length;
    }

    if (marker == '`' && character == '`') {
      return 0;
    }

    ++position;
  }

  return 0;
}

// The closing fence of a fenced code block of GFM section 4.5, returning the
// length of the fence
inline auto scan_close_code_fence(const std::string_view input,
                                  const std::size_t offset) noexcept
    -> std::size_t {
  const auto marker{character_at(input, offset)};
  if (marker != '`' && marker != '~') {
    return 0;
  }

  auto position{offset};
  while (position < input.size() && input[position] == marker) {
    ++position;
  }

  const auto length{position - offset};
  if (length < 3) {
    return 0;
  }

  while (position < input.size() &&
         (input[position] == ' ' || input[position] == '\t')) {
    ++position;
  }

  const auto character{character_at(input, position)};
  return character == '\n' || character == '\r' ? length : 0;
}

// The image media types whose data URLs safe mode keeps
constexpr std::array<std::string_view, 4> SAFE_DATA_URL_PREFIXES{
    {"data:image/png", "data:image/gif", "data:image/jpeg", "data:image/webp"}};

// Whether a link destination starts with a scheme that can run code or read
// local files, where only a few image data URLs are safe. RFC 2397 Section 3
// writes a data URL as `dataurl := "data:" [ mediatype ] [ ";base64" ] ","
// data` with `mediatype := [ type "/" subtype ] *( ";" parameter )`, so the
// subtype of an allowed image ends at a semicolon or a comma
inline auto is_dangerous_url(const std::string_view url) noexcept -> bool {
  for (const auto prefix : SAFE_DATA_URL_PREFIXES) {
    if (sourcemeta::core::starts_with_ignore_case(url, prefix) &&
        url.size() > prefix.size() &&
        (url[prefix.size()] == ';' || url[prefix.size()] == ',')) {
      return false;
    }
  }

  return sourcemeta::core::starts_with_ignore_case(url, "javascript:") ||
         sourcemeta::core::starts_with_ignore_case(url, "vbscript:") ||
         sourcemeta::core::starts_with_ignore_case(url, "file:") ||
         sourcemeta::core::starts_with_ignore_case(url, "data:");
}

// The start of a footnote definition, including the whitespace after its
// colon
inline auto scan_footnote_definition(const std::string_view input,
                                     const std::size_t offset) noexcept
    -> std::size_t {
  if (character_at(input, offset) != '[' ||
      character_at(input, offset + 1) != '^') {
    return 0;
  }

  auto position{offset + 2};
  while (position < input.size()) {
    const auto character{input[position]};
    if (character == ']' || character == ' ' || character == '\r' ||
        character == '\n' || character == '\t') {
      break;
    }

    ++position;
  }

  if (position == offset + 2 || character_at(input, position) != ']' ||
      character_at(input, position + 1) != ':') {
    return 0;
  }

  position += 2;
  while (position < input.size() &&
         (input[position] == ' ' || input[position] == '\t')) {
    ++position;
  }

  return position - offset;
}

inline auto scan_table_marker(const std::string_view input,
                              std::size_t position) noexcept -> std::size_t {
  const auto start{position};
  while (position < input.size() && is_line_space(input[position])) {
    ++position;
  }

  if (character_at(input, position) == ':') {
    ++position;
  }

  const auto dashes_start{position};
  while (position < input.size() && input[position] == '-') {
    ++position;
  }

  if (position == dashes_start) {
    return 0;
  }

  if (character_at(input, position) == ':') {
    ++position;
  }

  while (position < input.size() && is_line_space(input[position])) {
    ++position;
  }

  return position - start;
}

// The end of a table row, which is any whitespace and a line ending
inline auto scan_table_row_end(const std::string_view input,
                               const std::size_t offset) noexcept
    -> std::size_t {
  if (offset >= input.size()) {
    return 0;
  }

  auto position{offset};
  while (position < input.size() && is_line_space(input[position])) {
    ++position;
  }

  if (character_at(input, position) == '\r') {
    ++position;
  }

  return character_at(input, position) == '\n' ? position + 1 - offset : 0;
}

// The delimiter row of a table of GFM section 4.10
inline auto scan_table_start(const std::string_view input,
                             const std::size_t offset) noexcept -> std::size_t {
  if (offset >= input.size()) {
    return 0;
  }

  auto position{offset};
  if (input[position] == '|') {
    ++position;
  }

  const auto first{scan_table_marker(input, position)};
  if (first == 0) {
    return 0;
  }

  position += first;
  while (character_at(input, position) == '|') {
    const auto marker{scan_table_marker(input, position + 1)};
    if (marker == 0) {
      ++position;
      break;
    }

    position += 1 + marker;
  }

  const auto end{scan_table_row_end(input, position)};
  return end == 0 ? 0 : position + end - offset;
}

// The content of a table cell, which ends at a line ending or at a pipe that
// is not preceded by a backslash
inline auto scan_table_cell(const std::string_view input,
                            const std::size_t offset) noexcept -> std::size_t {
  auto position{offset};
  while (position < input.size()) {
    const auto character{input[position]};
    if (character == '\r' || character == '\n') {
      break;
    }

    if (character == '|' &&
        (position == offset || input[position - 1] != '\\')) {
      break;
    }

    ++position;
  }

  return position - offset;
}

// A pipe that ends a table cell, including the whitespace after it
inline auto scan_table_cell_end(const std::string_view input,
                                const std::size_t offset) noexcept
    -> std::size_t {
  if (character_at(input, offset) != '|') {
    return 0;
  }

  auto position{offset + 1};
  while (position < input.size() && is_line_space(input[position])) {
    ++position;
  }

  return position - offset;
}

// A task list item marker of GFM section 5.3 at the start of the content of a
// list item, which "consists of an optional number of spaces, a left bracket
// ([), either a whitespace character or the letter x in either lowercase or
// uppercase, and then a right bracket (])", where the list item already took
// the spaces, and which needs "at least one whitespace character before any
// other content". A line ending cannot come between the brackets, so the
// whitespace there is one of the other whitespace characters of GFM section 2.1
inline auto scan_task_list_marker(const std::string_view input,
                                  const std::size_t position) noexcept -> bool {
  if (character_at(input, position) != '[' ||
      character_at(input, position + 2) != ']') {
    return false;
  }

  const auto state{character_at(input, position + 1)};
  return (is_line_space(state) || state == 'x' || state == 'X') &&
         is_line_space(character_at(input, position + 3));
}

} // namespace sourcemeta::core::markdown

#endif
