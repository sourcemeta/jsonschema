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

// RFC 8414 Section 2: an issuer is an https URL with a non-empty host (RFC 3986
// Section 3.2) and no query or fragment, its scheme matched by code points to
// reject a non-canonical case
inline auto oauth_is_issuer_identifier(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->scheme().has_value() &&
         uri->scheme().value() == "https" && uri->host().has_value() &&
         !uri->host().value().empty() && !uri->query().has_value() &&
         !uri->fragment().has_value();
}

// RFC 9728 Section 1.2 and RFC 8707 Section 2: a resource is an https URL with
// a non-empty host and no fragment, a query tolerated unlike an issuer
inline auto oauth_is_resource_identifier(const std::string_view value) -> bool {
  const auto uri{oauth_try_parse_uri(value)};
  return uri.has_value() && uri->scheme().has_value() &&
         uri->scheme().value() == "https" && uri->host().has_value() &&
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
