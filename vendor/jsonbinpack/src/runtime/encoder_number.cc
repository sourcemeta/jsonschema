#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint64_t

namespace sourcemeta::jsonbinpack {

auto Encoder::DOUBLE_VARINT_TUPLE(const sourcemeta::core::JSON &document,
                                  const struct DOUBLE_VARINT_TUPLE &) -> void {
  assert(document.is_real());
  const auto value{document.to_real()};
  std::uint64_t point_position;
  const std::int64_t integral{
      real_digits<std::int64_t>(value, &point_position)};
  this->put_varint_zigzag(integral);
  this->put_varint(point_position);
}

} // namespace sourcemeta::jsonbinpack
