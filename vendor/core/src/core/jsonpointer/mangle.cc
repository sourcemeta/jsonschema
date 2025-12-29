#include <sourcemeta/core/jsonpointer.h>

#include <cassert>     // assert
#include <iomanip>     // std::setfill, std::setw
#include <sstream>     // std::ostringstream
#include <string_view> // std::string_view
#include <variant>     // std::visit

namespace {

// Special characters
constexpr auto ESCAPE_PREFIX = 'X';
constexpr auto UPPERCASE_PREFIX = 'U';
constexpr auto SEPARATOR = '_';
constexpr auto HYPHEN = '-';

// Reserved characters that need escaping
constexpr auto RESERVED_X_UPPER = 'X';
constexpr auto RESERVED_X_LOWER = 'x';
constexpr auto RESERVED_U_UPPER = 'U';
constexpr auto RESERVED_U_LOWER = 'u';
constexpr auto RESERVED_Z_UPPER = 'Z';
constexpr auto RESERVED_Z_LOWER = 'z';

// Special token markers
constexpr std::string_view TOKEN_EMPTY = "ZEmpty";
constexpr std::string_view TOKEN_WILDCARD_PROPERTY = "ZAnyProperty";
constexpr std::string_view TOKEN_WILDCARD_ITEM = "ZAnyItem";
constexpr std::string_view TOKEN_WILDCARD_KEY = "ZAnyKey";
constexpr std::string_view TOKEN_CONDITION = "ZMaybe";
constexpr std::string_view TOKEN_NEGATION = "ZNot";
constexpr std::string_view TOKEN_REGEX = "ZRegex";

constexpr auto ASCII_MAX = static_cast<unsigned char>(0x80);

// Locale-independent ASCII character classification
inline auto is_ascii_alpha(unsigned char character) noexcept -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z');
}

inline auto is_ascii_digit(unsigned char character) noexcept -> bool {
  return character >= '0' && character <= '9';
}

inline auto is_ascii_lower(unsigned char character) noexcept -> bool {
  return character >= 'a' && character <= 'z';
}

inline auto to_ascii_upper(unsigned char character) noexcept -> char {
  if (character >= 'a' && character <= 'z') {
    return static_cast<char>(character - 'a' + 'A');
  }
  return static_cast<char>(character);
}

inline auto hex_escape(std::ostringstream &output, char character) noexcept
    -> void {
  output << ESCAPE_PREFIX << std::uppercase << std::hex << std::setfill('0')
         << std::setw(2)
         << static_cast<unsigned int>(static_cast<unsigned char>(character));
}

inline auto is_reserved_at_start(char character) noexcept -> bool {
  switch (character) {
    case RESERVED_X_UPPER:
    case RESERVED_X_LOWER:
    case RESERVED_U_UPPER:
    case RESERVED_U_LOWER:
    case RESERVED_Z_UPPER:
    case RESERVED_Z_LOWER:
      return true;
    default:
      return false;
  }
}

inline auto encode_prefix(std::ostringstream &output,
                          std::string_view input) noexcept -> void {
  bool capitalize_next{true};
  bool first{true};

  for (const auto character : input) {
    const auto unsigned_character{static_cast<unsigned char>(character)};

    if (is_ascii_alpha(unsigned_character)) {
      if (capitalize_next && is_ascii_lower(unsigned_character)) {
        output << to_ascii_upper(unsigned_character);
      } else {
        output << character;
      }
      capitalize_next = false;
    } else if (is_ascii_digit(unsigned_character)) {
      if (first) {
        output << SEPARATOR;
      }
      output << character;
      capitalize_next = false;
    } else if (character == SEPARATOR || character == HYPHEN) {
      capitalize_next = true;
    } else {
      hex_escape(output, character);
      capitalize_next = true;
    }

    first = false;
  }
}

inline auto encode_string(std::ostringstream &output,
                          const std::string &input) noexcept -> void {
  bool segment_start{true};

  for (const auto character : input) {
    const auto unsigned_character{static_cast<unsigned char>(character)};

    if (is_ascii_alpha(unsigned_character)) {
      const bool is_lower{is_ascii_lower(unsigned_character)};
      if (segment_start) {
        if (is_reserved_at_start(character)) {
          hex_escape(output, character);
        } else if (is_lower) {
          output << to_ascii_upper(unsigned_character);
        } else {
          output << UPPERCASE_PREFIX << character;
        }
      } else if (character == RESERVED_X_UPPER ||
                 character == RESERVED_U_UPPER) {
        hex_escape(output, character);
      } else {
        output << character;
      }
      segment_start = false;
    } else if (is_ascii_digit(unsigned_character)) {
      output << character;
      segment_start = false;
    } else {
      hex_escape(output, character);
      // Only ASCII non-alphanumeric starts a new segment
      // Non-ASCII bytes (>= 0x80) do not start new segments (UTF-8 handling)
      segment_start = (unsigned_character < ASCII_MAX);
    }
  }
}

inline auto encode_string_or_empty(std::ostringstream &output,
                                   const std::string &input) noexcept -> void {
  if (input.empty()) {
    output << TOKEN_EMPTY;
  } else {
    encode_string(output, input);
  }
}

class TokenVisitor {
public:
  explicit TokenVisitor(std::ostringstream &output) noexcept
      : output_{output} {}

  auto operator()(const sourcemeta::core::Pointer::Token &token) const noexcept
      -> void {
    this->output_ << SEPARATOR;
    encode_string_or_empty(this->output_, token.to_property());
  }

  auto operator()(const sourcemeta::core::PointerTemplate::Wildcard &wildcard)
      const noexcept -> void {
    this->output_ << SEPARATOR;
    switch (wildcard) {
      case sourcemeta::core::PointerTemplate::Wildcard::Property:
        this->output_ << TOKEN_WILDCARD_PROPERTY;
        break;
      case sourcemeta::core::PointerTemplate::Wildcard::Item:
        this->output_ << TOKEN_WILDCARD_ITEM;
        break;
      case sourcemeta::core::PointerTemplate::Wildcard::Key:
        this->output_ << TOKEN_WILDCARD_KEY;
        break;
    }
  }

  auto operator()(const sourcemeta::core::PointerTemplate::Condition &condition)
      const noexcept -> void {
    this->output_ << SEPARATOR << TOKEN_CONDITION;
    if (condition.suffix.has_value()) {
      encode_string_or_empty(this->output_, condition.suffix.value());
    }
  }

  auto
  operator()(const sourcemeta::core::PointerTemplate::Negation &) const noexcept
      -> void {
    this->output_ << SEPARATOR << TOKEN_NEGATION;
  }

  auto operator()(const sourcemeta::core::PointerTemplate::Regex &regex)
      const noexcept -> void {
    this->output_ << SEPARATOR << TOKEN_REGEX;
    encode_string_or_empty(this->output_, regex);
  }

private:
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  std::ostringstream &output_;
};

} // namespace

namespace sourcemeta::core {

auto mangle(const PointerTemplate &pointer, const std::string_view prefix)
    -> std::string {
  assert(!prefix.empty());
  std::ostringstream output;
  encode_prefix(output, prefix);
  for (const auto &token : pointer) {
    std::visit(TokenVisitor{output}, token);
  }
  return output.str();
}

} // namespace sourcemeta::core
