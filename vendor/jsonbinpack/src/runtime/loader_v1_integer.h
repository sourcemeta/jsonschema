#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_INTEGER_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_INTEGER_H_

#include <sourcemeta/jsonbinpack/runtime.h>

#include <sourcemeta/core/json.h>

#include <cassert> // assert
#include <cstdint> // std::uint64_t

namespace sourcemeta::jsonbinpack::v1 {

auto BOUNDED_MULTIPLE_8BITS_ENUM_FIXED(const sourcemeta::core::JSON &options)
    -> Encoding {
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

auto FLOOR_MULTIPLE_ENUM_VARINT(const sourcemeta::core::JSON &options)
    -> Encoding {
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

auto ROOF_MULTIPLE_MIRROR_ENUM_VARINT(const sourcemeta::core::JSON &options)
    -> Encoding {
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

auto ARBITRARY_MULTIPLE_ZIGZAG_VARINT(const sourcemeta::core::JSON &options)
    -> Encoding {
  assert(options.defines("multiplier"));
  const auto &multiplier{options.at("multiplier")};
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::jsonbinpack::ARBITRARY_MULTIPLE_ZIGZAG_VARINT{
      static_cast<std::uint64_t>(multiplier.to_integer())};
}

} // namespace sourcemeta::jsonbinpack::v1

#endif
