#ifndef SOURCEMETA_CORE_REGEX_PREPROCESS_H_
#define SOURCEMETA_CORE_REGEX_PREPROCESS_H_

#include <array>       // std::array
#include <bitset>      // std::bitset
#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair

namespace sourcemeta::core {

namespace {

constexpr std::array<std::pair<std::string_view, std::string_view>, 42>
    unicode_property_map{{{"digit", "Nd"},
                          {"Decimal_Number", "Nd"},
                          {"space", "White_Space"},
                          {"White_Space", "White_Space"},
                          {"ASCII", "ASCII"},
                          {"Hex_Digit", "Hex_Digit"},
                          {"Alphabetic", "Alphabetic"},
                          {"Letter", "L"},
                          {"Uppercase_Letter", "Lu"},
                          {"Lowercase_Letter", "Ll"},
                          {"Titlecase_Letter", "Lt"},
                          {"Modifier_Letter", "Lm"},
                          {"Other_Letter", "Lo"},
                          {"Mark", "M"},
                          {"Nonspacing_Mark", "Mn"},
                          {"Spacing_Mark", "Mc"},
                          {"Enclosing_Mark", "Me"},
                          {"Number", "N"},
                          {"Letter_Number", "Nl"},
                          {"Other_Number", "No"},
                          {"Punctuation", "P"},
                          {"Connector_Punctuation", "Pc"},
                          {"Dash_Punctuation", "Pd"},
                          {"Open_Punctuation", "Ps"},
                          {"Close_Punctuation", "Pe"},
                          {"Initial_Punctuation", "Pi"},
                          {"Final_Punctuation", "Pf"},
                          {"Other_Punctuation", "Po"},
                          {"Symbol", "S"},
                          {"Math_Symbol", "Sm"},
                          {"Currency_Symbol", "Sc"},
                          {"Modifier_Symbol", "Sk"},
                          {"Other_Symbol", "So"},
                          {"Separator", "Z"},
                          {"Space_Separator", "Zs"},
                          {"Line_Separator", "Zl"},
                          {"Paragraph_Separator", "Zp"},
                          {"Other", "C"},
                          {"Control", "Cc"},
                          {"Format", "Cf"},
                          {"Unassigned", "Cn"},
                          {"Private_Use", "Co"}}};

constexpr std::string_view shorthand_chars{"dDwWsS"};
constexpr std::string_view simple_escapes{"btnrfv0"};
constexpr std::string_view simple_escape_values{"\b\t\n\r\f\v"};
constexpr std::string_view v_flag_syntax{"-][(){}/'|!#%&*+,.:;<=>?@`~^$"};

inline auto hex_value(char character) -> int {
  if (character >= '0' && character <= '9') {
    return character - '0';
  }

  if (character >= 'a' && character <= 'f') {
    return character - 'a' + 10;
  }

  if (character >= 'A' && character <= 'F') {
    return character - 'A' + 10;
  }

  return -1;
}

inline auto all_hex(const std::string &content, std::size_t start,
                    std::size_t count) -> bool {
  for (std::size_t offset = 0; offset < count; ++offset) {
    if (hex_value(content[start + offset]) < 0) {
      return false;
    }
  }

  return true;
}

inline auto parse_hex_digits(const std::string &content, std::size_t start,
                             std::size_t count) -> int {
  int value = 0;
  for (std::size_t offset = 0; offset < count; ++offset) {
    value = (value << 4) | hex_value(content[start + offset]);
  }

  return value;
}

inline auto set_range(std::bitset<128> &bits, int from, int to) -> void {
  for (int code = from; code <= to; ++code) {
    bits.set(static_cast<std::size_t>(code));
  }
}

inline auto set_shorthand_class(std::bitset<128> &characters,
                                const char shorthand) -> void {
  std::bitset<128> base;
  const char lower = static_cast<char>(shorthand | 32);
  if (lower == 'd') {
    set_range(base, '0', '9');
  } else if (lower == 'w') {
    set_range(base, 'a', 'z');
    set_range(base, 'A', 'Z');
    set_range(base, '0', '9');
    base.set('_');
  } else if (lower == 's') {
    for (const char code : {' ', '\t', '\n', '\r', '\f', '\v'}) {
      base.set(static_cast<std::size_t>(code));
    }
  } else {
    return;
  }

  if (shorthand >= 'A' && shorthand <= 'Z') {
    base.flip();
    base.reset(0);
  }

  characters |= base;
}

inline auto find_bracket_end(const std::string &content, std::size_t start,
                             bool track_nested = true) -> std::size_t {
  for (std::size_t depth = 1, position = start; position < content.size();
       ++position) {
    if (content[position] == '\\' && position + 1 < content.size()) {
      ++position;
    } else if (track_nested && content[position] == '[') {
      ++depth;
    } else if (content[position] == ']' && --depth == 0) {
      return position + 1;
    }
  }

  return content.size();
}

inline auto parse_escape(const std::string &content, std::size_t position,
                         std::size_t &end, int &code_point) -> void {
  if (position >= content.size()) {
    end = position;
    code_point = -1;
    return;
  }

  if (content[position] != '\\' || position + 1 >= content.size()) {
    end = position + 1;
    code_point = static_cast<unsigned char>(content[position]);
    return;
  }

  const char next = content[position + 1];
  if (next == 'x' && position + 3 < content.size() &&
      all_hex(content, position + 2, 2)) {
    end = position + 4;
    code_point = parse_hex_digits(content, position + 2, 2);
    return;
  }

  if (next == 'u' && position + 2 < content.size()) {
    if (content[position + 2] == '{') {
      std::size_t brace_end = position + 3;
      while (brace_end < content.size() && content[brace_end] != '}' &&
             hex_value(content[brace_end]) >= 0) {
        ++brace_end;
      }

      if (brace_end < content.size() && content[brace_end] == '}') {
        const int value =
            parse_hex_digits(content, position + 3, brace_end - position - 3);
        if (value < 128) {
          end = brace_end + 1;
          code_point = value;
          return;
        }
      }
    } else if (position + 5 < content.size() &&
               all_hex(content, position + 2, 4)) {
      const int value = parse_hex_digits(content, position + 2, 4);
      if (value < 128) {
        end = position + 6;
        code_point = value;
        return;
      }
    }
  }

  if (next == 'c' && position + 2 < content.size()) {
    const char control = content[position + 2];
    if ((control >= 'A' && control <= 'Z') ||
        (control >= 'a' && control <= 'z')) {
      end = position + 3;
      code_point = control % 32;
      return;
    }
  }

  end = position + 2;
  const auto escape_index = simple_escapes.find(next);
  if (escape_index < 6) {
    code_point = static_cast<unsigned char>(simple_escape_values[escape_index]);
  } else if (next == '0') {
    code_point = 0;
  } else if (shorthand_chars.find(next) != std::string_view::npos) {
    code_point = -1;
  } else {
    code_point = static_cast<unsigned char>(next);
  }
}

inline auto first_operator(const std::string &content, std::size_t start)
    -> std::pair<std::size_t, char> {
  for (std::size_t depth = 0, position = start; position + 1 < content.size();
       ++position) {
    const char current = content[position];
    if (current == '\\') {
      ++position;
    } else if (current == '[') {
      ++depth;
    } else if (current == ']' && depth > 0) {
      --depth;
    } else if (depth == 0 && (current == '-' || current == '&') &&
               content[position + 1] == current) {
      return {position, current};
    }
  }

  return {std::string::npos, '\0'};
}

inline auto has_nested_brackets(const std::string &content) -> bool {
  for (std::size_t position = 0; position < content.size(); ++position) {
    if (content[position] == '\\') {
      ++position;
    } else if (content[position] == '[') {
      // Found a potential nested bracket - check if it has a closing ]
      // Skip past this [ and look for its matching ]
      std::size_t inner_pos = position + 1;
      while (inner_pos < content.size()) {
        if (content[inner_pos] == '\\' && inner_pos + 1 < content.size()) {
          inner_pos += 2;
        } else if (content[inner_pos] == ']') {
          // Found a matching ] - this is a proper nested bracket
          return true;
        } else {
          ++inner_pos;
        }
      }
      // No matching ] found for this [, so it's just a literal [
      // Continue searching
    }
  }

  return false;
}

// Forward declaration
inline auto expand_set_ops(const std::string &content, std::bitset<128> &result)
    -> bool;

inline auto parse_class_to_bitset(const std::string &content, std::size_t start,
                                  std::bitset<128> &characters) -> std::size_t {
  std::size_t position = start;
  bool negated = position < content.size() && content[position] == '^';
  if (negated) {
    ++position;
  }

  while (position < content.size() && content[position] != ']') {
    if (content[position] == '\\' && position + 1 < content.size() &&
        shorthand_chars.find(content[position + 1]) != std::string_view::npos) {
      set_shorthand_class(characters, content[position + 1]);
      position += 2;
      continue;
    }

    if (content[position] == '[') {
      const auto nested_end = find_bracket_end(content, position + 1);
      const auto nested =
          content.substr(position + 1, nested_end - position - 2);
      std::bitset<128> nested_chars;
      if (first_operator(nested, 0).first != std::string::npos) {
        expand_set_ops(nested, nested_chars);
      } else {
        parse_class_to_bitset(nested, 0, nested_chars);
      }

      characters |= nested_chars;
      position = nested_end;
      continue;
    }

    std::size_t end{0};
    int first{0};
    parse_escape(content, position, end, first);
    if (first < 0) {
      position = end;
      continue;
    }

    if (end < content.size() && content[end] == '-' &&
        end + 1 < content.size() && content[end + 1] != ']') {
      std::size_t range_end{0};
      int second{0};
      parse_escape(content, end + 1, range_end, second);
      if (second >= 0) {
        for (int code = first; code <= second && code < 128; ++code) {
          characters.set(static_cast<std::size_t>(code));
        }

        position = range_end;
        continue;
      }
    }

    if (first < 128) {
      characters.set(static_cast<std::size_t>(first));
    }

    position = end;
  }

  if (negated) {
    for (std::size_t code = 32; code < 128; ++code) {
      characters.flip(code);
    }
  }

  return position;
}

inline auto append_char(std::string &result, std::size_t value) -> void {
  constexpr std::string_view hex{"0123456789abcdef"};
  if (value < 32 || value == 127) {
    result += "\\x";
    result += hex[(value >> 4) & 0xF];
    result += hex[value & 0xF];
  } else {
    if (std::string_view{"-]\\^"}.find(static_cast<char>(value)) !=
        std::string_view::npos) {
      result += '\\';
    }

    result += static_cast<char>(value);
  }
}

inline auto bitset_to_class(const std::bitset<128> &characters) -> std::string {
  std::string result{"["};
  for (std::size_t code = 0; code < 128; ++code) {
    if (!characters.test(code)) {
      continue;
    }

    const std::size_t range_start = code;
    while (code + 1 < 128 && characters.test(code + 1)) {
      ++code;
    }

    if (code - range_start >= 2) {
      append_char(result, range_start);
      result += '-';
      append_char(result, code);
    } else {
      for (std::size_t character = range_start; character <= code;
           ++character) {
        append_char(result, character);
      }
    }
  }

  result += ']';
  return result;
}

inline auto is_valid_escape(const std::string &content, std::size_t position)
    -> bool {
  if (position + 1 >= content.size()) {
    return false;
  }

  const char next = content[position + 1];
  if (std::string_view{"dDwWsSnrtfvb0\\"}.find(next) !=
      std::string_view::npos) {
    return true;
  }

  if (next == 'x') {
    return position + 3 < content.size() && all_hex(content, position + 2, 2);
  }

  if (next == 'u' && position + 2 < content.size()) {
    if (content[position + 2] == '{') {
      for (auto end = position + 3; end < content.size(); ++end) {
        if (content[end] == '}') {
          return true;
        } else if (hex_value(content[end]) < 0) {
          return false;
        }
      }

      return false;
    }

    return position + 5 < content.size() && all_hex(content, position + 2, 4);
  }

  if (next == 'c' && position + 2 < content.size()) {
    const char ctrl = content[position + 2];
    return (ctrl >= 'A' && ctrl <= 'Z') || (ctrl >= 'a' && ctrl <= 'z');
  }

  return v_flag_syntax.find(next) != std::string_view::npos;
}

inline auto is_valid_operand(const std::string &operand) -> bool {
  if (operand.empty()) {
    return false;
  }

  if (operand.front() == '[') {
    if (operand.size() >= 2 && operand.back() == ']') {
      const auto inner = operand.substr(1, operand.size() - 2);
      for (std::size_t position = 0; position < inner.size(); ++position) {
        if (inner[position] == '\\' && position + 1 < inner.size()) {
          if (!is_valid_escape(inner, position)) {
            return false;
          }

          ++position;
        } else if (inner[position] == '|' ||
                   (inner[position] == '-' && (position == inner.size() - 1 ||
                                               inner[position + 1] == ']'))) {
          return false;
        }
      }
    }

    return true;
  }

  if (operand.size() == 2 && operand[0] == '\\' &&
      shorthand_chars.find(operand[1]) != std::string_view::npos) {
    return true;
  }

  for (std::size_t position = 0; position < operand.size(); ++position) {
    if (operand[position] == '\\' && position + 1 < operand.size()) {
      ++position;
    } else if (operand[position] == '-' && position > 0 &&
               position + 1 < operand.size()) {
      return false;
    }
  }

  return true;
}

inline auto parse_operand(const std::string &operand,
                          std::bitset<128> &characters) -> bool {
  if (!is_valid_operand(operand)) {
    return false;
  }

  if (operand.size() >= 2 && operand.front() == '[' && operand.back() == ']') {
    const auto inner = operand.substr(1, operand.size() - 2);
    if (first_operator(inner, 0).first != std::string::npos) {
      return expand_set_ops(inner, characters);
    }
  }

  parse_class_to_bitset(operand, 0, characters);
  return true;
}

inline auto expand_set_ops(const std::string &content, std::bitset<128> &result)
    -> bool {
  auto [op_pos, op_char] = first_operator(content, 0);
  if (op_pos == std::string::npos) {
    parse_class_to_bitset(content, 0, result);
    return true;
  }

  if (auto [next_pos, next_op] = first_operator(content, op_pos + 2);
      next_pos != std::string::npos && next_op != op_char) {
    return false;
  }

  if (!parse_operand(content.substr(0, op_pos), result)) {
    return false;
  }

  for (std::size_t position = op_pos; position + 1 < content.size() &&
                                      content[position] == op_char &&
                                      content[position + 1] == op_char;) {
    auto [next, unused] = first_operator(content, position += 2);
    std::bitset<128> operand_chars;
    if (!parse_operand(next != std::string::npos
                           ? content.substr(position, next - position)
                           : content.substr(position),
                       operand_chars)) {
      return false;
    }

    result =
        op_char == '-' ? (result & ~operand_chars) : (result & operand_chars);
    position = next;
  }

  return true;
}

inline auto expand_char_class(const std::string &content)
    -> std::optional<std::string> {
  if (first_operator(content, 0).first == std::string::npos &&
      !has_nested_brackets(content)) {
    return "[" + content + "]";
  }

  std::bitset<128> result;
  if (!expand_set_ops(content, result)) {
    return std::nullopt;
  }

  return result.none() ? "(?!)" : bitset_to_class(result);
}

inline auto translate_property(const std::string_view name, const bool negated)
    -> std::optional<std::string> {
  for (const auto &[prop_name, pcre_name] : unicode_property_map) {
    if (name == prop_name) {
      return std::string("\\") + (negated ? 'P' : 'p') + '{' +
             std::string(pcre_name) + '}';
    }
  }

  return std::nullopt;
}

inline auto is_escaped(const std::string &pattern, std::size_t index) -> bool {
  std::size_t count = 0;
  for (auto position = index; position > 0 && pattern[position - 1] == '\\';
       --position) {
    ++count;
  }

  return (count % 2) == 1;
}

struct ShorthandExpansion {
  char escape;
  std::string_view inside_class;
  std::string_view outside_class;
};

// clang-format off
constexpr std::array<ShorthandExpansion, 8> shorthand_expansions{{
    {.escape = 'd', .inside_class = "0-9", .outside_class = "[0-9]"},
    {.escape = 'D', .inside_class = "", .outside_class = "[^0-9]"},
    {.escape = 'w', .inside_class = "a-zA-Z0-9_", .outside_class = "[a-zA-Z0-9_]"},
    {.escape = 'W', .inside_class = "", .outside_class = "[^a-zA-Z0-9_]"},
    {.escape = 's', .inside_class = R"(\t\v\f \x{00A0}\x{FEFF}\p{Zs}\n\r\x{2028}\x{2029})",
     .outside_class = R"([\t\v\f \x{00A0}\x{FEFF}\p{Zs}\n\r\x{2028}\x{2029}])"},
    {.escape = 'S', .inside_class = "", .outside_class = R"([^\t\v\f \x{00A0}\x{FEFF}\p{Zs}\n\r\x{2028}\x{2029}])"},
    {.escape = 'b', .inside_class = "", .outside_class = R"((?:(?<![a-zA-Z0-9_])(?=[a-zA-Z0-9_])|(?<=[a-zA-Z0-9_])(?![a-zA-Z0-9_])))"},
    {.escape = 'B', .inside_class = "", .outside_class = R"((?:(?<=[a-zA-Z0-9_])(?=[a-zA-Z0-9_])|(?<![a-zA-Z0-9_])(?![a-zA-Z0-9_])))"},
}};
// clang-format on

inline auto find_shorthand(char escape) -> const ShorthandExpansion * {
  for (const auto &expansion : shorthand_expansions) {
    if (expansion.escape == escape) {
      return &expansion;
    }
  }

  return nullptr;
}

} // namespace

inline auto preprocess_regex(const std::string &pattern)
    -> std::optional<std::string> {
  std::string result;
  result.reserve(pattern.size() * 2);
  bool in_class = false;

  for (std::size_t position = 0; position < pattern.size(); ++position) {
    const char current = pattern[position];
    if (current == '[' && !is_escaped(pattern, position) && !in_class) {
      // Find end both ways and check which applies
      const auto simple_end = find_bracket_end(pattern, position + 1, false);
      const auto nested_end = find_bracket_end(pattern, position + 1, true);

      const auto nested_content =
          pattern.substr(position + 1, nested_end - position - 2);

      // Check for v-flag operators in nested content
      const bool nested_has_ops =
          nested_content.find("--") != std::string::npos ||
          nested_content.find("&&") != std::string::npos;

      // Check if nested content starts with a nested bracket
      // This indicates true v-flag nested class syntax like [[a-z]...]
      const bool starts_with_nested =
          !nested_content.empty() && nested_content[0] == '[';

      // Use v-flag mode if:
      // 1. Nested content has v-flag operators (-- or &&), OR
      // 2. Content starts with [ (indicating v-flag nested class syntax)
      //    AND the ends differ (so there's actual nesting being tracked)
      const bool use_v_flag =
          nested_has_ops || (starts_with_nested && simple_end != nested_end);

      if (use_v_flag) {
        const auto expanded = expand_char_class(nested_content);
        if (!expanded) {
          return std::nullopt;
        }

        result += *expanded;
        position = nested_end - 1;
        continue;
      }

      // No v-flag syntax - use standard mode
      // This handles standard patterns like [^!*,;{}[\]~\n] where [ is literal
      in_class = true;
    } else if (current == ']' && !is_escaped(pattern, position)) {
      in_class = false;
    }

    if (current != '\\' || position + 1 >= pattern.size()) {
      result += current;
      continue;
    }

    const char next = pattern[position + 1];
    if (std::string_view{"\\[]^$"}.find(next) != std::string_view::npos) {
      result += current;
      result += next;
      ++position;
      continue;
    }

    if (next == 'u' && position + 2 < pattern.size()) {
      if (pattern[position + 2] == '{') {
        result += "\\x{";
        for (position += 3;
             position < pattern.size() && pattern[position] != '}';
             ++position) {
          result += pattern[position];
        }

        if (position < pattern.size()) {
          result += '}';
        }

        continue;
      }

      if (position + 5 < pattern.size() && all_hex(pattern, position + 2, 4)) {
        result += "\\x{" + pattern.substr(position + 2, 4) + '}';
        position += 5;
        continue;
      }
    }

    if ((next == 'p' || next == 'P') && position + 2 < pattern.size() &&
        pattern[position + 2] == '{') {
      const auto start = position;
      std::string name;
      for (position += 3; position < pattern.size() && pattern[position] != '}';
           ++position) {
        name += pattern[position];
      }

      if (position < pattern.size()) {
        if (auto translated = translate_property(name, next == 'P')) {
          result += *translated;
        } else {
          result += pattern.substr(start, position - start + 1);
        }
      } else {
        position = start;
        result += current;
      }

      continue;
    }

    if (const auto *expansion = find_shorthand(next)) {
      if (in_class && expansion->inside_class.empty()) {
        result += std::string{current} + next;
      } else {
        result += in_class ? expansion->inside_class : expansion->outside_class;
      }

      ++position;
    } else {
      result += current;
    }
  }

  return result;
}

} // namespace sourcemeta::core

#endif
