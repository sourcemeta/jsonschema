#ifndef SOURCEMETA_CORE_OAUTH_DECODE_H_
#define SOURCEMETA_CORE_OAUTH_DECODE_H_

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/uri.h>

#include <cassert>     // assert
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Decode one "application/x-www-form-urlencoded" value against the caller's
// arena following design convention 1: borrow from the input when it carries no
// escape, otherwise append the decoded bytes to the arena and view them there.
// The arena MUST be reserved up front to at least the total raw length of every
// value that will be decoded into it, since decoding only shrinks, so that no
// append reallocates and a prior borrowed view into it stays valid. Reserving
// the whole input length once satisfies this
inline auto oauth_form_decode_into(const std::string_view value,
                                   std::string &arena, std::string_view &result)
    -> bool {
  if (value.find_first_of("+%") == std::string_view::npos) {
    result = value;
    return true;
  }

  // The caller must have reserved enough headroom for the raw value, since
  // decoding only shrinks, so this append never reallocates and a prior
  // borrowed view into the arena stays valid. The assert catches a violation
  // loudly under test, and the guard fails closed rather than dangle a view if
  // the contract is ever broken in a release build
  assert(arena.capacity() - arena.size() >= value.size());
  if (arena.capacity() - arena.size() < value.size()) {
    return false;
  }

  const auto base{arena.size()};
  if (!URI::unescape_form(value, arena)) {
    return false;
  }

  result = std::string_view{arena}.substr(base);
  return true;
}

// Decode one "application/x-www-form-urlencoded" value into a wiping arena, for
// a request whose values are secret such as a token. Unlike the borrowing
// variant it always appends, so the decoded view lands in the wiping arena
// rather than the caller's buffer, and the value never aliases the arena. The
// arena MUST be reserved up front to at least the total raw length so no append
// reallocates and a prior view into it stays valid
inline auto oauth_form_decode_into_secure(const std::string_view value,
                                          SecureString &arena,
                                          std::string_view &result) -> bool {
  const auto base{arena.size()};
  if (!URI::unescape_form(value, arena)) {
    return false;
  }

  result = std::string_view{arena}.substr(base);
  return true;
}

} // namespace sourcemeta::core

#endif
