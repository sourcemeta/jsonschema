#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <cassert> // assert

namespace sourcemeta::jsonbinpack {

auto Encoder::FIXED_TYPED_ARBITRARY_OBJECT(
    const sourcemeta::core::JSON &document,
    const struct FIXED_TYPED_ARBITRARY_OBJECT &options) -> void {
  assert(document.is_object());
  assert(document.size() == options.size);

  for (const auto &entry : document.as_object()) {
    this->write(sourcemeta::core::JSON{entry.first}, *(options.key_encoding));
    this->write(entry.second, *(options.encoding));
  }
}

auto Encoder::VARINT_TYPED_ARBITRARY_OBJECT(
    const sourcemeta::core::JSON &document,
    const struct VARINT_TYPED_ARBITRARY_OBJECT &options) -> void {
  assert(document.is_object());
  const auto size{document.size()};
  this->put_varint(size);

  for (const auto &entry : document.as_object()) {
    this->write(sourcemeta::core::JSON{entry.first}, *(options.key_encoding));
    this->write(entry.second, *(options.encoding));
  }
}

} // namespace sourcemeta::jsonbinpack
