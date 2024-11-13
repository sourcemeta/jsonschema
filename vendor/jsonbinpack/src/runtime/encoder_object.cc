#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <cassert> // assert

namespace sourcemeta::jsonbinpack {

auto Encoder::FIXED_TYPED_ARBITRARY_OBJECT(
    const sourcemeta::jsontoolkit::JSON &document,
    const struct FIXED_TYPED_ARBITRARY_OBJECT &options) -> void {
  assert(document.is_object());
  assert(document.size() == options.size);

  for (const auto &[key, value] : document.as_object()) {
    this->write(sourcemeta::jsontoolkit::JSON{key}, *(options.key_encoding));
    this->write(value, *(options.encoding));
  }
}

auto Encoder::VARINT_TYPED_ARBITRARY_OBJECT(
    const sourcemeta::jsontoolkit::JSON &document,
    const struct VARINT_TYPED_ARBITRARY_OBJECT &options) -> void {
  assert(document.is_object());
  const auto size{document.size()};
  this->put_varint(size);

  for (const auto &[key, value] : document.as_object()) {
    this->write(sourcemeta::jsontoolkit::JSON{key}, *(options.key_encoding));
    this->write(value, *(options.encoding));
  }
}

} // namespace sourcemeta::jsonbinpack
