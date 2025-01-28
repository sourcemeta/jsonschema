#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include "unreachable.h"

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint64_t
#include <memory>  // std::make_shared

namespace sourcemeta::jsonbinpack {

auto Decoder::BYTE_CHOICE_INDEX(const struct BYTE_CHOICE_INDEX &options)
    -> sourcemeta::core::JSON {
  assert(!options.choices.empty());
  assert(is_byte(options.choices.size()));
  const std::uint8_t index{this->get_byte()};
  assert(options.choices.size() > index);
  return options.choices[index];
}

auto Decoder::LARGE_CHOICE_INDEX(const struct LARGE_CHOICE_INDEX &options)
    -> sourcemeta::core::JSON {
  assert(!options.choices.empty());
  const std::uint64_t index{this->get_varint()};
  assert(options.choices.size() > index);
  return options.choices[index];
}

auto Decoder::TOP_LEVEL_BYTE_CHOICE_INDEX(
    const struct TOP_LEVEL_BYTE_CHOICE_INDEX &options)
    -> sourcemeta::core::JSON {
  assert(!options.choices.empty());
  assert(is_byte(options.choices.size()));
  if (!this->has_more_data()) {
    return options.choices.front();
  } else {
    const std::uint16_t index{static_cast<std::uint16_t>(this->get_byte() + 1)};
    assert(options.choices.size() > index);
    return options.choices[index];
  }
}

auto Decoder::CONST_NONE(const struct CONST_NONE &options)
    -> sourcemeta::core::JSON {
  return options.value;
}

auto Decoder::ANY_PACKED_TYPE_TAG_BYTE_PREFIX(
    const struct ANY_PACKED_TYPE_TAG_BYTE_PREFIX &) -> sourcemeta::core::JSON {
  using namespace internal::ANY_PACKED_TYPE_TAG_BYTE_PREFIX;
  const std::uint8_t byte{this->get_byte()};
  const std::uint8_t type{
      static_cast<std::uint8_t>(byte & (0xff >> subtype_size))};
  const std::uint8_t subtype{static_cast<std::uint8_t>(byte >> type_size)};

  if (type == TYPE_OTHER) {
    switch (subtype) {
      case SUBTYPE_NULL:
        return sourcemeta::core::JSON{nullptr};
      case SUBTYPE_FALSE:
        return sourcemeta::core::JSON{false};
      case SUBTYPE_TRUE:
        return sourcemeta::core::JSON{true};
      case SUBTYPE_NUMBER:
        return this->DOUBLE_VARINT_TUPLE({});
      case SUBTYPE_POSITIVE_REAL_INTEGER_BYTE:
        return sourcemeta::core::JSON{static_cast<double>(this->get_byte())};
      case SUBTYPE_POSITIVE_INTEGER:
        return sourcemeta::core::JSON{
            static_cast<std::int64_t>(this->get_varint())};
      case SUBTYPE_NEGATIVE_INTEGER:
        return sourcemeta::core::JSON{
            -static_cast<std::int64_t>(this->get_varint()) - 1};
      case SUBTYPE_LONG_STRING_BASE_EXPONENT_7:
        return sourcemeta::core::JSON{
            this->get_string_utf8(this->get_varint() + 128)};
      case SUBTYPE_LONG_STRING_BASE_EXPONENT_8:
        return sourcemeta::core::JSON{
            this->get_string_utf8(this->get_varint() + 256)};
      case SUBTYPE_LONG_STRING_BASE_EXPONENT_9:
        return sourcemeta::core::JSON{
            this->get_string_utf8(this->get_varint() + 512)};
      case SUBTYPE_LONG_STRING_BASE_EXPONENT_10:
        return sourcemeta::core::JSON{
            this->get_string_utf8(this->get_varint() + 1024)};
    }

    // We should never get here. If so, it is definitely a bug
    unreachable();
  } else {
    switch (type) {
      case TYPE_POSITIVE_INTEGER_BYTE:
        return sourcemeta::core::JSON{subtype > 0 ? subtype - 1
                                                  : this->get_byte()};
      case TYPE_NEGATIVE_INTEGER_BYTE:
        return sourcemeta::core::JSON{
            subtype > 0 ? static_cast<std::int64_t>(-subtype)
                        : static_cast<std::int64_t>(-this->get_byte() - 1)};
      case TYPE_SHARED_STRING: {
        const auto length = subtype == 0
                                ? this->get_varint() - 1 + uint_max<5> * 2
                                : subtype - 1;
        const std::uint64_t position{this->position()};
        const std::uint64_t current{this->rewind(this->get_varint(), position)};
        const sourcemeta::core::JSON value{this->get_string_utf8(length)};
        this->seek(current);
        return value;
      };
      case TYPE_STRING:
        return subtype == 0
                   ? this->FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
                         {uint_max<5> * 2})
                   : sourcemeta::core::JSON{this->get_string_utf8(subtype - 1)};
      case TYPE_LONG_STRING:
        return sourcemeta::core::JSON{
            this->get_string_utf8(subtype + uint_max<5>)};
      case TYPE_ARRAY:
        return subtype == 0 ? this->FIXED_TYPED_ARRAY(
                                  {this->get_varint() + uint_max<5>,
                                   std::make_shared<Encoding>(
                                       sourcemeta::jsonbinpack::
                                           ANY_PACKED_TYPE_TAG_BYTE_PREFIX{}),
                                   {}})
                            : this->FIXED_TYPED_ARRAY(
                                  {static_cast<std::uint64_t>(subtype - 1),
                                   std::make_shared<Encoding>(
                                       sourcemeta::jsonbinpack::
                                           ANY_PACKED_TYPE_TAG_BYTE_PREFIX{}),
                                   {}});
      case TYPE_OBJECT:
        return subtype == 0
                   ? this->FIXED_TYPED_ARBITRARY_OBJECT(
                         {this->get_varint() + uint_max<5>,
                          std::make_shared<Encoding>(
                              sourcemeta::jsonbinpack::
                                  PREFIX_VARINT_LENGTH_STRING_SHARED{}),
                          std::make_shared<Encoding>(
                              sourcemeta::jsonbinpack::
                                  ANY_PACKED_TYPE_TAG_BYTE_PREFIX{})})
                   : this->FIXED_TYPED_ARBITRARY_OBJECT(
                         {static_cast<std::uint64_t>(subtype - 1),
                          std::make_shared<Encoding>(
                              sourcemeta::jsonbinpack::
                                  PREFIX_VARINT_LENGTH_STRING_SHARED{}),
                          std::make_shared<Encoding>(
                              sourcemeta::jsonbinpack::
                                  ANY_PACKED_TYPE_TAG_BYTE_PREFIX{})});
    }

    // We should never get here. If so, it is definitely a bug
    unreachable();
  }
}

} // namespace sourcemeta::jsonbinpack
