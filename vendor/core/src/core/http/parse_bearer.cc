#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace {

auto is_b64token_character(const char character) noexcept -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') ||
         (character >= '0' && character <= '9') || character == '-' ||
         character == '.' || character == '_' || character == '~' ||
         character == '+' || character == '/';
}

// RFC 6750 §2.1: b64token = 1*( ALPHA / DIGIT / "-" / "." / "_" / "~" / "+" /
// "/" ) *"=". At least one character from the alphabet, followed by optional
// trailing padding.
auto is_b64token(const std::string_view token) noexcept -> bool {
  std::size_t position{0};
  while (position < token.size() && is_b64token_character(token[position])) {
    ++position;
  }

  if (position == 0) {
    return false;
  }

  while (position < token.size()) {
    if (token[position] != '=') {
      return false;
    }
    ++position;
  }

  return true;
}

} // namespace

namespace sourcemeta::core {

auto http_parse_bearer(const std::string_view authorization) noexcept
    -> std::string_view {
  constexpr std::string_view scheme{"bearer"};
  if (authorization.size() <= scheme.size() ||
      authorization[scheme.size()] != ' ') {
    return {};
  }

  if (!equals_ignore_case(http_subview(authorization, 0, scheme.size()),
                          scheme)) {
    return {};
  }

  const auto token{http_trim_trailing_ows(http_trim_leading_ows(
      http_subview(authorization, scheme.size() + 1,
                   authorization.size() - scheme.size() - 1)))};
  if (!is_b64token(token)) {
    return {};
  }

  return token;
}

} // namespace sourcemeta::core
