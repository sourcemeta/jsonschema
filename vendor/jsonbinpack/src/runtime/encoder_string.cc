#include <sourcemeta/jsonbinpack/runtime_encoder.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t, std::uint16_t
#include <string>  // std::stoul

namespace sourcemeta::jsonbinpack {

auto Encoder::UTF8_STRING_NO_LENGTH(const sourcemeta::core::JSON &document,
                                    const struct UTF8_STRING_NO_LENGTH &options)
    -> void {
  assert(document.is_string());
  const sourcemeta::core::JSON::String &value{document.to_string()};
  this->put_string_utf8(value, options.size);
}

auto Encoder::FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &document,
    const struct FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED &options) -> void {
  assert(document.is_string());
  const sourcemeta::core::JSON::String &value{document.to_string()};
  const auto size{value.size()};
  assert(document.byte_size() == size);
  const auto shared{this->cache_.find(value, Cache::Type::Standalone)};

  // (1) Write 0x00 if shared, else do nothing
  if (shared.has_value()) {
    this->put_byte(0);
  }

  // (2) Write length of the string + 1 (so it will never be zero)
  this->put_varint(size - options.minimum + 1);

  // (3) Write relative offset if shared, else write plain string
  if (shared.has_value()) {
    this->put_varint(this->position() - shared.value());
  } else {
    this->cache_.record(value, this->position(), Cache::Type::Standalone);
    this->put_string_utf8(value, size);
  }
}

auto Encoder::ROOF_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &document,
    const struct ROOF_VARINT_PREFIX_UTF8_STRING_SHARED &options) -> void {
  assert(document.is_string());
  const sourcemeta::core::JSON::String &value{document.to_string()};
  const auto size{value.size()};
  assert(document.byte_size() == size);
  assert(size <= options.maximum);
  const auto shared{this->cache_.find(value, Cache::Type::Standalone)};

  // (1) Write 0x00 if shared, else do nothing
  if (shared.has_value()) {
    this->put_byte(0);
  }

  // (2) Write length of the string + 1 (so it will never be zero)
  this->put_varint(options.maximum - size + 1);

  // (3) Write relative offset if shared, else write plain string
  if (shared.has_value()) {
    this->put_varint(this->position() - shared.value());
  } else {
    this->cache_.record(value, this->position(), Cache::Type::Standalone);
    this->put_string_utf8(value, size);
  }
}

auto Encoder::BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &document,
    const struct BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED &options) -> void {
  assert(document.is_string());
  const sourcemeta::core::JSON::String &value{document.to_string()};
  const auto size{value.size()};
  assert(document.byte_size() == size);
  assert(options.minimum <= options.maximum);
  assert(is_byte(options.maximum - options.minimum + 1));
  assert(is_within(size, options.minimum, options.maximum));
  const auto shared{this->cache_.find(value, Cache::Type::Standalone)};

  // (1) Write 0x00 if shared, else do nothing
  if (shared.has_value()) {
    this->put_byte(0);
  }

  // (2) Write length of the string + 1 (so it will never be zero)
  this->put_byte(static_cast<std::uint8_t>(size - options.minimum + 1));

  // (3) Write relative offset if shared, else write plain string
  if (shared.has_value()) {
    this->put_varint(this->position() - shared.value());
  } else {
    this->cache_.record(value, this->position(), Cache::Type::Standalone);
    this->put_string_utf8(value, size);
  }
}

auto Encoder::RFC3339_DATE_INTEGER_TRIPLET(
    const sourcemeta::core::JSON &document,
    const struct RFC3339_DATE_INTEGER_TRIPLET &) -> void {
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

auto Encoder::PREFIX_VARINT_LENGTH_STRING_SHARED(
    const sourcemeta::core::JSON &document,
    const struct PREFIX_VARINT_LENGTH_STRING_SHARED &) -> void {
  assert(document.is_string());
  const sourcemeta::core::JSON::String &value{document.to_string()};

  const auto shared{
      this->cache_.find(value, Cache::Type::PrefixLengthVarintPlusOne)};
  if (shared.has_value()) {
    const auto new_offset{this->position()};
    this->put_byte(0);
    this->put_varint(this->position() - shared.value());
    // Bump the context cache for locality purposes
    this->cache_.record(value, new_offset,
                        Cache::Type::PrefixLengthVarintPlusOne);
  } else {
    const auto size{value.size()};
    assert(document.byte_size() == size);
    this->cache_.record(value, this->position(),
                        Cache::Type::PrefixLengthVarintPlusOne);
    this->put_varint(size + 1);
    // Also record a standalone variant of it
    this->cache_.record(value, this->position(), Cache::Type::Standalone);
    this->put_string_utf8(value, size);
  }
}

} // namespace sourcemeta::jsonbinpack
