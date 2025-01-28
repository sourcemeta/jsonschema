#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include <cmath>   // std::pow
#include <cstdint> // std::int64_t, std::uint64_t

namespace sourcemeta::jsonbinpack {

auto Decoder::DOUBLE_VARINT_TUPLE(const struct DOUBLE_VARINT_TUPLE &)
    -> sourcemeta::core::JSON {
  const std::int64_t digits{this->get_varint_zigzag()};
  const std::uint64_t point{this->get_varint()};
  const double divisor{std::pow(10, static_cast<double>(point))};
  return sourcemeta::core::JSON{static_cast<double>(digits) / divisor};
}

} // namespace sourcemeta::jsonbinpack
