#ifndef SOURCEMETA_CORE_EMAIL_HELPERS_H_
#define SOURCEMETA_CORE_EMAIL_HELPERS_H_

#include <sourcemeta/core/ip.h>
#include <sourcemeta/core/text.h>

#include <cstdint>     // std::uint8_t, std::uint16_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// RFC 5321 §4.1.2: atext = ALPHA / DIGIT / "!" / "#" / "$" / "%" /
// "&" / "'" / "*" / "+" / "-" / "/" / "=" / "?" / "^" / "_" / "`" /
// "{" / "|" / "}" / "~"
constexpr auto is_atext(const char character) -> bool {
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
      return sourcemeta::core::is_alphanum(character);
  }
}

// RFC 5321 §4.1.2: qtextSMTP = %d32-33 / %d35-91 / %d93-126
constexpr auto is_qtext_smtp(const unsigned char character) -> bool {
  return (character >= 32 && character <= 33) ||
         (character >= 35 && character <= 91) ||
         (character >= 93 && character <= 126);
}

// RFC 5321 §4.1.3: dcontent = %d33-90 / %d94-126
constexpr auto is_dcontent(const unsigned char character) -> bool {
  return (character >= 33 && character <= 90) ||
         (character >= 94 && character <= 126);
}

// RFC 5321 §4.1.2: Ldh-str = *( ALPHA / DIGIT / "-" ) Let-dig
// RFC 5321 §4.1.3: Standardized-tag = Ldh-str
constexpr auto is_ldh_str(const std::string_view value) -> bool {
  if (value.empty() || !sourcemeta::core::is_alphanum(value.back())) {
    return false;
  }
  for (std::string_view::size_type position{0}; position + 1 < value.size();
       position += 1) {
    const auto character{value[position]};
    if (!sourcemeta::core::is_alphanum(character) && character != '-') {
      return false;
    }
  }
  return true;
}

// RFC 5321 §4.1.3: Snum = 1*3DIGIT ; representing a decimal integer
// value in the range 0 through 255. Leading zeros are permitted, unlike
// the RFC 3986 dec-octet that backs is_ipv4
constexpr auto is_snum(const std::string_view value) -> bool {
  if (value.empty() || value.size() > 3) {
    return false;
  }
  std::uint16_t result{0};
  for (const auto character : value) {
    if (character < '0' || character > '9') {
      return false;
    }
    result = static_cast<std::uint16_t>(
        (result * 10) + static_cast<std::uint16_t>(character - '0'));
  }
  return result <= 255;
}

// RFC 5321 §4.1.3: IPv4-address-literal = Snum 3("." Snum)
constexpr auto is_ipv4_address_literal(const std::string_view value) -> bool {
  std::string_view::size_type start{0};
  std::uint8_t octets{0};
  while (true) {
    const auto dot{value.find('.', start)};
    const auto octet{dot == std::string_view::npos
                         ? value.substr(start)
                         : value.substr(start, dot - start)};
    if (!is_snum(octet)) {
      return false;
    }
    octets = static_cast<std::uint8_t>(octets + 1);
    // A valid literal has exactly four octets, so stop before the counter could
    // wrap on a pathological run of segments
    if (octets > 4) {
      return false;
    }
    if (dot == std::string_view::npos) {
      break;
    }
    start = dot + 1;
  }
  return octets == 4;
}

// RFC 5234 §2.3: ABNF literal strings are case-insensitive by default
// RFC 5321 §4.1.3: IPv6-address-literal prefix is the literal "IPv6:"
constexpr auto matches_ipv6_tag(const std::string_view value) -> bool {
  return value.size() >= 5 && (value[0] == 'I' || value[0] == 'i') &&
         (value[1] == 'P' || value[1] == 'p') &&
         (value[2] == 'v' || value[2] == 'V') && value[3] == '6' &&
         value[4] == ':';
}

// RFC 5321 §4.1.3: General-address-literal = Standardized-tag ":" 1*dcontent
constexpr auto is_general_address_literal(const std::string_view value)
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
  if (!inner.contains(':')) {
    return is_ipv4_address_literal(inner);
  }
  return is_general_address_literal(inner);
}

// RFC 3986 §2.1: "For consistency, URI producers and normalizers should use
// uppercase hexadecimal digits for all percent-encodings"
inline auto percent_encode(const unsigned char byte, std::string &output)
    -> void {
  constexpr std::string_view HEXADECIMAL{"0123456789ABCDEF"};
  output.push_back('%');
  output.push_back(HEXADECIMAL[byte >> 4U]);
  output.push_back(HEXADECIMAL[byte & 0x0FU]);
}

// RFC 6068 §2: within addr-spec, the characters that cannot appear in a URI,
// plus "%", the gen-delims other than "@" and ":", and the sub-delims "&",
// ";", and "=" all MUST be percent-encoded. Erratum 7919 would lift the
// sub-delims mandate, but the §6.1 example encodes "Mike&family" as
// "Mike%26family", so the canonical spelling keeps encoding them. The "," is
// encoded as well because the "to" production takes it as the address list
// separator, and "@" inside quoted content is encoded following the §6.2
// example "%22not%40me%22"
constexpr auto is_mailto_verbatim(const char character) -> bool {
  switch (character) {
    case '!':
    case '$':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case '-':
    case '.':
    case ':':
    case '_':
    case '~':
      return true;
    default:
      return sourcemeta::core::is_alphanum(character);
  }
}

// RFC 7565 §7: userpart consists of unreserved, sub-delims, and pct-encoded,
// so those two literal sets pass through and every other octet is
// percent-encoded, as the §4 example does for "juliet@capulet.example"
constexpr auto is_acct_userpart_verbatim(const char character) -> bool {
  switch (character) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case '-':
    case '.':
    case ';':
    case '=':
    case '_':
    case '~':
      return true;
    default:
      return sourcemeta::core::is_alphanum(character);
  }
}

} // namespace

#endif
