#ifndef SOURCEMETA_CORE_URITEMPLATE_HELPERS_H_
#define SOURCEMETA_CORE_URITEMPLATE_HELPERS_H_

#include <sourcemeta/core/uritemplate.h>

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::void_t

namespace sourcemeta::core {

// Type traits to detect optional static members
template <typename T, typename = void> struct has_op : std::false_type {};
template <typename T>
struct has_op<T, std::void_t<decltype(T::op)>> : std::true_type {};

template <typename T, typename = void> struct has_prefix : std::false_type {};
template <typename T>
struct has_prefix<T, std::void_t<decltype(T::prefix)>> : std::true_type {};

template <typename T, typename = void>
struct has_empty_suffix : std::false_type {};
template <typename T>
struct has_empty_suffix<T, std::void_t<decltype(T::empty_suffix)>>
    : std::true_type {};

inline auto is_unreserved(const char character) -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9') || character == '-' ||
         character == '.' || character == '_' || character == '~';
}

inline auto is_reserved(const char character) -> bool {
  return character == ':' || character == '/' || character == '?' ||
         character == '#' || character == '[' || character == ']' ||
         character == '@' || character == '!' || character == '$' ||
         character == '&' || character == '\'' || character == '(' ||
         character == ')' || character == '*' || character == '+' ||
         character == ',' || character == ';' || character == '=';
}

inline auto is_hex(const char character) -> bool {
  return (character >= '0' && character <= '9') ||
         (character >= 'A' && character <= 'F') ||
         (character >= 'a' && character <= 'f');
}

static constexpr std::array<char, 16> HEX_DIGITS = {
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
     'F'}};

inline auto append_percent_encoded(std::string &output, const char character)
    -> void {
  const auto byte = static_cast<unsigned char>(character);
  output += '%';
  output += HEX_DIGITS[byte >> 4];
  output += HEX_DIGITS[byte & 0x0F];
}

inline auto percent_encode(std::string &output, const std::string_view input)
    -> void {
  output.reserve(output.size() + input.size() * 3);
  for (const char character : input) {
    if (is_unreserved(character)) {
      output += character;
    } else {
      append_percent_encoded(output, character);
    }
  }
}

inline auto percent_encode_reserved(std::string &output,
                                    const std::string_view input) -> void {
  output.reserve(output.size() + input.size() * 3);
  for (std::size_t index = 0; index < input.size(); ++index) {
    const char character = input[index];
    if (is_unreserved(character) || is_reserved(character) ||
        (character == '%' && index + 2 < input.size() &&
         is_hex(input[index + 1]) && is_hex(input[index + 2]))) {
      output += character;
    } else {
      append_percent_encoded(output, character);
    }
  }
}

template <typename T>
inline auto encode(std::string &output, const std::string_view input) -> void {
  if constexpr (T::allow_reserved) {
    percent_encode_reserved(output, input);
  } else {
    percent_encode(output, input);
  }
}

template <typename T>
inline auto append_name(std::string &result, const std::string_view name,
                        const bool value_empty, const bool has_more) -> void {
  if constexpr (T::named) {
    result += name;
    if (value_empty && !has_more) {
      if constexpr (has_empty_suffix<T>::value) {
        result += T::empty_suffix;
      }
    } else {
      result += '=';
    }
  }
}

// RFC 6570 Section 2.3: varchar = ALPHA / DIGIT / "_"
inline auto is_varchar(const char character) noexcept -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9') || character == '_';
}

// Variable name character including dot for dotted names like "foo.bar"
inline auto is_varname_char(const char character) noexcept -> bool {
  return is_varchar(character) || character == '.';
}

// RFC 6570 Section 2.2: operator = op-level2 / op-level3 / op-reserve
inline auto is_operator(const char character) noexcept -> bool {
  return character == '+' || character == '#' || character == '.' ||
         character == '/' || character == ';' || character == '?' ||
         character == '&';
}

// RFC 6570 Section 2.2: op-reserve = "=" / "," / "!" / "@" / "|"
inline auto is_reserved_operator(const char character) noexcept -> bool {
  return character == '=' || character == ',' || character == '!' ||
         character == '@' || character == '|';
}

// RFC 6570 Section 2.4: modifier = prefix / explode
inline auto is_modifier(const char character) noexcept -> bool {
  return character == ':' || character == '*';
}

inline auto parse_varname(const std::string_view input, std::size_t position)
    -> std::size_t {
  if (position >= input.size() ||
      (!is_varchar(input[position]) && input[position] != '%')) {
    throw URITemplateParseError(position + 1);
  }

  while (position < input.size() && input[position] != '}' &&
         input[position] != ',' && input[position] != ':' &&
         input[position] != '*') {
    const char character = input[position];

    if (is_varchar(character)) {
      position++;
    } else if (character == '.') {
      position++;
      if (position >= input.size() ||
          (!is_varchar(input[position]) && input[position] != '%')) {
        throw URITemplateParseError(position + 1);
      }
    } else if (character == '%') {
      if (position + 2 >= input.size()) {
        throw URITemplateParseError(position + 1);
      }
      if (!is_hex(input[position + 1]) || !is_hex(input[position + 2])) {
        throw URITemplateParseError(position + 1);
      }
      position += 3;
    } else {
      throw URITemplateParseError(position + 1);
    }
  }

  return position;
}

inline auto
parse_variable_list(const std::string_view input, std::size_t position,
                    std::vector<URITemplateVariableSpecification> &variables)
    -> std::size_t {
  while (true) {
    const auto start = position;
    position = parse_varname(input, position);

    if (position == start) {
      throw URITemplateParseError(position + 1);
    }

    const auto name = input.substr(start, position - start);
    std::uint16_t length = 0;
    bool explode = false;

    if (position >= input.size()) {
      throw URITemplateParseError(1);
    }

    if (input[position] == ':') {
      position++;
      if (position >= input.size() || input[position] < '1' ||
          input[position] > '9') {
        throw URITemplateParseError(position + 1);
      }

      const auto prefix_start = position;
      while (position < input.size() && input[position] >= '0' &&
             input[position] <= '9') {
        position++;
        if (position - prefix_start > 4) {
          throw URITemplateParseError(position);
        }
      }

      const auto prefix_str =
          input.substr(prefix_start, position - prefix_start);
      std::uint16_t value = 0;
      for (const char character : prefix_str) {
        value = static_cast<std::uint16_t>(
            value * 10 + static_cast<std::uint16_t>(character - '0'));
      }

      if (value > 9999 || value == 0) {
        throw URITemplateParseError(prefix_start + 1);
      }

      length = value;
    } else if (input[position] == '*') {
      explode = true;
      position++;
    }

    variables.push_back(URITemplateVariableSpecification{
        .name = name, .length = length, .explode = explode});

    if (position >= input.size()) {
      throw URITemplateParseError(1);
    }

    if (input[position] == '}') {
      break;
    }

    if (input[position] == ',') {
      position++;
    }
  }

  return position;
}

template <typename T>
auto parse_expression(const std::string_view input) -> URITemplateParseResult {
  if constexpr (std::is_same_v<T, URITemplateTokenLiteral>) {
    if (input.empty() || input[0] == '{') {
      return std::nullopt;
    }

    if (input[0] == '}') {
      throw URITemplateParseError(1);
    }

    std::size_t position = 1;
    while (position < input.size()) {
      if (input[position] == '{') {
        break;
      }
      if (input[position] == '}') {
        throw URITemplateParseError(position + 1);
      }
      position++;
    }

    return std::make_pair(
        URITemplateToken{URITemplateTokenLiteral{input.substr(0, position)}},
        position);
  } else {
    if (input.empty() || input[0] != '{') {
      return std::nullopt;
    }

    std::size_t var_start;
    if constexpr (has_op<T>::value) {
      if (input.size() < 3 || input[1] != T::op) {
        return std::nullopt;
      }
      var_start = 2;
    } else {
      if (input.size() < 2) {
        throw URITemplateParseError(1);
      }
      // Not a simple variable if it has an operator
      if (is_operator(input[1])) {
        return std::nullopt;
      }
      var_start = 1;
    }

    std::vector<URITemplateVariableSpecification> variables;
    const auto end_position = parse_variable_list(input, var_start, variables);
    return std::make_pair(URITemplateToken{T{std::move(variables)}},
                          end_position + 1);
  }
}

template <typename T>
auto expand_expression(
    std::string &result,
    const std::vector<URITemplateVariableSpecification> &variables,
    const std::function<URITemplateValue(std::string_view)> &callback) -> void {
  bool first_var = true;

  for (const auto &variable : variables) {
    auto response = callback(variable.name);
    if (!response.has_value()) {
      continue;
    }

    bool first_value = true;

    while (true) {
      const auto &[value, object_key, has_more] = response.value();

      if (variable.length > 0 &&
          (has_more || object_key.has_value() || !first_value)) {
        throw URITemplateExpansionError{
            "Prefix modifier cannot be applied to composite values"};
      }

      auto actual_value = value;
      if (variable.length > 0) {
        actual_value = actual_value.substr(0, variable.length);
      }

      if (variable.explode) {
        if (first_var && first_value) {
          if constexpr (has_prefix<T>::value) {
            result += T::prefix;
          }
          first_var = false;
        } else {
          result += T::separator;
        }

        if (object_key.has_value()) {
          encode<T>(result, object_key.value());
          result += '=';
          encode<T>(result, actual_value);
        } else if constexpr (T::named) {
          result += variable.name;
          if (actual_value.empty()) {
            if constexpr (has_empty_suffix<T>::value) {
              result += T::empty_suffix;
            }
          } else {
            result += '=';
            encode<T>(result, actual_value);
          }
        } else {
          encode<T>(result, actual_value);
        }
      } else {
        if (first_var && first_value) {
          if constexpr (has_prefix<T>::value) {
            result += T::prefix;
          }
          first_var = false;
          append_name<T>(result, variable.name, actual_value.empty(), has_more);
        } else if (first_value) {
          result += T::separator;
          append_name<T>(result, variable.name, actual_value.empty(), has_more);
        } else {
          result += ',';
        }

        if (!first_value || !actual_value.empty() || has_more) {
          if (object_key.has_value()) {
            encode<T>(result, object_key.value());
            result += ',';
          }
          encode<T>(result, actual_value);
        }
      }

      first_value = false;

      if (!has_more) {
        break;
      }

      response = callback(variable.name);
      if (!response.has_value()) {
        break;
      }
    }
  }
}

} // namespace sourcemeta::core

#endif
