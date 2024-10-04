#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_ENCODER_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_ENCODER_H_

#include <sourcemeta/jsonbinpack/numeric.h>

#include <sourcemeta/jsonbinpack/runtime_encoder_basic.h>
#include <sourcemeta/jsonbinpack/runtime_plan.h>
#include <sourcemeta/jsonbinpack/runtime_plan_wrap.h>

#include <sourcemeta/jsontoolkit/json.h>

#include <cstdint> // std::uint8_t, std::uint16_t, std::int64_t, std::uint64_t
#include <cstdlib> // std::abort
#include <ostream> // std::basic_ostream
#include <string>  // std::basic_string, std::stoul
#include <utility> // std::move

namespace sourcemeta::jsonbinpack {

/// @ingroup runtime
template <typename CharT, typename Traits>
class Encoder : private BasicEncoder<CharT, Traits> {
public:
  Encoder(std::basic_ostream<CharT, Traits> &output)
      : BasicEncoder<CharT, Traits>{output} {}

  auto encode(const sourcemeta::jsontoolkit::JSON &document,
              const Plan &encoding) -> void {
    switch (encoding.index()) {
#define HANDLE_ENCODING(index, name)                                           \
  case (index):                                                                \
    return this->name(document,                                                \
                      std::get<sourcemeta::jsonbinpack::name>(encoding));
      HANDLE_ENCODING(0, BOUNDED_MULTIPLE_8BITS_ENUM_FIXED)
      HANDLE_ENCODING(1, FLOOR_MULTIPLE_ENUM_VARINT)
      HANDLE_ENCODING(2, ROOF_MULTIPLE_MIRROR_ENUM_VARINT)
      HANDLE_ENCODING(3, ARBITRARY_MULTIPLE_ZIGZAG_VARINT)
      HANDLE_ENCODING(4, DOUBLE_VARINT_TUPLE)
      HANDLE_ENCODING(5, BYTE_CHOICE_INDEX)
      HANDLE_ENCODING(6, LARGE_CHOICE_INDEX)
      HANDLE_ENCODING(7, TOP_LEVEL_BYTE_CHOICE_INDEX)
      HANDLE_ENCODING(8, CONST_NONE)
      HANDLE_ENCODING(9, UTF8_STRING_NO_LENGTH)
      HANDLE_ENCODING(10, FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED)
      HANDLE_ENCODING(11, ROOF_VARINT_PREFIX_UTF8_STRING_SHARED)
      HANDLE_ENCODING(12, BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED)
      HANDLE_ENCODING(13, RFC3339_DATE_INTEGER_TRIPLET)
      HANDLE_ENCODING(14, PREFIX_VARINT_LENGTH_STRING_SHARED)
      HANDLE_ENCODING(15, FIXED_TYPED_ARRAY)
      HANDLE_ENCODING(16, BOUNDED_8BITS_TYPED_ARRAY)
      HANDLE_ENCODING(17, FLOOR_TYPED_ARRAY)
      HANDLE_ENCODING(18, ROOF_TYPED_ARRAY)
      HANDLE_ENCODING(19, FIXED_TYPED_ARBITRARY_OBJECT)
      HANDLE_ENCODING(20, VARINT_TYPED_ARBITRARY_OBJECT)
      HANDLE_ENCODING(21, ANY_PACKED_TYPE_TAG_BYTE_PREFIX)
#undef HANDLE_ENCODING
    }

    // We should never get here. If so, it is definitely a bug
    assert(false);
    std::abort();
  }

// The methods that implement individual encodings as considered private
#ifndef DOXYGEN

  auto BOUNDED_MULTIPLE_8BITS_ENUM_FIXED(
      const sourcemeta::jsontoolkit::JSON &document,
      const BOUNDED_MULTIPLE_8BITS_ENUM_FIXED &options) -> void {
    assert(document.is_integer());
    const std::int64_t value{document.to_integer()};
    assert(is_within(value, options.minimum, options.maximum));
    assert(options.multiplier > 0);
    assert(abs(value) % options.multiplier == 0);
    const std::int64_t enum_minimum{
        divide_ceil(options.minimum, options.multiplier)};
#ifndef NDEBUG
    const std::int64_t enum_maximum{
        divide_floor(options.maximum, options.multiplier)};
#endif
    assert(is_byte(enum_maximum - enum_minimum));
    this->put_byte(static_cast<std::uint8_t>(
        (value / static_cast<std::int64_t>(options.multiplier)) -
        enum_minimum));
  }

  auto FLOOR_MULTIPLE_ENUM_VARINT(const sourcemeta::jsontoolkit::JSON &document,
                                  const FLOOR_MULTIPLE_ENUM_VARINT &options)
      -> void {
    assert(document.is_integer());
    const std::int64_t value{document.to_integer()};
    assert(options.minimum <= value);
    assert(options.multiplier > 0);
    assert(abs(value) % options.multiplier == 0);
    if (options.multiplier == 1) {
      return this->put_varint(
          static_cast<std::uint64_t>(value - options.minimum));
    }

    return this->put_varint(
        (static_cast<std::uint64_t>(value) / options.multiplier) -
        static_cast<std::uint64_t>(divide_ceil(
            options.minimum, static_cast<std::uint64_t>(options.multiplier))));
  }

  auto ROOF_MULTIPLE_MIRROR_ENUM_VARINT(
      const sourcemeta::jsontoolkit::JSON &document,
      const ROOF_MULTIPLE_MIRROR_ENUM_VARINT &options) -> void {
    assert(document.is_integer());
    const std::int64_t value{document.to_integer()};
    assert(value <= options.maximum);
    assert(options.multiplier > 0);
    assert(abs(value) % options.multiplier == 0);
    if (options.multiplier == 1) {
      return this->put_varint(
          static_cast<std::uint64_t>(options.maximum - value));
    }

    return this->put_varint(
        static_cast<std::uint64_t>(
            divide_floor(options.maximum, options.multiplier)) -
        (static_cast<std::uint64_t>(value) / options.multiplier));
  }

  auto ARBITRARY_MULTIPLE_ZIGZAG_VARINT(
      const sourcemeta::jsontoolkit::JSON &document,
      const ARBITRARY_MULTIPLE_ZIGZAG_VARINT &options) -> void {
    assert(document.is_integer());
    const std::int64_t value{document.to_integer()};
    assert(options.multiplier > 0);
    assert(abs(value) % options.multiplier == 0);
    this->put_varint_zigzag(value /
                            static_cast<std::int64_t>(options.multiplier));
  }

  auto DOUBLE_VARINT_TUPLE(const sourcemeta::jsontoolkit::JSON &document,
                           const DOUBLE_VARINT_TUPLE &) -> void {
    assert(document.is_real());
    const auto value{document.to_real()};
    std::uint64_t point_position;
    const std::int64_t integral{
        real_digits<std::int64_t>(value, &point_position)};
    this->put_varint_zigzag(integral);
    this->put_varint(point_position);
  }

  auto BYTE_CHOICE_INDEX(const sourcemeta::jsontoolkit::JSON &document,
                         const BYTE_CHOICE_INDEX &options) -> void {
    assert(!options.choices.empty());
    assert(is_byte(options.choices.size()));
    const auto iterator{std::find_if(
        std::cbegin(options.choices), std::cend(options.choices),
        [&document](const auto &choice) { return choice == document; })};
    assert(iterator != std::cend(options.choices));
    const auto cursor{std::distance(std::cbegin(options.choices), iterator)};
    assert(is_within(cursor, 0,
                     static_cast<std::int64_t>(options.choices.size())));
    this->put_byte(static_cast<std::uint8_t>(cursor));
  }

  auto LARGE_CHOICE_INDEX(const sourcemeta::jsontoolkit::JSON &document,
                          const LARGE_CHOICE_INDEX &options) -> void {
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

  auto
  TOP_LEVEL_BYTE_CHOICE_INDEX(const sourcemeta::jsontoolkit::JSON &document,
                              const TOP_LEVEL_BYTE_CHOICE_INDEX &options)
      -> void {
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

  auto CONST_NONE(
#ifndef NDEBUG
      const sourcemeta::jsontoolkit::JSON &document, const CONST_NONE &options)
#else
      const sourcemeta::jsontoolkit::JSON &, const CONST_NONE &)
#endif
      -> void {
    assert(document == options.value);
  }

  auto UTF8_STRING_NO_LENGTH(const sourcemeta::jsontoolkit::JSON &document,
                             const UTF8_STRING_NO_LENGTH &options) -> void {
    assert(document.is_string());
    const std::basic_string<CharT> value{document.to_string()};
    this->put_string_utf8(value, options.size);
  }

  auto FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
      const sourcemeta::jsontoolkit::JSON &document,
      const FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED &options) -> void {
    assert(document.is_string());
    const std::basic_string<CharT> value{document.to_string()};
    const auto size{value.size()};
    assert(document.size() == size);
    const bool is_shared{this->context().has(value, ContextType::Standalone)};

    // (1) Write 0x00 if shared, else do nothing
    if (is_shared) {
      this->put_byte(0);
    }

    // (2) Write length of the string + 1 (so it will never be zero)
    this->put_varint(size - options.minimum + 1);

    // (3) Write relative offset if shared, else write plain string
    if (is_shared) {
      this->put_varint(this->position() -
                       this->context().offset(value, ContextType::Standalone));
    } else {
      this->context().record(value, this->position(), ContextType::Standalone);
      this->put_string_utf8(value, size);
    }
  }

  auto ROOF_VARINT_PREFIX_UTF8_STRING_SHARED(
      const sourcemeta::jsontoolkit::JSON &document,
      const ROOF_VARINT_PREFIX_UTF8_STRING_SHARED &options) -> void {
    assert(document.is_string());
    const std::basic_string<CharT> value{document.to_string()};
    const auto size{value.size()};
    assert(document.size() == size);
    assert(size <= options.maximum);
    const bool is_shared{this->context().has(value, ContextType::Standalone)};

    // (1) Write 0x00 if shared, else do nothing
    if (is_shared) {
      this->put_byte(0);
    }

    // (2) Write length of the string + 1 (so it will never be zero)
    this->put_varint(options.maximum - size + 1);

    // (3) Write relative offset if shared, else write plain string
    if (is_shared) {
      this->put_varint(this->position() -
                       this->context().offset(value, ContextType::Standalone));
    } else {
      this->context().record(value, this->position(), ContextType::Standalone);
      this->put_string_utf8(value, size);
    }
  }

  auto BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED(
      const sourcemeta::jsontoolkit::JSON &document,
      const BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED &options) -> void {
    assert(document.is_string());
    const std::basic_string<CharT> value{document.to_string()};
    const auto size{value.size()};
    assert(document.size() == size);
    assert(options.minimum <= options.maximum);
    assert(is_byte(options.maximum - options.minimum + 1));
    assert(is_within(size, options.minimum, options.maximum));
    const bool is_shared{this->context().has(value, ContextType::Standalone)};

    // (1) Write 0x00 if shared, else do nothing
    if (is_shared) {
      this->put_byte(0);
    }

    // (2) Write length of the string + 1 (so it will never be zero)
    this->put_byte(static_cast<std::uint8_t>(size - options.minimum + 1));

    // (3) Write relative offset if shared, else write plain string
    if (is_shared) {
      this->put_varint(this->position() -
                       this->context().offset(value, ContextType::Standalone));
    } else {
      this->context().record(value, this->position(), ContextType::Standalone);
      this->put_string_utf8(value, size);
    }
  }

  auto
  RFC3339_DATE_INTEGER_TRIPLET(const sourcemeta::jsontoolkit::JSON &document,
                               const RFC3339_DATE_INTEGER_TRIPLET &) -> void {
    assert(document.is_string());
    const auto &value{document.to_string()};
    assert(value.size() == 10);
    assert(document.size() == value.size());

    // As according to RFC3339: Internet Protocols MUST
    // generate four digit years in dates.
    const std::uint16_t year{
        static_cast<std::uint16_t>(std::stoul(value.substr(0, 4)))};
    const std::uint8_t month{
        static_cast<std::uint8_t>(std::stoul(value.substr(5, 2)))};
    const std::uint8_t day{
        static_cast<std::uint8_t>(std::stoul(value.substr(8, 2)))};
    assert(month >= 1 && month <= 12);
    assert(day >= 1 && day <= 31);

    this->put_bytes(year);
    this->put_byte(month);
    this->put_byte(day);
  }

  auto PREFIX_VARINT_LENGTH_STRING_SHARED(
      const sourcemeta::jsontoolkit::JSON &document,
      const PREFIX_VARINT_LENGTH_STRING_SHARED &) -> void {
    assert(document.is_string());
    const std::basic_string<CharT> value{document.to_string()};

    if (this->context().has(value, ContextType::PrefixLengthVarintPlusOne)) {
      const auto new_offset{this->position()};
      this->put_byte(0);
      this->put_varint(this->position() -
                       this->context().offset(
                           value, ContextType::PrefixLengthVarintPlusOne));
      // Bump the context cache for locality purposes
      this->context().record(value, new_offset,
                             ContextType::PrefixLengthVarintPlusOne);
    } else {
      const auto size{value.size()};
      assert(document.size() == size);
      this->context().record(value, this->position(),
                             ContextType::PrefixLengthVarintPlusOne);
      this->put_varint(size + 1);
      // Also record a standalone variant of it
      this->context().record(value, this->position(), ContextType::Standalone);
      this->put_string_utf8(value, size);
    }
  }

  // TODO: Implement STRING_BROTLI encoding
  // TODO: Implement STRING_DICTIONARY_COMPRESSOR encoding
  // TODO: Implement STRING_UNBOUNDED_SCOPED_PREFIX_LENGTH encoding
  // TODO: Implement URL_PROTOCOL_HOST_REST encoding

  auto FIXED_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &document,
                         const FIXED_TYPED_ARRAY &options) -> void {
    assert(document.is_array());
    assert(document.size() == options.size);
    const auto prefix_encodings{options.prefix_encodings.size()};
    assert(prefix_encodings <= document.size());
    for (std::size_t index = 0; index < options.size; index++) {
      const Plan &encoding{prefix_encodings > index
                               ? options.prefix_encodings[index].value
                               : options.encoding->value};
      this->encode(document.at(index), encoding);
    }
  }

  auto BOUNDED_8BITS_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &document,
                                 const BOUNDED_8BITS_TYPED_ARRAY &options)
      -> void {
    assert(options.maximum >= options.minimum);
    const auto size{document.size()};
    assert(is_within(size, options.minimum, options.maximum));
    assert(is_byte(options.maximum - options.minimum));
    this->put_byte(static_cast<std::uint8_t>(size - options.minimum));
    this->FIXED_TYPED_ARRAY(document, {size, std::move(options.encoding),
                                       std::move(options.prefix_encodings)});
  }

  auto FLOOR_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &document,
                         const FLOOR_TYPED_ARRAY &options) -> void {
    const auto size{document.size()};
    assert(size >= options.minimum);
    this->put_varint(size - options.minimum);
    this->FIXED_TYPED_ARRAY(document, {size, std::move(options.encoding),
                                       std::move(options.prefix_encodings)});
  }

  auto ROOF_TYPED_ARRAY(const sourcemeta::jsontoolkit::JSON &document,
                        const ROOF_TYPED_ARRAY &options) -> void {
    const auto size{document.size()};
    assert(size <= options.maximum);
    this->put_varint(options.maximum - size);
    this->FIXED_TYPED_ARRAY(document, {size, std::move(options.encoding),
                                       std::move(options.prefix_encodings)});
  }

  auto
  FIXED_TYPED_ARBITRARY_OBJECT(const sourcemeta::jsontoolkit::JSON &document,
                               const FIXED_TYPED_ARBITRARY_OBJECT &options)
      -> void {
    assert(document.is_object());
    assert(document.size() == options.size);

    for (const auto &[key, value] : document.as_object()) {
      this->encode(sourcemeta::jsontoolkit::JSON{key},
                   options.key_encoding->value);
      this->encode(value, options.encoding->value);
    }
  }

  auto
  VARINT_TYPED_ARBITRARY_OBJECT(const sourcemeta::jsontoolkit::JSON &document,
                                const VARINT_TYPED_ARBITRARY_OBJECT &options)
      -> void {
    assert(document.is_object());
    const auto size{document.size()};
    this->put_varint(size);

    for (const auto &[key, value] : document.as_object()) {
      this->encode(sourcemeta::jsontoolkit::JSON{key},
                   options.key_encoding->value);
      this->encode(value, options.encoding->value);
    }
  }

  auto
  ANY_PACKED_TYPE_TAG_BYTE_PREFIX(const sourcemeta::jsontoolkit::JSON &document,
                                  const ANY_PACKED_TYPE_TAG_BYTE_PREFIX &)
      -> void {
    using namespace internal::ANY_PACKED_TYPE_TAG_BYTE_PREFIX;
    if (document.is_null()) {
      this->put_byte(TYPE_OTHER | (SUBTYPE_NULL << type_size));
    } else if (document.is_boolean()) {
      const std::uint8_t subtype{document.to_boolean() ? SUBTYPE_TRUE
                                                       : SUBTYPE_FALSE};
      this->put_byte(TYPE_OTHER |
                     static_cast<std::uint8_t>(subtype << type_size));
    } else if (document.is_real()) {
      this->put_byte(TYPE_OTHER | SUBTYPE_NUMBER << type_size);
      this->DOUBLE_VARINT_TUPLE(document, {});
    } else if (document.is_integer()) {
      const std::int64_t value{document.to_integer()};
      const bool is_positive{value >= 0};
      const std::uint64_t absolute{
          is_positive ? static_cast<std::uint64_t>(value) : abs(value) - 1};
      if (is_byte(absolute)) {
        const std::uint8_t type{is_positive ? TYPE_POSITIVE_INTEGER_BYTE
                                            : TYPE_NEGATIVE_INTEGER_BYTE};
        const std::uint8_t absolute_byte{static_cast<std::uint8_t>(absolute)};
        if (absolute < uint_max<5>) {
          this->put_byte(type | static_cast<std::uint8_t>((absolute_byte + 1)
                                                          << type_size));
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
      const std::basic_string<CharT> value{document.to_string()};
      const auto size{document.byte_size()};
      const bool is_shared{this->context().has(value, ContextType::Standalone)};
      if (size < uint_max<5>) {
        const std::uint8_t type{is_shared ? TYPE_SHARED_STRING : TYPE_STRING};
        this->put_byte(
            static_cast<std::uint8_t>(type | ((size + 1) << type_size)));
        if (is_shared) {
          this->put_varint(
              this->position() -
              this->context().offset(value, ContextType::Standalone));
        } else {
          this->context().record(value, this->position(),
                                 ContextType::Standalone);
          this->put_string_utf8(value, size);
        }
      } else if (size >= uint_max<5> && size < uint_max<5> * 2 && !is_shared) {
        this->put_byte(static_cast<std::uint8_t>(
            TYPE_LONG_STRING | ((size - uint_max<5>) << type_size)));
        this->put_string_utf8(value, size);
      } else if (size >= 2 << (SUBTYPE_LONG_STRING_BASE_EXPONENT_7 - 1) &&
                 !is_shared) {
        const std::uint8_t exponent{closest_smallest_exponent(
            size, 2, SUBTYPE_LONG_STRING_BASE_EXPONENT_7,
            SUBTYPE_LONG_STRING_BASE_EXPONENT_10)};
        this->put_byte(
            static_cast<std::uint8_t>(TYPE_OTHER | (exponent << type_size)));
        this->put_varint(size -
                         static_cast<std::uint64_t>(2 << (exponent - 1)));
        this->put_string_utf8(value, size);
      } else {
        // Exploit the fact that a shared string always starts
        // with an impossible length marker (0) to avoid having
        // to encode an additional tag
        if (!is_shared) {
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

      Plan encoding{sourcemeta::jsonbinpack::ANY_PACKED_TYPE_TAG_BYTE_PREFIX{}};
      this->FIXED_TYPED_ARRAY(document, {size, wrap(std::move(encoding)), {}});
    } else if (document.is_object()) {
      const auto size{document.size()};
      if (size >= uint_max<5>) {
        this->put_byte(TYPE_OBJECT);
        this->put_varint(size - uint_max<5>);
      } else {
        this->put_byte(
            static_cast<std::uint8_t>(TYPE_OBJECT | ((size + 1) << type_size)));
      }

      Plan key_encoding{
          sourcemeta::jsonbinpack::PREFIX_VARINT_LENGTH_STRING_SHARED{}};
      Plan value_encoding{
          sourcemeta::jsonbinpack::ANY_PACKED_TYPE_TAG_BYTE_PREFIX{}};
      this->FIXED_TYPED_ARBITRARY_OBJECT(document,
                                         {size, wrap(std::move(key_encoding)),
                                          wrap(std::move(value_encoding))});
    } else {
      // We should never get here.
      assert(false);
      std::abort();
    }
  }

#endif

private:
  using ContextType = typename Context<CharT>::Type;
};

} // namespace sourcemeta::jsonbinpack

#endif
