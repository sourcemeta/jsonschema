#ifndef SOURCEMETA_CORE_OAUTH_SYNTAX_H_
#define SOURCEMETA_CORE_OAUTH_SYNTAX_H_

#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>   // std::ranges::all_of
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

// URI::is_uri validates the RFC 3986 grammar without constructing, but leaves
// the port unbounded while construction rejects a port above 32 bits, so the
// construction can still throw and is guarded rather than assumed to succeed
inline auto oauth_try_parse_uri(const std::string_view value)
    -> std::optional<URI> {
  if (!URI::is_uri(value)) {
    return std::nullopt;
  }

  try {
    return URI{value};
  } catch (const URIParseError &) {
    return std::nullopt;
  }
}

// An issuer identifier a document advertises for someone else, rather than the
// one the document was retrieved for. RFC 8414 Section 2 gives it the same
// shape, but Section 4 scopes code-point comparison to "comparing values in the
// messages to known values", and an advertised issuer is matched against
// nothing at parse time. Its validity therefore follows RFC 3986 Section 3.1,
// which makes the scheme case-insensitive
inline auto oauth_is_advertised_issuer(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->is_https() && uri->host().has_value() &&
         !uri->host().value().empty() && !uri->query().has_value() &&
         !uri->fragment().has_value();
}

// A resource identifier a document advertises for someone else, given the same
// case-insensitive scheme treatment as an advertised issuer, with a query
// tolerated per RFC 9728 Section 1.2 and RFC 8707 Section 2
inline auto oauth_is_advertised_resource(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->is_https() && uri->host().has_value() &&
         !uri->host().value().empty() && !uri->fragment().has_value();
}

// RFC 3986 Section 2.3: "unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"",
// the character set RFC 7636 reuses for the PKCE verifier and challenge
inline auto oauth_is_unreserved(const char character) noexcept -> bool {
  return is_alphanum(character) || character == '-' || character == '.' ||
         character == '_' || character == '~';
}

inline auto oauth_is_unreserved_string(const std::string_view value) noexcept
    -> bool {
  return std::ranges::all_of(value, oauth_is_unreserved);
}

// RFC 7636 Section 4.1: "code-verifier = 43*128unreserved"
inline auto oauth_is_pkce_verifier(const std::string_view value) noexcept
    -> bool {
  return value.size() >= 43 && value.size() <= 128 &&
         oauth_is_unreserved_string(value);
}

// RFC 7636 Section 4.2: "code-challenge = 43*128unreserved"
inline auto oauth_is_pkce_challenge(const std::string_view value) noexcept
    -> bool {
  return value.size() >= 43 && value.size() <= 128 &&
         oauth_is_unreserved_string(value);
}

} // namespace sourcemeta::core

#endif
