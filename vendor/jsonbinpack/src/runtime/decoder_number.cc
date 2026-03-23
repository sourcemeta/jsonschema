#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include <cstdint> // std::int64_t, std::uint64_t

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC optimize("no-reciprocal-math")
#endif

namespace sourcemeta::jsonbinpack {

auto Decoder::DOUBLE_VARINT_TUPLE(const struct DOUBLE_VARINT_TUPLE &)
    -> sourcemeta::core::JSON {
#ifdef __clang__
#pragma clang fp reciprocal(off)
#endif
  const std::int64_t digits{this->get_varint_zigzag()};
  const std::uint64_t point{this->get_varint()};
  double divisor{1.0};
  for (std::uint64_t i = 0; i < point; ++i) {
    divisor *= 10.0;
  }
  return sourcemeta::core::JSON{static_cast<double>(digits) / divisor};
}

} // namespace sourcemeta::jsonbinpack
