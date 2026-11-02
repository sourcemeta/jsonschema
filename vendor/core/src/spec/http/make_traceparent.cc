#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <array>   // std::array
#include <cstdint> // std::uint8_t
#include <span>    // std::span

#include "entropy.h"

namespace sourcemeta::core {

auto http_make_traceparent(const bool sampled) -> std::array<char, 55> {
  // Sixteen bytes identify the trace and eight the operation within it, and
  // neither may be all zeroes, so the two are drawn separately
  std::array<std::uint8_t, 16> trace{};
  std::array<std::uint8_t, 8> parent{};
  http_draw_identifier(trace);
  http_draw_identifier(parent);

  std::array<char, 55> result{};
  result[0] = '0';
  result[1] = '0';
  result[2] = '-';
  bytes_to_hex(trace, std::span{result}.subspan(3, 32));
  result[35] = '-';
  bytes_to_hex(parent, std::span{result}.subspan(36, 16));
  result[52] = '-';

  // Only the lowest bit carries a meaning, and the rest are left clear because
  // "Vendors MUST set those to zero"
  // https://www.w3.org/TR/trace-context/#other-flags
  result[53] = '0';
  result[54] = sampled ? '1' : '0';
  return result;
}

} // namespace sourcemeta::core
