#include <sourcemeta/core/dns.h>

namespace sourcemeta::core {

// RFC 952 §B: let-dig = ALPHA / DIGIT
// RFC 1123 §2.1: first character of a label is letter or digit
static constexpr auto is_let_dig(const char character) -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9');
}

// RFC 952 §B: let-dig-hyp = ALPHA / DIGIT / "-"
static constexpr auto is_let_dig_hyp(const char character) -> bool {
  return is_let_dig(character) || character == '-';
}

auto is_hostname(const std::string_view value) -> bool {
  // RFC 952 §B: <hname> requires at least one <name>
  if (value.empty()) {
    return false;
  }

  // RFC 1123 §2.1: SHOULD handle host names of up to 255 characters
  if (value.size() > 255) {
    return false;
  }

  std::string_view::size_type position{0};

  while (position < value.size()) {
    const auto label_start{position};

    // RFC 1123 §2.1: first character is letter or digit
    if (!is_let_dig(value[position])) {
      return false;
    }
    position += 1;

    while (position < value.size() && value[position] != '.') {
      // RFC 952 §B: interior characters are let-dig-hyp
      if (!is_let_dig_hyp(value[position])) {
        return false;
      }
      position += 1;
    }

    const auto label_length{position - label_start};

    // RFC 1123 §2.1: MUST handle host names of up to 63 characters (per label)
    if (label_length > 63) {
      return false;
    }

    // RFC 952 §B + ASSUMPTIONS: last character must not be a minus sign
    if (value[position - 1] == '-') {
      return false;
    }

    // If we stopped on a dot, there must be another label following it
    if (position < value.size()) {
      // value[position] == '.'
      position += 1;
      // Trailing dot: JSON Schema test suite requires rejection (TS d7+ #15)
      if (position >= value.size()) {
        return false;
      }
    }
  }

  return true;
}

} // namespace sourcemeta::core
