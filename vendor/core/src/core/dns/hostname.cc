#include <sourcemeta/core/dns.h>
#include <sourcemeta/core/idna.h>
#include <sourcemeta/core/text.h>

#include <string_view> // std::string_view

namespace sourcemeta::core {

auto is_hostname_label(const std::string_view value) -> bool {
  // RFC 1035 §2.3.4: per-label cap is 63 octets
  if (value.empty() || value.size() > 63) {
    return false;
  }

  // RFC 1123 §2.1: first character must be let-dig, never hyphen, and RFC 952
  // §B ends a label in a let-dig too, where let-dig = ALPHA / DIGIT
  if (!is_alphanum(value.front()) || !is_alphanum(value.back())) {
    return false;
  }

  for (const auto character : value) {
    if (character != '-' && !is_alphanum(character)) {
      return false;
    }
  }

  return true;
}

auto is_hostname(const std::string_view value) -> bool {
  // RFC 952 §B: <hname> requires at least one <name>
  if (value.empty()) {
    return false;
  }

  // RFC 1123 §2.1: SHOULD handle host names of up to 255 characters. This is
  // intentionally looser than the stricter 253-octet cap applied to the
  // internationalized form
  if (value.size() > 255) {
    return false;
  }

  std::string_view remaining{value};
  while (true) {
    const auto dot{remaining.find('.')};
    const auto label{remaining.substr(0, dot)};
    if (!is_hostname_label(label)) {
      return false;
    }

    // RFC 5891 §5.3: an A-label starts in "xn--", interpreted
    // case-insensitively. A-labels must also satisfy RFC 5891 §4.2.3 and
    // RFC 5892 (Punycode round-trip, IDNA 2008 derived properties,
    // contextual rules)
    if (starts_with_ignore_case(label, "xn--") &&
        !idna_is_valid_a_label(label)) {
      return false;
    }

    if (dot == std::string_view::npos) {
      return true;
    }

    remaining.remove_prefix(dot + 1);
    // Trailing dot is not part of the host name grammar
    if (remaining.empty()) {
      return false;
    }
  }
}

} // namespace sourcemeta::core
