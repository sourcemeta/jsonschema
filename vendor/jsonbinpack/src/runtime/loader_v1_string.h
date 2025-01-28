#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_STRING_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_STRING_H_

#include <sourcemeta/jsonbinpack/runtime.h>

#include <sourcemeta/core/json.h>

#include <cassert> // assert
#include <cstdint> // std::uint64_t

namespace sourcemeta::jsonbinpack::v1 {

auto UTF8_STRING_NO_LENGTH(const sourcemeta::core::JSON &options) -> Encoding {
  assert(options.defines("size"));
  const auto &size{options.at("size")};
  assert(size.is_integer());
  assert(size.is_positive());
  return sourcemeta::jsonbinpack::UTF8_STRING_NO_LENGTH{
      static_cast<std::uint64_t>(size.to_integer())};
}

auto FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &options) -> Encoding {
  assert(options.defines("minimum"));
  const auto &minimum{options.at("minimum")};
  assert(minimum.is_integer());
  assert(minimum.is_positive());
  return sourcemeta::jsonbinpack::FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED{
      static_cast<std::uint64_t>(minimum.to_integer())};
}

auto ROOF_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &options) -> Encoding {
  assert(options.defines("maximum"));
  const auto &maximum{options.at("maximum")};
  assert(maximum.is_integer());
  assert(maximum.is_positive());
  return sourcemeta::jsonbinpack::ROOF_VARINT_PREFIX_UTF8_STRING_SHARED{
      static_cast<std::uint64_t>(maximum.to_integer())};
}

auto BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &options) -> Encoding {
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

auto RFC3339_DATE_INTEGER_TRIPLET(const sourcemeta::core::JSON &) -> Encoding {
  return sourcemeta::jsonbinpack::RFC3339_DATE_INTEGER_TRIPLET{};
}

auto PREFIX_VARINT_LENGTH_STRING_SHARED(const sourcemeta::core::JSON &)
    -> Encoding {
  return sourcemeta::jsonbinpack::PREFIX_VARINT_LENGTH_STRING_SHARED{};
}

} // namespace sourcemeta::jsonbinpack::v1

#endif
