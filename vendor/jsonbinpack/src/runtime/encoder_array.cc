#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t
#include <utility> // std::move

namespace sourcemeta::jsonbinpack {

auto Encoder::FIXED_TYPED_ARRAY(const sourcemeta::core::JSON &document,
                                const struct FIXED_TYPED_ARRAY &options)
    -> void {
  assert(document.is_array());
  assert(document.size() == options.size);
  const auto prefix_encodings{options.prefix_encodings.size()};
  assert(prefix_encodings <= document.size());
  for (std::size_t index = 0; index < options.size; index++) {
    const Encoding &encoding{prefix_encodings > index
                                 ? options.prefix_encodings[index]
                                 : *(options.encoding)};
    this->write(document.at(index), encoding);
  }
}

auto Encoder::BOUNDED_8BITS_TYPED_ARRAY(
    const sourcemeta::core::JSON &document,
    const struct BOUNDED_8BITS_TYPED_ARRAY &options) -> void {
  assert(options.maximum >= options.minimum);
  const auto size{document.size()};
  assert(is_within(size, options.minimum, options.maximum));
  assert(is_byte(options.maximum - options.minimum));
  this->put_byte(static_cast<std::uint8_t>(size - options.minimum));
  this->FIXED_TYPED_ARRAY(document,
                          {.size = size,
                           .encoding = options.encoding,
                           .prefix_encodings = options.prefix_encodings});
}

auto Encoder::FLOOR_TYPED_ARRAY(const sourcemeta::core::JSON &document,
                                const struct FLOOR_TYPED_ARRAY &options)
    -> void {
  const auto size{document.size()};
  assert(size >= options.minimum);
  this->put_varint(size - options.minimum);
  this->FIXED_TYPED_ARRAY(document,
                          {.size = size,
                           .encoding = options.encoding,
                           .prefix_encodings = options.prefix_encodings});
}

auto Encoder::ROOF_TYPED_ARRAY(const sourcemeta::core::JSON &document,
                               const struct ROOF_TYPED_ARRAY &options) -> void {
  const auto size{document.size()};
  assert(size <= options.maximum);
  this->put_varint(options.maximum - size);
  this->FIXED_TYPED_ARRAY(document,
                          {.size = size,
                           .encoding = options.encoding,
                           .prefix_encodings = options.prefix_encodings});
}

} // namespace sourcemeta::jsonbinpack
