#ifndef SOURCEMETA_CORE_REGEX_IREGEXP_H_
#define SOURCEMETA_CORE_REGEX_IREGEXP_H_

#include <sourcemeta/core/unicode.h>

#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint32_t
#include <optional>     // std::optional, std::nullopt
#include <string>       // std::string, std::to_string
#include <string_view>  // std::string_view
#include <system_error> // std::errc

namespace sourcemeta::core {

namespace {

// RFC 9485 Section 8 permits limiting the composability of patterns that
// challenge the engine. Grouping recurses, so depth is capped to keep
// attacker controlled patterns from overflowing the stack
constexpr std::size_t IREGEXP_MAXIMUM_GROUP_DEPTH{64};

// SingleCharEsc = "\" ( %x28-2B ; '('-'+'
//  / "-" / "." / "?" / %x5B-5E ; '['-'^'
//  / %s"n" / %s"r" / %s"t" / %x7B-7D ; '{'-'}' )
inline auto is_iregexp_single_char_escape(const char character) -> bool {
  return (character >= 0x28 && character <= 0x2B) || character == '-' ||
         character == '.' || character == '?' ||
         (character >= 0x5B && character <= 0x5E) || character == 'n' ||
         character == 'r' || character == 't' ||
         (character >= 0x7B && character <= 0x7D);
}

// charProp = IsCategory, where the Others production is %s"C" with an
// optional second letter drawn from c, f, n, and o, so the bare major
// category conforms while the surrogate form does not. Accepting the major
// category cannot reintroduce surrogates, as they have no representation in
// well formed UTF-8 input
inline auto is_iregexp_category(const std::string_view name) -> bool {
  if (name.empty() || name.size() > 2) {
    return false;
  }

  switch (name.front()) {
    case 'L':
      return name.size() == 1 || std::string_view{"lmotu"}.contains(name[1]);
    case 'M':
      return name.size() == 1 || std::string_view{"cen"}.contains(name[1]);
    case 'N':
      return name.size() == 1 || std::string_view{"dlo"}.contains(name[1]);
    case 'P':
      return name.size() == 1 || std::string_view{"cdefios"}.contains(name[1]);
    case 'Z':
      return name.size() == 1 || std::string_view{"lps"}.contains(name[1]);
    case 'S':
      return name.size() == 1 || std::string_view{"ckmo"}.contains(name[1]);
    case 'C':
      return name.size() == 1 || std::string_view{"cfno"}.contains(name[1]);
    default:
      return false;
  }
}

// Copy one full UTF-8 encoded code point, rejecting malformed sequences.
// The engine later rejects overlong forms and surrogates when compiling
inline auto iregexp_copy_character(const std::string_view pattern,
                                   std::size_t &position, std::string &output)
    -> bool {
  const auto lead{static_cast<unsigned char>(pattern[position])};
  const auto size{utf8_lead_byte_size(lead)};
  if (size == 0 || position + size > pattern.size()) {
    return false;
  }

  for (std::size_t offset{1}; offset < size; ++offset) {
    if (!is_utf8_continuation(
            static_cast<unsigned char>(pattern[position + offset]))) {
      return false;
    }
  }

  output.append(pattern.substr(position, size));
  position += size;
  return true;
}

// charClassEsc = catEsc / complEsc, which along with SingleCharEsc are the
// only escape forms in the entire grammar
inline auto iregexp_escape(const std::string_view pattern,
                           std::size_t &position, std::string &output) -> bool {
  if (position + 1 >= pattern.size()) {
    return false;
  }

  const char next{pattern[position + 1]};
  if (is_iregexp_single_char_escape(next)) {
    output += '\\';
    output += next;
    position += 2;
    return true;
  }

  // catEsc = %s"\p{" charProp "}" and complEsc = %s"\P{" charProp "}"
  if (next == 'p' || next == 'P') {
    if (position + 2 >= pattern.size() || pattern[position + 2] != '{') {
      return false;
    }

    const auto closing{pattern.find('}', position + 3)};
    if (closing == std::string_view::npos) {
      return false;
    }

    const auto name{pattern.substr(position + 3, closing - position - 3)};
    if (!is_iregexp_category(name)) {
      return false;
    }

    output.append(pattern.substr(position, closing - position + 1));
    position = closing + 1;
    return true;
  }

  return false;
}

// CCchar = ( %x00-2C / %x2E-5A / %x5E-D7FF / %xE000-10FFFF ) / SingleCharEsc,
// which excludes exactly the dash, both brackets, and the backslash
inline auto iregexp_class_character(const std::string_view pattern,
                                    std::size_t &position, std::string &output)
    -> bool {
  const char character{pattern[position]};
  if (character == '\\') {
    if (position + 1 < pattern.size() &&
        is_iregexp_single_char_escape(pattern[position + 1])) {
      output += '\\';
      output += pattern[position + 1];
      position += 2;
      return true;
    }

    return false;
  }

  if (character == '-' || character == '[' || character == ']') {
    return false;
  }

  return iregexp_copy_character(pattern, position, output);
}

// CCE1 = ( CCchar [ "-" CCchar ] ) / charClassEsc
inline auto iregexp_class_element(const std::string_view pattern,
                                  std::size_t &position, std::string &output)
    -> bool {
  if (pattern[position] == '\\' && position + 1 < pattern.size() &&
      (pattern[position + 1] == 'p' || pattern[position + 1] == 'P')) {
    return iregexp_escape(pattern, position, output);
  }

  if (!iregexp_class_character(pattern, position, output)) {
    return false;
  }

  // A dash followed by a closing bracket is the optional trailing dash of
  // the enclosing class expression, not a range
  if (position + 1 < pattern.size() && pattern[position] == '-' &&
      pattern[position + 1] != ']') {
    output += '-';
    position += 1;
    return iregexp_class_character(pattern, position, output);
  }

  return true;
}

// charClassExpr = "[" [ "^" ] ( "-" / CCE1 ) *CCE1 [ "-" ] "]", with the
// prose restriction that it is not allowed to match "[^]"
inline auto iregexp_class_expression(const std::string_view pattern,
                                     std::size_t &position, std::string &output)
    -> bool {
  output += '[';
  position += 1;
  if (position < pattern.size() && pattern[position] == '^') {
    output += '^';
    position += 1;
  }

  if (position < pattern.size() && pattern[position] == '-') {
    output += '-';
    position += 1;
  } else {
    if (position >= pattern.size() || pattern[position] == ']') {
      return false;
    }

    if (!iregexp_class_element(pattern, position, output)) {
      return false;
    }
  }

  while (position < pattern.size() && pattern[position] != ']' &&
         pattern[position] != '-') {
    if (!iregexp_class_element(pattern, position, output)) {
      return false;
    }
  }

  if (position < pattern.size() && pattern[position] == '-') {
    output += '-';
    position += 1;
  }

  if (position >= pattern.size() || pattern[position] != ']') {
    return false;
  }

  output += ']';
  position += 1;
  return true;
}

// quantifier = ( "*" / "+" / "?" ) / range-quantifier
// range-quantifier = "{" QuantExact [ "," [ QuantExact ] ] "}"
inline auto iregexp_quantifier(const std::string_view pattern,
                               std::size_t &position, std::string &output)
    -> bool {
  const char character{pattern[position]};
  if (character == '*' || character == '+' || character == '?') {
    output += character;
    position += 1;
    return true;
  }

  const auto minimum_begin{position + 1};
  auto cursor{minimum_begin};
  while (cursor < pattern.size() && pattern[cursor] >= '0' &&
         pattern[cursor] <= '9') {
    cursor += 1;
  }

  // QuantExact = 1*%x30-39, so at least one digit is required. Counts that
  // overflow the representable range are rejected, as RFC 9485 Section 8
  // permits for quantifiers of excessive magnitude
  if (cursor == minimum_begin) {
    return false;
  }

  std::uint32_t minimum{0};
  const auto minimum_result{std::from_chars(pattern.data() + minimum_begin,
                                            pattern.data() + cursor, minimum)};
  if (minimum_result.ec != std::errc{}) {
    return false;
  }

  output += '{';
  output += std::to_string(minimum);
  if (cursor < pattern.size() && pattern[cursor] == ',') {
    output += ',';
    cursor += 1;
    const auto maximum_begin{cursor};
    while (cursor < pattern.size() && pattern[cursor] >= '0' &&
           pattern[cursor] <= '9') {
      cursor += 1;
    }

    if (cursor > maximum_begin) {
      std::uint32_t maximum{0};
      const auto maximum_result{std::from_chars(
          pattern.data() + maximum_begin, pattern.data() + cursor, maximum)};
      // Out of order bounds are an error, consistent with how the permissive
      // dialect rejects them for ECMA 262
      if (maximum_result.ec != std::errc{} || minimum > maximum) {
        return false;
      }

      output += std::to_string(maximum);
    }
  }

  if (cursor >= pattern.size() || pattern[cursor] != '}') {
    return false;
  }

  output += '}';
  position = cursor + 1;
  return true;
}

// i-regexp = branch *( "|" branch ) with branch = *piece, so empty branches
// conform. The caller at depth zero owns the whole input while group
// recursion stops at the closing parenthesis for its caller to consume
inline auto iregexp_branches(const std::string_view pattern,
                             std::size_t &position, std::string &output,
                             const std::size_t depth) -> bool {
  // piece = atom [ quantifier ], so a quantifier is only valid directly
  // after a quantifiable atom and never twice in a row
  bool quantifiable{false};
  while (position < pattern.size()) {
    const char character{pattern[position]};
    if (character == '|') {
      output += '|';
      position += 1;
      quantifiable = false;
    } else if (character == ')') {
      return depth > 0;
    } else if (character == '(') {
      if (depth >= IREGEXP_MAXIMUM_GROUP_DEPTH) {
        return false;
      }

      output += '(';
      position += 1;
      if (!iregexp_branches(pattern, position, output, depth + 1)) {
        return false;
      }

      if (position >= pattern.size() || pattern[position] != ')') {
        return false;
      }

      output += ')';
      position += 1;
      quantifiable = true;
    } else if (character == '*' || character == '+' || character == '?' ||
               character == '{') {
      if (!quantifiable || !iregexp_quantifier(pattern, position, output)) {
        return false;
      }

      quantifiable = false;
    } else if (character == '.') {
      // RFC 9485 Section 5.3: for any unescaped dots outside character
      // classes, "replace the dot with [^\n\r]"
      output += "[^\\n\\r]";
      position += 1;
      quantifiable = true;
    } else if (character == '[') {
      if (!iregexp_class_expression(pattern, position, output)) {
        return false;
      }

      quantifiable = true;
    } else if (character == '\\') {
      if (!iregexp_escape(pattern, position, output)) {
        return false;
      }

      quantifiable = true;
    } else if (character == '^' || character == '$') {
      // RFC 9485 Section 4 defers to XSD semantics, where the caret and the
      // dollar sign outside character classes are ordinary characters, so
      // they are escaped for the engine instead of acting as assertions. A
      // leading caret inside a character class remains the negation marker
      // per the grammar. Note that the Section 5 engine mappings, which are
      // "not normative" by their own introduction, pass these characters
      // through unescaped and therefore silently give them assertion
      // semantics, which is why most implementations and some test suites
      // disagree with the normative behavior implemented here
      output += '\\';
      output += character;
      position += 1;
      quantifiable = true;
    } else if (character == ']' || character == '}') {
      // NormalChar excludes both closing delimiters, so they may only
      // appear escaped
      return false;
    } else {
      if (!iregexp_copy_character(pattern, position, output)) {
        return false;
      }

      quantifiable = true;
    }
  }

  return depth == 0;
}

} // namespace

// Validate a pattern against the RFC 9485 grammar and produce the equivalent
// engine pattern following the RFC 9485 Section 5.4 mapping, which encloses
// the translation "in \A(?: and )\z". Skipping the enclosure yields substring
// search semantics instead of whole input matching
inline auto translate_iregexp(const std::string_view pattern,
                              const bool anchored)
    -> std::optional<std::string> {
  std::string result;
  result.reserve(pattern.size() + 8);
  result += anchored ? "\\A(?:" : "(?:";
  std::size_t position{0};
  if (!iregexp_branches(pattern, position, result, 0)) {
    return std::nullopt;
  }

  result += anchored ? ")\\z" : ")";
  return result;
}

} // namespace sourcemeta::core

#endif
