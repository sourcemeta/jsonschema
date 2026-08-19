#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_parse_bearer(const std::string_view authorization) noexcept
    -> std::string_view {
  constexpr std::string_view SCHEME{"bearer"};
  if (authorization.size() <= SCHEME.size() ||
      authorization[SCHEME.size()] != ' ') {
    return {};
  }

  if (!equals_ignore_case(http_subview(authorization, 0, SCHEME.size()),
                          SCHEME)) {
    return {};
  }

  const auto token{http_trim_trailing_ows(http_trim_leading_ows(
      http_subview(authorization, SCHEME.size() + 1,
                   authorization.size() - SCHEME.size() - 1)))};
  if (!http_is_b64token(token)) {
    return {};
  }

  return token;
}

} // namespace sourcemeta::core
