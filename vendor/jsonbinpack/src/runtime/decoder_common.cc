#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include "unreachable.h"

#include <cassert> // assert
#include <variant> // std::get

namespace sourcemeta::jsonbinpack {

Decoder::Decoder(Stream &input) : InputStream{input} {}

auto Decoder::read(const Encoding &encoding) -> sourcemeta::core::JSON {
  switch (encoding.index()) {
#define HANDLE_DECODING(index, name)                                           \
  case (index):                                                                \
    return this->name(std::get<sourcemeta::jsonbinpack::name>(encoding));
    HANDLE_DECODING(0, BOUNDED_MULTIPLE_8BITS_ENUM_FIXED)
    HANDLE_DECODING(1, FLOOR_MULTIPLE_ENUM_VARINT)
    HANDLE_DECODING(2, ROOF_MULTIPLE_MIRROR_ENUM_VARINT)
    HANDLE_DECODING(3, ARBITRARY_MULTIPLE_ZIGZAG_VARINT)
    HANDLE_DECODING(4, DOUBLE_VARINT_TUPLE)
    HANDLE_DECODING(5, BYTE_CHOICE_INDEX)
    HANDLE_DECODING(6, LARGE_CHOICE_INDEX)
    HANDLE_DECODING(7, TOP_LEVEL_BYTE_CHOICE_INDEX)
    HANDLE_DECODING(8, CONST_NONE)
    HANDLE_DECODING(9, ANY_PACKED_TYPE_TAG_BYTE_PREFIX)
    HANDLE_DECODING(10, UTF8_STRING_NO_LENGTH)
    HANDLE_DECODING(11, FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED)
    HANDLE_DECODING(12, ROOF_VARINT_PREFIX_UTF8_STRING_SHARED)
    HANDLE_DECODING(13, BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED)
    HANDLE_DECODING(14, RFC3339_DATE_INTEGER_TRIPLET)
    HANDLE_DECODING(15, PREFIX_VARINT_LENGTH_STRING_SHARED)
    HANDLE_DECODING(16, FIXED_TYPED_ARRAY)
    HANDLE_DECODING(17, BOUNDED_8BITS_TYPED_ARRAY)
    HANDLE_DECODING(18, FLOOR_TYPED_ARRAY)
    HANDLE_DECODING(19, ROOF_TYPED_ARRAY)
    HANDLE_DECODING(20, FIXED_TYPED_ARBITRARY_OBJECT)
    HANDLE_DECODING(21, VARINT_TYPED_ARBITRARY_OBJECT)
#undef HANDLE_DECODING
    default:
      // We should never get here. If so, it is definitely a bug
      unreachable();
  }
}

} // namespace sourcemeta::jsonbinpack
