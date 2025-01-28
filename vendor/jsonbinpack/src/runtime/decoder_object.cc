#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint64_t

namespace sourcemeta::jsonbinpack {

auto Decoder::FIXED_TYPED_ARBITRARY_OBJECT(
    const struct FIXED_TYPED_ARBITRARY_OBJECT &options)
    -> sourcemeta::core::JSON {
  sourcemeta::core::JSON document = sourcemeta::core::JSON::make_object();
  for (std::size_t index = 0; index < options.size; index++) {
    const sourcemeta::core::JSON key = this->read(*(options.key_encoding));
    assert(key.is_string());
    document.assign(key.to_string(), this->read(*(options.encoding)));
  }

  assert(document.size() == options.size);
  return document;
};

auto Decoder::VARINT_TYPED_ARBITRARY_OBJECT(
    const struct VARINT_TYPED_ARBITRARY_OBJECT &options)
    -> sourcemeta::core::JSON {
  const std::uint64_t size{this->get_varint()};
  sourcemeta::core::JSON document = sourcemeta::core::JSON::make_object();
  for (std::size_t index = 0; index < size; index++) {
    const sourcemeta::core::JSON key = this->read(*(options.key_encoding));
    assert(key.is_string());
    document.assign(key.to_string(), this->read(*(options.encoding)));
  }

  assert(document.size() == size);
  return document;
};

} // namespace sourcemeta::jsonbinpack
