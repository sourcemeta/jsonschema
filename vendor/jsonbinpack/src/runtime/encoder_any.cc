#include <sourcemeta/jsonbinpack/numeric.h>
#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include "unreachable.h"

#include <algorithm> // std::find_if
#include <cassert>   // assert
#include <cstdint>   // std::uint8_t, std::int64_t, std::uint64_t
#include <iterator>  // std::cbegin, std::cend, std::distance
#include <memory>    // std::make_shared
#include <utility>   // std::move

namespace sourcemeta::jsonbinpack {

auto Encoder::BYTE_CHOICE_INDEX(const sourcemeta::core::JSON &document,
                                const struct BYTE_CHOICE_INDEX &options)
    -> void {
  assert(!options.choices.empty());
  assert(is_byte(options.choices.size()));
  const auto iterator{std::find_if(
      std::cbegin(options.choices), std::cend(options.choices),
      [&document](const auto &choice) { return choice == document; })};
  assert(iterator != std::cend(options.choices));
  const auto cursor{std::distance(std::cbegin(options.choices), iterator)};
  assert(
      is_within(cursor, 0, static_cast<std::int64_t>(options.choices.size())));
  this->put_byte(static_cast<std::uint8_t>(cursor));
}

auto Encoder::LARGE_CHOICE_INDEX(const sourcemeta::core::JSON &document,
                                 const struct LARGE_CHOICE_INDEX &options)
    -> void {
  assert(options.choices.size() > 0);
  const auto iterator{std::find_if(
      std::cbegin(options.choices), std::cend(options.choices),
      [&document](const auto &choice) { return choice == document; })};
  assert(iterator != std::cend(options.choices));
  const auto cursor{std::distance(std::cbegin(options.choices), iterator)};
  assert(is_within(cursor, static_cast<std::uint64_t>(0),
                   options.choices.size() - 1));
  this->put_varint(static_cast<std::uint64_t>(cursor));
}

auto Encoder::TOP_LEVEL_BYTE_CHOICE_INDEX(
    const sourcemeta::core::JSON &document,
    const struct TOP_LEVEL_BYTE_CHOICE_INDEX &options) -> void {
  assert(options.choices.size() > 0);
  assert(is_byte(options.choices.size()));
  const auto iterator{std::find_if(
      std::cbegin(options.choices), std::cend(options.choices),
      [&document](auto const &choice) { return choice == document; })};
  assert(iterator != std::cend(options.choices));
  const auto cursor{std::distance(std::cbegin(options.choices), iterator)};
  assert(is_within(cursor, 0,
                   static_cast<std::int64_t>(options.choices.size()) - 1));
  // This encoding encodes the first option of the enum as "no data"
  if (cursor > 0) {
    this->put_byte(static_cast<std::uint8_t>(cursor - 1));
  }
}

auto Encoder::CONST_NONE(
#ifndef NDEBUG
    const sourcemeta::core::JSON &document, const struct CONST_NONE &options)
#else
    const sourcemeta::core::JSON &, const struct CONST_NONE &)
#endif
    -> void {
  assert(document == options.value);
}

auto Encoder::ANY_PACKED_TYPE_TAG_BYTE_PREFIX(
    const sourcemeta::core::JSON &document,
    const struct ANY_PACKED_TYPE_TAG_BYTE_PREFIX &) -> void {
  using namespace internal::ANY_PACKED_TYPE_TAG_BYTE_PREFIX;
  if (document.is_null()) {
    this->put_byte(TYPE_OTHER | (SUBTYPE_NULL << type_size));
  } else if (document.is_boolean()) {
    const std::uint8_t subtype{document.to_boolean() ? SUBTYPE_TRUE
                                                     : SUBTYPE_FALSE};
    this->put_byte(TYPE_OTHER |
                   static_cast<std::uint8_t>(subtype << type_size));
  } else if (document.is_integer_real()) {
    const auto value{document.as_integer()};
    if (value >= 0 && is_byte(value)) {
      this->put_byte(TYPE_OTHER | SUBTYPE_POSITIVE_REAL_INTEGER_BYTE
                                      << type_size);
      this->put_byte(static_cast<std::uint8_t>(value));
    } else {
      this->put_byte(TYPE_OTHER | SUBTYPE_NUMBER << type_size);
      this->DOUBLE_VARINT_TUPLE(document, {});
    }
  } else if (document.is_real()) {
    this->put_byte(TYPE_OTHER | SUBTYPE_NUMBER << type_size);
    this->DOUBLE_VARINT_TUPLE(document, {});
  } else if (document.is_integer()) {
    const std::int64_t value{document.to_integer()};
    const bool is_positive{value >= 0};
    const std::uint64_t absolute{is_positive ? static_cast<std::uint64_t>(value)
                                             : abs(value) - 1};
    if (is_byte(absolute)) {
      const std::uint8_t type{is_positive ? TYPE_POSITIVE_INTEGER_BYTE
                                          : TYPE_NEGATIVE_INTEGER_BYTE};
      const std::uint8_t absolute_byte{static_cast<std::uint8_t>(absolute)};
      if (absolute < uint_max<5>) {
        this->put_byte(
            type | static_cast<std::uint8_t>((absolute_byte + 1) << type_size));
      } else {
        this->put_byte(type);
        this->put_byte(absolute_byte);
      }
    } else {
      const std::uint8_t subtype{is_positive ? SUBTYPE_POSITIVE_INTEGER
                                             : SUBTYPE_NEGATIVE_INTEGER};
      this->put_byte(TYPE_OTHER |
                     static_cast<std::uint8_t>(subtype << type_size));
      this->put_varint(absolute);
    }
  } else if (document.is_string()) {
    const sourcemeta::core::JSON::String value{document.to_string()};
    const auto size{document.byte_size()};
    const auto shared{this->cache_.find(value, Cache::Type::Standalone)};
    if (size < uint_max<5>) {
      const std::uint8_t type{shared.has_value() ? TYPE_SHARED_STRING
                                                 : TYPE_STRING};
      this->put_byte(
          static_cast<std::uint8_t>(type | ((size + 1) << type_size)));
      if (shared.has_value()) {
        this->put_varint(this->position() - shared.value());
      } else {
        this->cache_.record(value, this->position(), Cache::Type::Standalone);
        this->put_string_utf8(value, size);
      }
    } else if (size >= uint_max<5> && size < uint_max<5> * 2 &&
               !shared.has_value()) {
      this->put_byte(static_cast<std::uint8_t>(
          TYPE_LONG_STRING | ((size - uint_max<5>) << type_size)));
      this->put_string_utf8(value, size);
    } else if (size >= 2 << (SUBTYPE_LONG_STRING_BASE_EXPONENT_7 - 1) &&
               !shared.has_value()) {
      const std::uint8_t exponent{closest_smallest_exponent(
          size, 2, SUBTYPE_LONG_STRING_BASE_EXPONENT_7,
          SUBTYPE_LONG_STRING_BASE_EXPONENT_10)};
      this->put_byte(
          static_cast<std::uint8_t>(TYPE_OTHER | (exponent << type_size)));
      this->put_varint(size - static_cast<std::uint64_t>(2 << (exponent - 1)));
      this->put_string_utf8(value, size);
    } else {
      // Exploit the fact that a shared string always starts
      // with an impossible length marker (0) to avoid having
      // to encode an additional tag
      if (!shared.has_value()) {
        this->put_byte(TYPE_STRING);
      }

      // If we got this far, the string is at least a certain length
      return FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(document,
                                                    {uint_max<5> * 2});
    }
  } else if (document.is_array()) {
    const auto size{document.size()};
    if (size >= uint_max<5>) {
      this->put_byte(TYPE_ARRAY);
      this->put_varint(size - uint_max<5>);
    } else {
      this->put_byte(
          static_cast<std::uint8_t>(TYPE_ARRAY | ((size + 1) << type_size)));
    }

    Encoding encoding{
        sourcemeta::jsonbinpack::ANY_PACKED_TYPE_TAG_BYTE_PREFIX{}};
    this->FIXED_TYPED_ARRAY(
        document, {size, std::make_shared<Encoding>(std::move(encoding)), {}});
  } else if (document.is_object()) {
    const auto size{document.size()};
    if (size >= uint_max<5>) {
      this->put_byte(TYPE_OBJECT);
      this->put_varint(size - uint_max<5>);
    } else {
      this->put_byte(
          static_cast<std::uint8_t>(TYPE_OBJECT | ((size + 1) << type_size)));
    }

    Encoding key_encoding{
        sourcemeta::jsonbinpack::PREFIX_VARINT_LENGTH_STRING_SHARED{}};
    Encoding value_encoding{
        sourcemeta::jsonbinpack::ANY_PACKED_TYPE_TAG_BYTE_PREFIX{}};
    this->FIXED_TYPED_ARBITRARY_OBJECT(
        document, {size, std::make_shared<Encoding>(std::move(key_encoding)),
                   std::make_shared<Encoding>(std::move(value_encoding))});
  } else {
    // We should never get here
    unreachable();
  }
}

} // namespace sourcemeta::jsonbinpack
