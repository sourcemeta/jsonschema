#ifndef SOURCEMETA_JSONBINPACK_PARSER_V1_H_
#define SOURCEMETA_JSONBINPACK_PARSER_V1_H_

#include <sourcemeta/jsonbinpack/runtime.h>
#include <sourcemeta/jsonbinpack/runtime_parser.h>
#include <sourcemeta/jsontoolkit/json.h>

#include <algorithm> // std::transform
#include <cassert>   // assert
#include <cstdint>   // std::uint64_t
#include <iterator>  // std::back_inserter
#include <utility>   // std::move
#include <vector>    // std::vector

namespace sourcemeta::jsonbinpack::v1 {

// Integers

auto BOUNDED_MULTIPLE_8BITS_ENUM_FIXED(
    const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("minimum"));
  assert(options.defines("maximum"));
  assert(options.defines("multiplier"));
  const auto &minimum{options.at("minimum")};
  const auto &maximum{options.at("maximum")};
  const auto &multiplier{options.at("multiplier")};
  assert(minimum.is_integer());
  assert(maximum.is_integer());
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::jsonbinpack::BOUNDED_MULTIPLE_8BITS_ENUM_FIXED{
      minimum.to_integer(), maximum.to_integer(),
      static_cast<std::uint64_t>(multiplier.to_integer())};
}

auto FLOOR_MULTIPLE_ENUM_VARINT(const sourcemeta::jsontoolkit::JSON &options)
    -> Plan {
  assert(options.defines("minimum"));
  assert(options.defines("multiplier"));
  const auto &minimum{options.at("minimum")};
  const auto &multiplier{options.at("multiplier")};
  assert(minimum.is_integer());
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::jsonbinpack::FLOOR_MULTIPLE_ENUM_VARINT{
      minimum.to_integer(),
      static_cast<std::uint64_t>(multiplier.to_integer())};
}

auto ROOF_MULTIPLE_MIRROR_ENUM_VARINT(
    const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("maximum"));
  assert(options.defines("multiplier"));
  const auto &maximum{options.at("maximum")};
  const auto &multiplier{options.at("multiplier")};
  assert(maximum.is_integer());
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::jsonbinpack::ROOF_MULTIPLE_MIRROR_ENUM_VARINT{
      maximum.to_integer(),
      static_cast<std::uint64_t>(multiplier.to_integer())};
}

auto ARBITRARY_MULTIPLE_ZIGZAG_VARINT(
    const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("multiplier"));
  const auto &multiplier{options.at("multiplier")};
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::jsonbinpack::ARBITRARY_MULTIPLE_ZIGZAG_VARINT{
      static_cast<std::uint64_t>(multiplier.to_integer())};
}

// Numbers

auto DOUBLE_VARINT_TUPLE(const sourcemeta::jsontoolkit::JSON &) -> Plan {
  return sourcemeta::jsonbinpack::DOUBLE_VARINT_TUPLE{};
}

// Enumerations

auto BYTE_CHOICE_INDEX(const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("choices"));
  const auto &choices{options.at("choices")};
  assert(choices.is_array());
  const auto &array{choices.as_array()};
  std::vector<sourcemeta::jsontoolkit::JSON> elements{array.cbegin(),
                                                      array.cend()};
  return sourcemeta::jsonbinpack::BYTE_CHOICE_INDEX({std::move(elements)});
}

auto LARGE_CHOICE_INDEX(const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("choices"));
  const auto &choices{options.at("choices")};
  assert(choices.is_array());
  const auto &array{choices.as_array()};
  std::vector<sourcemeta::jsontoolkit::JSON> elements{array.cbegin(),
                                                      array.cend()};
  return sourcemeta::jsonbinpack::LARGE_CHOICE_INDEX({std::move(elements)});
}

auto TOP_LEVEL_BYTE_CHOICE_INDEX(const sourcemeta::jsontoolkit::JSON &options)
    -> Plan {
  assert(options.defines("choices"));
  const auto &choices{options.at("choices")};
  assert(choices.is_array());
  const auto &array{choices.as_array()};
  std::vector<sourcemeta::jsontoolkit::JSON> elements{array.cbegin(),
                                                      array.cend()};
  return sourcemeta::jsonbinpack::TOP_LEVEL_BYTE_CHOICE_INDEX(
      {std::move(elements)});
}

auto CONST_NONE(const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("value"));
  return sourcemeta::jsonbinpack::CONST_NONE({options.at("value")});
}

// Strings

auto UTF8_STRING_NO_LENGTH(const sourcemeta::jsontoolkit::JSON &options)
    -> Plan {
  assert(options.defines("size"));
  const auto &size{options.at("size")};
  assert(size.is_integer());
  assert(size.is_positive());
  return sourcemeta::jsonbinpack::UTF8_STRING_NO_LENGTH{
      static_cast<std::uint64_t>(size.to_integer())};
}

auto FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("minimum"));
  const auto &minimum{options.at("minimum")};
  assert(minimum.is_integer());
  assert(minimum.is_positive());
  return sourcemeta::jsonbinpack::FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED{
      static_cast<std::uint64_t>(minimum.to_integer())};
}

auto ROOF_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("maximum"));
  const auto &maximum{options.at("maximum")};
  assert(maximum.is_integer());
  assert(maximum.is_positive());
  return sourcemeta::jsonbinpack::ROOF_VARINT_PREFIX_UTF8_STRING_SHARED{
      static_cast<std::uint64_t>(maximum.to_integer())};
}

auto BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("minimum"));
  assert(options.defines("maximum"));
  const auto &minimum{options.at("minimum")};
  const auto &maximum{options.at("maximum")};
  assert(minimum.is_integer());
  assert(maximum.is_integer());
  assert(minimum.is_positive());
  assert(maximum.is_positive());
  return sourcemeta::jsonbinpack::BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED{
      static_cast<std::uint64_t>(minimum.to_integer()),
      static_cast<std::uint64_t>(maximum.to_integer())};
}

auto RFC3339_DATE_INTEGER_TRIPLET(const sourcemeta::jsontoolkit::JSON &)
    -> Plan {
  return sourcemeta::jsonbinpack::RFC3339_DATE_INTEGER_TRIPLET{};
}

auto PREFIX_VARINT_LENGTH_STRING_SHARED(const sourcemeta::jsontoolkit::JSON &)
    -> Plan {
  return sourcemeta::jsonbinpack::PREFIX_VARINT_LENGTH_STRING_SHARED{};
}

// Arrays

auto FIXED_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("size"));
  assert(options.defines("encoding"));
  assert(options.defines("prefixEncodings"));
  const auto &size{options.at("size")};
  const auto &array_encoding{options.at("encoding")};
  const auto &prefix_encodings{options.at("prefixEncodings")};
  assert(size.is_integer());
  assert(size.is_positive());
  assert(array_encoding.is_object());
  assert(prefix_encodings.is_array());
  std::vector<Plan> encodings;
  std::transform(prefix_encodings.as_array().cbegin(),
                 prefix_encodings.as_array().cend(),
                 std::back_inserter(encodings),
                 [](const auto &element) { return parse(element); });
  assert(encodings.size() == prefix_encodings.size());
  return sourcemeta::jsonbinpack::FIXED_TYPED_ARRAY{
      static_cast<std::uint64_t>(size.to_integer()),
      wrap(parse(array_encoding)), wrap(encodings.begin(), encodings.end())};
}

auto BOUNDED_8BITS_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &options)
    -> Plan {
  assert(options.defines("minimum"));
  assert(options.defines("maximum"));
  assert(options.defines("encoding"));
  assert(options.defines("prefixEncodings"));
  const auto &minimum{options.at("minimum")};
  const auto &maximum{options.at("maximum")};
  const auto &array_encoding{options.at("encoding")};
  const auto &prefix_encodings{options.at("prefixEncodings")};
  assert(minimum.is_integer());
  assert(maximum.is_integer());
  assert(minimum.is_positive());
  assert(maximum.is_positive());
  assert(array_encoding.is_object());
  assert(prefix_encodings.is_array());
  std::vector<Plan> encodings;
  std::transform(prefix_encodings.as_array().cbegin(),
                 prefix_encodings.as_array().cend(),
                 std::back_inserter(encodings),
                 [](const auto &element) { return parse(element); });
  assert(encodings.size() == prefix_encodings.size());
  return sourcemeta::jsonbinpack::BOUNDED_8BITS_TYPED_ARRAY{
      static_cast<std::uint64_t>(minimum.to_integer()),
      static_cast<std::uint64_t>(maximum.to_integer()),
      wrap(parse(array_encoding)), wrap(encodings.begin(), encodings.end())};
}

auto FLOOR_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("minimum"));
  assert(options.defines("encoding"));
  assert(options.defines("prefixEncodings"));
  const auto &minimum{options.at("minimum")};
  const auto &array_encoding{options.at("encoding")};
  const auto &prefix_encodings{options.at("prefixEncodings")};
  assert(minimum.is_integer());
  assert(minimum.is_positive());
  assert(array_encoding.is_object());
  assert(prefix_encodings.is_array());
  std::vector<Plan> encodings;
  std::transform(prefix_encodings.as_array().cbegin(),
                 prefix_encodings.as_array().cend(),
                 std::back_inserter(encodings),
                 [](const auto &element) { return parse(element); });
  assert(encodings.size() == prefix_encodings.size());
  return sourcemeta::jsonbinpack::FLOOR_TYPED_ARRAY{
      static_cast<std::uint64_t>(minimum.to_integer()),
      wrap(parse(array_encoding)), wrap(encodings.begin(), encodings.end())};
}

auto ROOF_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &options) -> Plan {
  assert(options.defines("maximum"));
  assert(options.defines("encoding"));
  assert(options.defines("prefixEncodings"));
  const auto &maximum{options.at("maximum")};
  const auto &array_encoding{options.at("encoding")};
  const auto &prefix_encodings{options.at("prefixEncodings")};
  assert(maximum.is_integer());
  assert(maximum.is_positive());
  assert(array_encoding.is_object());
  assert(prefix_encodings.is_array());
  std::vector<Plan> encodings;
  std::transform(prefix_encodings.as_array().cbegin(),
                 prefix_encodings.as_array().cend(),
                 std::back_inserter(encodings),
                 [](const auto &element) { return parse(element); });
  assert(encodings.size() == prefix_encodings.size());
  return sourcemeta::jsonbinpack::ROOF_TYPED_ARRAY{
      static_cast<std::uint64_t>(maximum.to_integer()),
      wrap(parse(array_encoding)), wrap(encodings.begin(), encodings.end())};
}

// Any

auto ANY_PACKED_TYPE_TAG_BYTE_PREFIX(const sourcemeta::jsontoolkit::JSON &)
    -> Plan {
  return sourcemeta::jsonbinpack::ANY_PACKED_TYPE_TAG_BYTE_PREFIX{};
}

} // namespace sourcemeta::jsonbinpack::v1

#endif
