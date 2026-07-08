#include <sourcemeta/core/dns.h>
#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <array>       // std::array
#include <charconv>    // std::to_chars
#include <chrono>      // std::chrono::seconds
#include <cstddef>     // std::size_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

auto is_token_character(const char character) noexcept -> bool {
  // RFC 9110 §5.6.2: tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" /
  // "-" / "." / "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
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
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
      return true;
    default:
      return sourcemeta::core::is_alphanum(character);
  }
}

auto is_cookie_octet(const char character) noexcept -> bool {
  // RFC 6265 §4.1.1: cookie-octet = %x21 / %x23-2B / %x2D-3A / %x3C-5B /
  // %x5D-7E, that is US-ASCII excluding CTLs, whitespace, DQUOTE, comma,
  // semicolon, and backslash
  const auto value{static_cast<unsigned char>(character)};
  return value == 0x21 || (value >= 0x23 && value <= 0x2B) ||
         (value >= 0x2D && value <= 0x3A) || (value >= 0x3C && value <= 0x5B) ||
         (value >= 0x5D && value <= 0x7E);
}

auto is_attribute_octet(const char character) noexcept -> bool {
  // RFC 6265bis §4.1.1: path-value and domain-value are made of av-octet =
  // %x20-3A / %x3C-7E, that is printable US-ASCII excluding the ";" delimiter,
  // control characters, DEL, and any non-ASCII byte
  const auto value{static_cast<unsigned char>(character)};
  return value >= 0x20 && value <= 0x7E && character != ';';
}

auto is_token(const std::string_view value) noexcept -> bool {
  if (value.empty()) {
    return false;
  }

  for (const auto character : value) {
    if (!is_token_character(character)) {
      return false;
    }
  }

  return true;
}

auto is_cookie_value(const std::string_view value) noexcept -> bool {
  // RFC 6265 §4.1.1: cookie-value is either bare cookie-octets or the same
  // wrapped in a matched pair of double quotes
  for (const auto character : sourcemeta::core::unquote(value, '"')) {
    if (!is_cookie_octet(character)) {
      return false;
    }
  }

  return true;
}

auto is_attribute_value(const std::string_view value) noexcept -> bool {
  for (const auto character : value) {
    if (!is_attribute_octet(character)) {
      return false;
    }
  }

  return true;
}

auto is_cookie_domain(const std::string_view value) -> bool {
  // RFC 6265 §4.1.2.3 lets a domain carry an ignorable leading dot, which is
  // dropped before the remainder is checked as an RFC 1123 host name
  const auto host{value.starts_with('.') ? value.substr(1) : value};
  return sourcemeta::core::is_hostname(host);
}

constexpr std::string_view COOKIE_PAIR_SEPARATOR{"="};
constexpr std::string_view COOKIE_ATTRIBUTE_SEPARATOR{"; "};
constexpr std::string_view COOKIE_DOMAIN{"Domain="};
constexpr std::string_view COOKIE_PATH{"Path="};
constexpr std::string_view COOKIE_MAX_AGE{"Max-Age="};
constexpr std::string_view COOKIE_SECURE{"Secure"};
constexpr std::string_view COOKIE_HTTP_ONLY{"HttpOnly"};
constexpr std::string_view COOKIE_SAME_SITE{"SameSite="};
constexpr std::string_view COOKIE_SAME_SITE_STRICT{"Strict"};
constexpr std::string_view COOKIE_SAME_SITE_LAX{"Lax"};
constexpr std::string_view COOKIE_SAME_SITE_NONE{"None"};

auto same_site_token(const sourcemeta::core::HTTPCookieSameSite value) noexcept
    -> std::string_view {
  switch (value) {
    case sourcemeta::core::HTTPCookieSameSite::Strict:
      return COOKIE_SAME_SITE_STRICT;
    case sourcemeta::core::HTTPCookieSameSite::Lax:
      return COOKIE_SAME_SITE_LAX;
    case sourcemeta::core::HTTPCookieSameSite::None:
      return COOKIE_SAME_SITE_NONE;
  }

  std::unreachable();
}

auto required_size(const sourcemeta::core::HTTPCookie &cookie,
                   const std::string_view max_age) noexcept -> std::size_t {
  std::size_t size{cookie.name.size() + COOKIE_PAIR_SEPARATOR.size() +
                   cookie.value.size()};
  if (cookie.domain.has_value()) {
    size += COOKIE_ATTRIBUTE_SEPARATOR.size() + COOKIE_DOMAIN.size() +
            cookie.domain->size();
  }
  if (cookie.path.has_value()) {
    size += COOKIE_ATTRIBUTE_SEPARATOR.size() + COOKIE_PATH.size() +
            cookie.path->size();
  }
  if (cookie.max_age.has_value()) {
    size += COOKIE_ATTRIBUTE_SEPARATOR.size() + COOKIE_MAX_AGE.size() +
            max_age.size();
  }
  if (cookie.secure) {
    size += COOKIE_ATTRIBUTE_SEPARATOR.size() + COOKIE_SECURE.size();
  }
  if (cookie.http_only) {
    size += COOKIE_ATTRIBUTE_SEPARATOR.size() + COOKIE_HTTP_ONLY.size();
  }
  if (cookie.same_site.has_value()) {
    size += COOKIE_ATTRIBUTE_SEPARATOR.size() + COOKIE_SAME_SITE.size() +
            same_site_token(*cookie.same_site).size();
  }
  return size;
}

} // namespace

namespace sourcemeta::core {

auto http_cookie_valid(const HTTPCookie &cookie) -> bool {
  if (!is_token(cookie.name) || !is_cookie_value(cookie.value)) {
    return false;
  }

  if (cookie.path.has_value() && !is_attribute_value(*cookie.path)) {
    return false;
  }

  if (cookie.domain.has_value() && !is_cookie_domain(*cookie.domain)) {
    return false;
  }

  // RFC 6265bis §4.1.1 defines the max age attribute value as one or more
  // digits, so a negative expiry cannot be serialised. Zero or a positive value
  // is valid, and zero is the canonical way to expire a cookie
  if (cookie.max_age.has_value() && cookie.max_age->count() < 0) {
    return false;
  }

  // RFC 6265bis §5.7 ignores a cookie whose same-site mode is none unless it is
  // also marked secure
  if (cookie.same_site == HTTPCookieSameSite::None && !cookie.secure) {
    return false;
  }

  // RFC 6265bis §4.1.3.1 requires a cookie whose name carries the secure prefix
  // to be marked secure. The prefix is matched case-insensitively because RFC
  // 6265bis §5.4 has user agents do so, and a mismatched cookie is dropped
  if (starts_with_ignore_case(cookie.name, "__Secure-") && !cookie.secure) {
    return false;
  }

  // RFC 6265bis §4.1.3.2 requires a cookie whose name carries the host prefix
  // to be marked secure, scoped to the root path, and free of a domain, again
  // matched case-insensitively to agree with the user agent
  if (starts_with_ignore_case(cookie.name, "__Host-") &&
      (!cookie.secure || cookie.domain.has_value() ||
       !cookie.path.has_value() || *cookie.path != "/")) {
    return false;
  }

  return true;
}

auto http_serialize_cookie(const HTTPCookie &cookie, std::string &out) -> bool {
  if (!http_cookie_valid(cookie)) {
    return false;
  }

  // Format the max age into a stack buffer, avoiding an intermediate heap
  // allocation. A non-negative representation never exceeds these digits
  std::array<char, std::numeric_limits<std::chrono::seconds::rep>::digits10 + 2>
      max_age_buffer;
  std::string_view max_age;
  if (cookie.max_age.has_value()) {
    const auto result{std::to_chars(
        max_age_buffer.data(), max_age_buffer.data() + max_age_buffer.size(),
        cookie.max_age->count())};
    max_age = std::string_view{
        max_age_buffer.data(),
        static_cast<std::size_t>(result.ptr - max_age_buffer.data())};
  }

  out.reserve(out.size() + required_size(cookie, max_age));
  out.append(cookie.name);
  out.append(COOKIE_PAIR_SEPARATOR);
  out.append(cookie.value);

  if (cookie.domain.has_value()) {
    out.append(COOKIE_ATTRIBUTE_SEPARATOR);
    out.append(COOKIE_DOMAIN);
    out.append(*cookie.domain);
  }

  if (cookie.path.has_value()) {
    out.append(COOKIE_ATTRIBUTE_SEPARATOR);
    out.append(COOKIE_PATH);
    out.append(*cookie.path);
  }

  if (cookie.max_age.has_value()) {
    out.append(COOKIE_ATTRIBUTE_SEPARATOR);
    out.append(COOKIE_MAX_AGE);
    out.append(max_age);
  }

  if (cookie.secure) {
    out.append(COOKIE_ATTRIBUTE_SEPARATOR);
    out.append(COOKIE_SECURE);
  }

  if (cookie.http_only) {
    out.append(COOKIE_ATTRIBUTE_SEPARATOR);
    out.append(COOKIE_HTTP_ONLY);
  }

  if (cookie.same_site.has_value()) {
    out.append(COOKIE_ATTRIBUTE_SEPARATOR);
    out.append(COOKIE_SAME_SITE);
    out.append(same_site_token(*cookie.same_site));
  }

  return true;
}

auto http_serialize_cookie(const HTTPCookie &cookie)
    -> std::optional<std::string> {
  std::string out;
  if (!http_serialize_cookie(cookie, out)) {
    return std::nullopt;
  }

  return out;
}

} // namespace sourcemeta::core
