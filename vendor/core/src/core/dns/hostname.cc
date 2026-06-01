#include <sourcemeta/core/dns.h>
#include <sourcemeta/core/idna.h>

#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// RFC 952 §B: let-dig = ALPHA / DIGIT
// RFC 1123 §2.1: first character of a label is letter or digit
static constexpr auto is_let_dig(const char character) -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9');
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
    bool last_was_hyphen{false};
    bool label_has_content{false};

    while (position < value.size() && value[position] != '.') {
      const auto character{value[position]};
      if (character == '-') {
        // RFC 1123 §2.1: first character must be let-dig, never hyphen
        if (!label_has_content) {
          return false;
        }
        last_was_hyphen = true;
        position += 1;
        label_has_content = true;
        continue;
      }

      if (is_let_dig(character)) {
        last_was_hyphen = false;
        position += 1;
        label_has_content = true;
        continue;
      }

      return false;
    }

    // RFC 1035 §2.3.4: per-label cap is 63 octets
    const auto label_length{position - label_start};
    if (label_length == 0 || label_length > 63 || last_was_hyphen) {
      return false;
    }

    // RFC 5890 §2.3.2.1: the ACE prefix "xn--" is case-insensitive. A-labels
    // must also satisfy RFC 5891 §4.2.3 and RFC 5892 (Punycode round-trip,
    // IDNA 2008 derived properties, contextual rules)
    if (label_length >= 4 && ((value[label_start] | 0x20) == 'x') &&
        ((value[label_start + 1] | 0x20) == 'n') &&
        value[label_start + 2] == '-' && value[label_start + 3] == '-') {
      std::string canonical{value.substr(label_start, label_length)};
      canonical[0] = 'x';
      canonical[1] = 'n';
      if (!idna_is_valid_a_label(canonical)) {
        return false;
      }
    }

    if (position < value.size()) {
      // value[position] == '.'
      position += 1;
      if (position == value.size()) {
        // Trailing dot is not part of the host name grammar
        return false;
      }
    }
  }

  return true;
}

} // namespace sourcemeta::core
