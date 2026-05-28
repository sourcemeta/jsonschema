#ifndef SOURCEMETA_CORE_EMAIL_HELPERS_H_
#define SOURCEMETA_CORE_EMAIL_HELPERS_H_

#include <sourcemeta/core/ip.h>

#include <string_view> // std::string_view

namespace {

// RFC 5321 §4.1.2: atext = ALPHA / DIGIT / "!" / "#" / "$" / "%" /
// "&" / "'" / "*" / "+" / "-" / "/" / "=" / "?" / "^" / "_" / "`" /
// "{" / "|" / "}" / "~"
inline constexpr auto is_atext(const char character) -> bool {
  switch (character) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '/':
    case '=':
    case '?':
    case '^':
    case '_':
    case '`':
    case '{':
    case '|':
    case '}':
    case '~':
      return true;
    default:
      return (character >= 'A' && character <= 'Z') ||
             (character >= 'a' && character <= 'z') ||
             (character >= '0' && character <= '9');
  }
}

// RFC 5321 §4.1.2: qtextSMTP = %d32-33 / %d35-91 / %d93-126
inline constexpr auto is_qtext_smtp(const unsigned char character) -> bool {
  return (character >= 32 && character <= 33) ||
         (character >= 35 && character <= 91) ||
         (character >= 93 && character <= 126);
}

// RFC 5321 §4.1.2: Let-dig = ALPHA / DIGIT
inline constexpr auto is_let_dig(const char character) -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9');
}

// RFC 5321 §4.1.3: dcontent = %d33-90 / %d94-126
inline constexpr auto is_dcontent(const unsigned char character) -> bool {
  return (character >= 33 && character <= 90) ||
         (character >= 94 && character <= 126);
}

// RFC 5321 §4.1.2: Ldh-str = *( ALPHA / DIGIT / "-" ) Let-dig
// RFC 5321 §4.1.3: Standardized-tag = Ldh-str
inline constexpr auto is_ldh_str(const std::string_view value) -> bool {
  if (value.empty() || !is_let_dig(value.back())) {
    return false;
  }
  for (std::string_view::size_type position{0}; position + 1 < value.size();
       position += 1) {
    const auto character{value[position]};
    if (!is_let_dig(character) && character != '-') {
      return false;
    }
  }
  return true;
}

// RFC 5234 §2.3: ABNF literal strings are case-insensitive by default
// RFC 5321 §4.1.3: IPv6-address-literal prefix is the literal "IPv6:"
inline constexpr auto matches_ipv6_tag(const std::string_view value) -> bool {
  return value.size() >= 5 && (value[0] == 'I' || value[0] == 'i') &&
         (value[1] == 'P' || value[1] == 'p') &&
         (value[2] == 'v' || value[2] == 'V') && value[3] == '6' &&
         value[4] == ':';
}

// RFC 5321 §4.1.3: General-address-literal = Standardized-tag ":" 1*dcontent
inline constexpr auto is_general_address_literal(const std::string_view value)
    -> bool {
  const auto colon_position{value.find(':')};
  if (colon_position == std::string_view::npos) {
    return false;
  }
  if (!is_ldh_str(value.substr(0, colon_position))) {
    return false;
  }
  const auto content{value.substr(colon_position + 1)};
  if (content.empty()) {
    return false;
  }
  for (const auto character : content) {
    if (!is_dcontent(static_cast<unsigned char>(character))) {
      return false;
    }
  }
  return true;
}

// RFC 5321 §4.1.3: validate the address-literal payload (between "[" and "]")
// as IPv6, IPv4, or General-address-literal. Always ASCII; no IDNA applies
inline auto is_address_literal(const std::string_view domain) -> bool {
  if (domain.back() != ']') {
    return false;
  }
  // RFC 5321 §4.5.3.1.2: 255-octet cap on a domain "name or number"
  if (domain.size() > 255) {
    return false;
  }
  const auto inner{domain.substr(1, domain.size() - 2)};
  // RFC 5321 §4.1.3: IPv6-address-literal = "IPv6:" IPv6-addr
  if (matches_ipv6_tag(inner) && sourcemeta::core::is_ipv6(inner.substr(5))) {
    return true;
  }
  // RFC 5234 §3.2: ABNF alternatives are unordered. A failed IPv6 match
  // falls through to IPv4 or General-address-literal.
  // RFC 5321 §4.1.3: IPv4-address-literal has no ":";
  // General-address-literal requires ":"
  if (inner.find(':') == std::string_view::npos) {
    return sourcemeta::core::is_ipv4(inner);
  }
  return is_general_address_literal(inner);
}

} // namespace

#endif
