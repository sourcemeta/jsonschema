#ifndef SOURCEMETA_CORE_OAUTH_SCOPE_H_
#define SOURCEMETA_CORE_OAUTH_SCOPE_H_

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

// RFC 6749 Section 3.3: "The value of the scope parameter is expressed as a
// list of space-delimited, case-sensitive strings" whose order does not
// matter, so membership is an exact code point comparison of whole tokens,
// scanned without allocation. A scope token has at least one character, so an
// empty sought value is never a member
inline auto oauth_scope_contains(const std::string_view scope,
                                 const std::string_view value) -> bool {
  if (value.empty()) {
    return false;
  }

  std::size_t position{0};
  while (position < scope.size()) {
    const auto next{scope.find(' ', position)};
    const auto token{scope.substr(position, next == std::string_view::npos
                                                ? std::string_view::npos
                                                : next - position)};
    if (token == value) {
      return true;
    }

    if (next == std::string_view::npos) {
      break;
    }

    position = next + 1;
  }

  return false;
}

} // namespace sourcemeta::core

#endif
