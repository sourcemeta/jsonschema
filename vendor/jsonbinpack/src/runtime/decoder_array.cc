#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint64_t
#include <utility> // std::move

namespace sourcemeta::jsonbinpack {

auto Decoder::FIXED_TYPED_ARRAY(const struct FIXED_TYPED_ARRAY &options)
    -> sourcemeta::core::JSON {
  const auto prefix_encodings{options.prefix_encodings.size()};
  sourcemeta::core::JSON result = sourcemeta::core::JSON::make_array();
  for (std::size_t index = 0; index < options.size; index++) {
    const Encoding &encoding{prefix_encodings > index
                                 ? options.prefix_encodings[index]
                                 : *(options.encoding)};
    result.push_back(this->read(encoding));
  }

  assert(result.size() == options.size);
  return result;
};

auto Decoder::BOUNDED_8BITS_TYPED_ARRAY(
    const struct BOUNDED_8BITS_TYPED_ARRAY &options) -> sourcemeta::core::JSON {
  assert(options.maximum >= options.minimum);
  assert(is_byte(options.maximum - options.minimum));
  const std::uint8_t byte{this->get_byte()};
  const std::uint64_t size{byte + options.minimum};
  assert(is_within(size, options.minimum, options.maximum));
  return this->FIXED_TYPED_ARRAY(
      {size, std::move(options.encoding), std::move(options.prefix_encodings)});
};

auto Decoder::FLOOR_TYPED_ARRAY(const struct FLOOR_TYPED_ARRAY &options)
    -> sourcemeta::core::JSON {
  const std::uint64_t value{this->get_varint()};
  const std::uint64_t size{value + options.minimum};
  assert(size >= value);
  assert(size >= options.minimum);
  return this->FIXED_TYPED_ARRAY(
      {size, std::move(options.encoding), std::move(options.prefix_encodings)});
};

auto Decoder::ROOF_TYPED_ARRAY(const struct ROOF_TYPED_ARRAY &options)
    -> sourcemeta::core::JSON {
  const std::uint64_t value{this->get_varint()};
  const std::uint64_t size{options.maximum - value};
  assert(size <= options.maximum);
  return this->FIXED_TYPED_ARRAY(
      {size, std::move(options.encoding), std::move(options.prefix_encodings)});
};

} // namespace sourcemeta::jsonbinpack
