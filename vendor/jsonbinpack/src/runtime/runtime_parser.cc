#include <sourcemeta/jsonbinpack/runtime_parser.h>

#include "runtime_parser_v1.h"

#include <cassert>   // assert
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error

namespace sourcemeta::jsonbinpack {

auto parse(const sourcemeta::jsontoolkit::JSON &input) -> Plan {
  assert(input.defines("binpackEncoding"));
  assert(input.defines("binpackOptions"));
  const auto encoding{input.at("binpackEncoding").to_string()};
  const auto &options{input.at("binpackOptions")};

#define PARSE_ENCODING(version, name)                                          \
  if (encoding == #name)                                                       \
    return version::name(options);

  // Integers
  PARSE_ENCODING(v1, BOUNDED_MULTIPLE_8BITS_ENUM_FIXED)
  PARSE_ENCODING(v1, FLOOR_MULTIPLE_ENUM_VARINT)
  PARSE_ENCODING(v1, ROOF_MULTIPLE_MIRROR_ENUM_VARINT)
  PARSE_ENCODING(v1, ARBITRARY_MULTIPLE_ZIGZAG_VARINT)
  // Numbers
  PARSE_ENCODING(v1, DOUBLE_VARINT_TUPLE)
  // Enumerations
  PARSE_ENCODING(v1, BYTE_CHOICE_INDEX)
  PARSE_ENCODING(v1, LARGE_CHOICE_INDEX)
  PARSE_ENCODING(v1, TOP_LEVEL_BYTE_CHOICE_INDEX)
  PARSE_ENCODING(v1, CONST_NONE)
  // Strings
  PARSE_ENCODING(v1, UTF8_STRING_NO_LENGTH)
  PARSE_ENCODING(v1, FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED)
  PARSE_ENCODING(v1, ROOF_VARINT_PREFIX_UTF8_STRING_SHARED)
  PARSE_ENCODING(v1, BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED)
  PARSE_ENCODING(v1, RFC3339_DATE_INTEGER_TRIPLET)
  PARSE_ENCODING(v1, PREFIX_VARINT_LENGTH_STRING_SHARED)
  // Arrays
  PARSE_ENCODING(v1, FIXED_TYPED_ARRAY)
  PARSE_ENCODING(v1, BOUNDED_8BITS_TYPED_ARRAY)
  PARSE_ENCODING(v1, FLOOR_TYPED_ARRAY)
  PARSE_ENCODING(v1, ROOF_TYPED_ARRAY)
  // Any
  PARSE_ENCODING(v1, ANY_PACKED_TYPE_TAG_BYTE_PREFIX)

#undef PARSE_ENCODING

  // TODO: Have a custom error for this
  std::ostringstream error;
  error << "Unrecognized encoding: " << encoding;
  throw std::runtime_error(error.str());
}

} // namespace sourcemeta::jsonbinpack
