#ifndef SOURCEMETA_CORE_REGEX_PREPROCESS_H_
#define SOURCEMETA_CORE_REGEX_PREPROCESS_H_

#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

inline auto is_escaped(const std::string &pattern, const std::size_t index)
    -> bool {
  std::size_t count{0};
  std::size_t position{index};
  while (position > 0 && pattern[position - 1] == '\\') {
    ++count;
    --position;
  }

  return (count % 2) == 1;
}

inline auto translate_unicode_property(const std::string_view property_name,
                                       const bool negated)
    -> std::optional<std::string> {
  const char prefix{negated ? 'P' : 'p'};

  // NOLINTNEXTLINE(bugprone-branch-clone)
  if (property_name == "digit") {
    return std::string("\\") + prefix + "{Nd}";
  }
  // NOLINTNEXTLINE(bugprone-branch-clone)
  else if (property_name == "space") {
    return std::string("\\") + prefix + "{White_Space}";
  } else if (property_name == "ASCII") {
    return std::string("\\") + prefix + "{ASCII}";
  } else if (property_name == "Hex_Digit") {
    return std::string("\\") + prefix + "{Hex_Digit}";
  } else if (property_name == "Alphabetic") {
    return std::string("\\") + prefix + "{Alphabetic}";
  } else if (property_name == "White_Space") {
    return std::string("\\") + prefix + "{White_Space}";
  } else if (property_name == "Letter") {
    return std::string("\\") + prefix + "{L}";
  } else if (property_name == "Uppercase_Letter") {
    return std::string("\\") + prefix + "{Lu}";
  } else if (property_name == "Lowercase_Letter") {
    return std::string("\\") + prefix + "{Ll}";
  } else if (property_name == "Titlecase_Letter") {
    return std::string("\\") + prefix + "{Lt}";
  } else if (property_name == "Modifier_Letter") {
    return std::string("\\") + prefix + "{Lm}";
  } else if (property_name == "Other_Letter") {
    return std::string("\\") + prefix + "{Lo}";
  } else if (property_name == "Mark") {
    return std::string("\\") + prefix + "{M}";
  } else if (property_name == "Nonspacing_Mark") {
    return std::string("\\") + prefix + "{Mn}";
  } else if (property_name == "Spacing_Mark") {
    return std::string("\\") + prefix + "{Mc}";
  } else if (property_name == "Enclosing_Mark") {
    return std::string("\\") + prefix + "{Me}";
  } else if (property_name == "Number") {
    return std::string("\\") + prefix + "{N}";
  } else if (property_name == "Decimal_Number") {
    return std::string("\\") + prefix + "{Nd}";
  } else if (property_name == "Letter_Number") {
    return std::string("\\") + prefix + "{Nl}";
  } else if (property_name == "Other_Number") {
    return std::string("\\") + prefix + "{No}";
  } else if (property_name == "Punctuation") {
    return std::string("\\") + prefix + "{P}";
  } else if (property_name == "Connector_Punctuation") {
    return std::string("\\") + prefix + "{Pc}";
  } else if (property_name == "Dash_Punctuation") {
    return std::string("\\") + prefix + "{Pd}";
  } else if (property_name == "Open_Punctuation") {
    return std::string("\\") + prefix + "{Ps}";
  } else if (property_name == "Close_Punctuation") {
    return std::string("\\") + prefix + "{Pe}";
  } else if (property_name == "Initial_Punctuation") {
    return std::string("\\") + prefix + "{Pi}";
  } else if (property_name == "Final_Punctuation") {
    return std::string("\\") + prefix + "{Pf}";
  } else if (property_name == "Other_Punctuation") {
    return std::string("\\") + prefix + "{Po}";
  } else if (property_name == "Symbol") {
    return std::string("\\") + prefix + "{S}";
  } else if (property_name == "Math_Symbol") {
    return std::string("\\") + prefix + "{Sm}";
  } else if (property_name == "Currency_Symbol") {
    return std::string("\\") + prefix + "{Sc}";
  } else if (property_name == "Modifier_Symbol") {
    return std::string("\\") + prefix + "{Sk}";
  } else if (property_name == "Other_Symbol") {
    return std::string("\\") + prefix + "{So}";
  } else if (property_name == "Separator") {
    return std::string("\\") + prefix + "{Z}";
  } else if (property_name == "Space_Separator") {
    return std::string("\\") + prefix + "{Zs}";
  } else if (property_name == "Line_Separator") {
    return std::string("\\") + prefix + "{Zl}";
  } else if (property_name == "Paragraph_Separator") {
    return std::string("\\") + prefix + "{Zp}";
  } else if (property_name == "Other") {
    return std::string("\\") + prefix + "{C}";
  } else if (property_name == "Control") {
    return std::string("\\") + prefix + "{Cc}";
  } else if (property_name == "Format") {
    return std::string("\\") + prefix + "{Cf}";
  } else if (property_name == "Unassigned") {
    return std::string("\\") + prefix + "{Cn}";
  } else if (property_name == "Private_Use") {
    return std::string("\\") + prefix + "{Co}";
  }

  return std::nullopt;
}

} // namespace

inline auto preprocess_regex(const std::string &pattern) -> std::string {
  std::string result;
  result.reserve(pattern.size() * 2);
  bool in_char_class{false};

  for (std::size_t index{0}; index < pattern.size(); ++index) {
    if (pattern[index] == '[' && !is_escaped(pattern, index)) {
      if (in_char_class) {
        result += "\\[";
      } else {
        in_char_class = true;
        result += pattern[index];
      }
      continue;
    }

    if (pattern[index] == ']' && !is_escaped(pattern, index)) {
      in_char_class = false;
      result += pattern[index];
      continue;
    }

    if (pattern[index] == '\\' && index + 1 < pattern.size()) {
      const char next_char{pattern[index + 1]};

      if (next_char == '\\') {
        result += "\\\\";
        ++index;
        continue;
      }

      if (next_char == '[' || next_char == ']' || next_char == '^' ||
          next_char == '$') {
        result += '\\';
        result += next_char;
        ++index;
        continue;
      }

      if (next_char == 'u' && index + 2 < pattern.size() &&
          pattern[index + 2] == '{') {
        result += "\\x{";
        index += 2;
        for (++index; index < pattern.size() && pattern[index] != '}';
             ++index) {
          result += pattern[index];
        }
        if (index < pattern.size()) {
          result += '}';
        }
        continue;
      }

      if (next_char == 'u' && index + 5 < pattern.size()) {
        bool is_hex{true};
        for (std::size_t offset{0}; offset < 4; ++offset) {
          const char hex_char{pattern[index + 2 + offset]};
          if (!((hex_char >= '0' && hex_char <= '9') ||
                (hex_char >= 'a' && hex_char <= 'f') ||
                (hex_char >= 'A' && hex_char <= 'F'))) {
            is_hex = false;
            break;
          }
        }

        if (is_hex) {
          result += "\\x{";
          result += pattern[index + 2];
          result += pattern[index + 3];
          result += pattern[index + 4];
          result += pattern[index + 5];
          result += '}';
          index += 5;
          continue;
        }
      }

      if ((next_char == 'p' || next_char == 'P') &&
          index + 2 < pattern.size() && pattern[index + 2] == '{') {
        const bool negated{next_char == 'P'};
        const std::size_t start_index{index};
        index += 3;
        std::string property_name;
        while (index < pattern.size() && pattern[index] != '}') {
          property_name += pattern[index];
          ++index;
        }

        if (index < pattern.size()) {
          const auto translated{
              translate_unicode_property(property_name, negated)};
          if (translated.has_value()) {
            result += translated.value();
          } else {
            result += pattern.substr(start_index, index - start_index + 1);
          }
        } else {
          index = start_index;
          result += pattern[index];
        }
        continue;
      }

      if (next_char == 'd') {
        result += in_char_class ? "0-9" : "[0-9]";
        ++index;
        continue;
      }

      if (next_char == 'D' && !in_char_class) {
        result += "[^0-9]";
        ++index;
        continue;
      }

      if (next_char == 'w') {
        result += in_char_class ? "a-zA-Z0-9_" : "[a-zA-Z0-9_]";
        ++index;
        continue;
      }

      if (next_char == 'W' && !in_char_class) {
        result += "[^a-zA-Z0-9_]";
        ++index;
        continue;
      }

      if (next_char == 's') {
        result +=
            in_char_class
                ? R"(\t\v\f \x{00A0}\x{FEFF}\p{Zs}\n\r\x{2028}\x{2029})"
                : R"([\t\v\f \x{00A0}\x{FEFF}\p{Zs}\n\r\x{2028}\x{2029}])";
        ++index;
        continue;
      }

      if (next_char == 'S' && !in_char_class) {
        result += R"([^\t\v\f \x{00A0}\x{FEFF}\p{Zs}\n\r\x{2028}\x{2029}])";
        ++index;
        continue;
      }

      if (next_char == 'b' && !in_char_class) {
        result +=
            R"((?:(?<![a-zA-Z0-9_])(?=[a-zA-Z0-9_])|(?<=[a-zA-Z0-9_])(?![a-zA-Z0-9_])))";
        ++index;
        continue;
      }

      if (next_char == 'B' && !in_char_class) {
        result +=
            R"((?:(?<=[a-zA-Z0-9_])(?=[a-zA-Z0-9_])|(?<![a-zA-Z0-9_])(?![a-zA-Z0-9_])))";
        ++index;
        continue;
      }
    }

    result += pattern[index];
  }

  return result;
}

} // namespace sourcemeta::core

#endif
