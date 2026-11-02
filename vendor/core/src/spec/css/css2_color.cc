#include <sourcemeta/core/css.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/text.h>

#include <array>   // std::array
#include <cassert> // assert
#include <cstdint> // std::uint8_t

namespace {

// CSS Core Syntax whitespace: U+0009, U+000A, U+000C, U+000D, U+0020
constexpr auto is_css_whitespace(const char character) noexcept -> bool {
  return character == ' ' || character == '\t' || character == '\n' ||
         character == '\r' || character == '\f';
}

constexpr auto equals_ascii_ci(const std::string_view left,
                               const std::string_view right) noexcept -> bool {
  assert(left.size() == right.size());
  for (std::string_view::size_type position{0}; position < left.size();
       position += 1) {
    if (sourcemeta::core::to_lowercase(left[position]) != right[position]) {
      return false;
    }
  }
  return true;
}

constexpr std::array<std::string_view, 17> CSS2_KEYWORDS{
    {"aqua", "black", "blue", "fuchsia", "gray", "green", "lime", "maroon",
     "navy", "olive", "orange", "purple", "red", "silver", "teal", "white",
     "yellow"}};

auto skip_whitespace(const std::string_view value,
                     std::string_view::size_type &position) noexcept -> void {
  while (position < value.size() && is_css_whitespace(value[position])) {
    position += 1;
  }
}

auto match_literal_ci(const std::string_view value,
                      std::string_view::size_type &position,
                      const std::string_view literal) noexcept -> bool {
  if (position + literal.size() > value.size()) {
    return false;
  }
  for (std::string_view::size_type index{0}; index < literal.size();
       index += 1) {
    if (sourcemeta::core::to_lowercase(value[position + index]) !=
        literal[index]) {
      return false;
    }
  }
  position += literal.size();
  return true;
}

auto match_byte(const std::string_view value,
                std::string_view::size_type &position,
                const char expected) noexcept -> bool {
  if (position >= value.size() || value[position] != expected) {
    return false;
  }
  position += 1;
  return true;
}

auto parse_number(const std::string_view value,
                  std::string_view::size_type &position,
                  bool &has_decimal) noexcept -> bool {
  const auto start{position};
  has_decimal = false;

  if (position < value.size() &&
      (value[position] == '+' || value[position] == '-')) {
    position += 1;
  }

  const auto integer_start{position};
  while (position < value.size() &&
         sourcemeta::core::is_digit(value[position])) {
    position += 1;
  }
  const auto integer_digits{position - integer_start};

  if (position < value.size() && value[position] == '.') {
    has_decimal = true;
    position += 1;
    const auto fractional_start{position};
    while (position < value.size() &&
           sourcemeta::core::is_digit(value[position])) {
      position += 1;
    }
    const auto fractional_digits{position - fractional_start};
    if (fractional_digits == 0) {
      position = start;
      return false;
    }
  } else if (integer_digits == 0) {
    position = start;
    return false;
  }

  return true;
}

enum class RgbValueKind : std::uint8_t { Integer, Percentage };

auto parse_value(const std::string_view value,
                 std::string_view::size_type &position,
                 RgbValueKind &kind) noexcept -> bool {
  const auto start{position};
  bool has_decimal{false};
  if (!parse_number(value, position, has_decimal)) {
    return false;
  }

  if (position < value.size() && value[position] == '%') {
    position += 1;
    kind = RgbValueKind::Percentage;
    return true;
  }

  if (has_decimal) {
    position = start;
    return false;
  }

  kind = RgbValueKind::Integer;
  return true;
}

} // namespace

namespace sourcemeta::core {

auto is_css2_hex_color(const std::string_view value) noexcept -> bool {
  if (value.size() != 4 && value.size() != 7) {
    return false;
  }

  if (value[0] != '#') {
    return false;
  }

  for (std::string_view::size_type position{1}; position < value.size();
       position += 1) {
    if (!sourcemeta::core::is_hex_digit(value[position])) {
      return false;
    }
  }

  return true;
}

auto is_css2_color_keyword(const std::string_view value) noexcept -> bool {
  if (value.size() < 3 || value.size() > 7) {
    return false;
  }

  for (const auto &keyword : CSS2_KEYWORDS) {
    if (keyword.size() != value.size()) {
      continue;
    }
    if (equals_ascii_ci(value, keyword)) {
      return true;
    }
  }

  return false;
}

auto is_css2_rgb_function(const std::string_view value) noexcept -> bool {
  std::string_view::size_type position{0};

  // Per CSS 2.1, the function-token is `IDENT(` with no whitespace between
  // the identifier and the opening paren, and the `<color>` value itself
  // does not include surrounding whitespace
  if (!match_literal_ci(value, position, "rgb")) {
    return false;
  }

  if (!match_byte(value, position, '(')) {
    return false;
  }

  skip_whitespace(value, position);

  RgbValueKind first_kind{};
  if (!parse_value(value, position, first_kind)) {
    return false;
  }

  skip_whitespace(value, position);
  if (!match_byte(value, position, ',')) {
    return false;
  }
  skip_whitespace(value, position);

  RgbValueKind second_kind{};
  if (!parse_value(value, position, second_kind)) {
    return false;
  }
  if (second_kind != first_kind) {
    return false;
  }

  skip_whitespace(value, position);
  if (!match_byte(value, position, ',')) {
    return false;
  }
  skip_whitespace(value, position);

  RgbValueKind third_kind{};
  if (!parse_value(value, position, third_kind)) {
    return false;
  }
  if (third_kind != first_kind) {
    return false;
  }

  skip_whitespace(value, position);
  if (!match_byte(value, position, ')')) {
    return false;
  }

  return position == value.size();
}

auto is_css2_color(const std::string_view value) noexcept -> bool {
  return is_css2_hex_color(value) || is_css2_color_keyword(value) ||
         is_css2_rgb_function(value);
}

} // namespace sourcemeta::core
