#include <sourcemeta/jsonbinpack/runtime_decoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint16_t, std::uint64_t
#include <iomanip> // std::setw, std::setfill
#include <sstream> // std::basic_ostringstream

namespace sourcemeta::jsonbinpack {

auto Decoder::UTF8_STRING_NO_LENGTH(const struct UTF8_STRING_NO_LENGTH &options)
    -> sourcemeta::core::JSON {
  return sourcemeta::core::JSON{this->get_string_utf8(options.size)};
}

auto Decoder::FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
    const struct FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED &options)
    -> sourcemeta::core::JSON {
  const std::uint64_t prefix{this->get_varint()};
  const bool is_shared{prefix == 0};
  const std::uint64_t length{(is_shared ? this->get_varint() : prefix) +
                             options.minimum - 1};
  assert(length >= options.minimum);

  if (is_shared) {
    const std::uint64_t position{this->position()};
    const std::uint64_t current{this->rewind(this->get_varint(), position)};
    const sourcemeta::core::JSON value{this->get_string_utf8(length)};
    this->seek(current);
    return value;
  } else {
    return UTF8_STRING_NO_LENGTH({length});
  }
}

auto Decoder::ROOF_VARINT_PREFIX_UTF8_STRING_SHARED(
    const struct ROOF_VARINT_PREFIX_UTF8_STRING_SHARED &options)
    -> sourcemeta::core::JSON {
  const std::uint64_t prefix{this->get_varint()};
  const bool is_shared{prefix == 0};
  const std::uint64_t length{options.maximum -
                             (is_shared ? this->get_varint() : prefix) + 1};
  assert(length <= options.maximum);

  if (is_shared) {
    const std::uint64_t position{this->position()};
    const std::uint64_t current{this->rewind(this->get_varint(), position)};
    const sourcemeta::core::JSON value{UTF8_STRING_NO_LENGTH({length})};
    this->seek(current);
    return value;
  } else {
    return UTF8_STRING_NO_LENGTH({length});
  }
}

auto Decoder::BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED(
    const struct BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED &options)
    -> sourcemeta::core::JSON {
  assert(options.minimum <= options.maximum);
  assert(is_byte(options.maximum - options.minimum));
  const std::uint8_t prefix{this->get_byte()};
  const bool is_shared{prefix == 0};
  const std::uint64_t length{(is_shared ? this->get_byte() : prefix) +
                             options.minimum - 1};
  assert(is_within(length, options.minimum, options.maximum));

  if (is_shared) {
    const std::uint64_t position{this->position()};
    const std::uint64_t current{this->rewind(this->get_varint(), position)};
    const sourcemeta::core::JSON value{UTF8_STRING_NO_LENGTH({length})};
    this->seek(current);
    return value;
  } else {
    return UTF8_STRING_NO_LENGTH({length});
  }
}

auto Decoder::RFC3339_DATE_INTEGER_TRIPLET(
    const struct RFC3339_DATE_INTEGER_TRIPLET &) -> sourcemeta::core::JSON {
  const std::uint16_t year{this->get_word()};
  const std::uint8_t month{this->get_byte()};
  const std::uint8_t day{this->get_byte()};

  assert(year <= 9999);
  assert(month >= 1 && month <= 12);
  assert(day >= 1 && day <= 31);

  std::basic_ostringstream<sourcemeta::core::JSON::Char,
                           sourcemeta::core::JSON::CharTraits>
      output;
  output << std::setfill('0');
  output << std::setw(4) << year;
  output << "-";
  // Cast the bytes to a larger integer, otherwise
  // they will be interpreted as characters.
  output << std::setw(2) << static_cast<std::uint16_t>(month);
  output << "-";
  output << std::setw(2) << static_cast<std::uint16_t>(day);

  return sourcemeta::core::JSON{output.str()};
}

auto Decoder::PREFIX_VARINT_LENGTH_STRING_SHARED(
    const struct PREFIX_VARINT_LENGTH_STRING_SHARED &options)
    -> sourcemeta::core::JSON {
  const std::uint64_t prefix{this->get_varint()};
  if (prefix == 0) {
    const std::uint64_t position{this->position()};
    const std::uint64_t current{this->rewind(this->get_varint(), position)};
    const sourcemeta::core::JSON value{
        PREFIX_VARINT_LENGTH_STRING_SHARED(options)};
    this->seek(current);
    return value;
  } else {
    return sourcemeta::core::JSON{this->get_string_utf8(prefix - 1)};
  }
}

} // namespace sourcemeta::jsonbinpack
