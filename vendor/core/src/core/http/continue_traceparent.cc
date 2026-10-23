#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::ranges::copy
#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string_view> // std::string_view

#include "entropy.h"

namespace sourcemeta::core {

auto http_continue_traceparent(const std::string_view value, const bool sampled)
    -> std::optional<std::array<char, 55>> {
  // The value that arrived is the only source for the trace, so one that does
  // not parse leaves nothing to carry on from
  if (!http_is_traceparent(value)) {
    return std::nullopt;
  }

  std::array<std::uint8_t, 8> parent{};
  http_draw_identifier(parent);

  // The revision is the one this library implements rather than the one that
  // was read, since a value is reconstructed "according to the highest version
  // of the specification known to the implementation", and whatever a later
  // one appended goes with it
  // https://www.w3.org/TR/trace-context/#versioning-of-traceparent
  std::array<char, 55> result{};
  result[0] = '0';
  result[1] = '0';
  result[2] = '-';

  // The trace carries on, so its identifier is the one that arrived
  std::ranges::copy(value.substr(3, 32), result.begin() + 3);
  result[35] = '-';

  // The operation is this one rather than the caller's, and drawing a fresh
  // identifier is also what lets the sampling decision differ from the one
  // that arrived: "The parent-id field MUST be set to a new value with the
  // sampled flag update"
  // https://www.w3.org/TR/trace-context/#mutating-the-traceparent-field
  bytes_to_hex(parent, std::span{result}.subspan(36, 16));
  result[52] = '-';
  result[53] = '0';
  result[54] = sampled ? '1' : '0';
  return result;
}

} // namespace sourcemeta::core
