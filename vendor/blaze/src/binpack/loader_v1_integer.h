#ifndef SOURCEMETA_BLAZE_BINPACK_LOADER_V1_INTEGER_H_
#define SOURCEMETA_BLAZE_BINPACK_LOADER_V1_INTEGER_H_

#include <sourcemeta/blaze/binpack.h>

#include <sourcemeta/core/json.h>

#include <cassert> // assert
#include <cstdint> // std::uint64_t

namespace sourcemeta::blaze::v1 {

// The encoding names are the ones the JSON BinPack specification defines, so
// they keep their casing rather than following the C++ naming convention
// NOLINTBEGIN(readability-identifier-naming)

auto BOUNDED_MULTIPLE_8BITS_ENUM_FIXED(const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
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
  return sourcemeta::blaze::BOUNDED_MULTIPLE_8BITS_ENUM_FIXED{
      .minimum = minimum.to_integer(),
      .maximum = maximum.to_integer(),
      .multiplier = static_cast<std::uint64_t>(multiplier.to_integer())};
}

auto FLOOR_MULTIPLE_ENUM_VARINT(const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
  assert(options.defines("minimum"));
  assert(options.defines("multiplier"));
  const auto &minimum{options.at("minimum")};
  const auto &multiplier{options.at("multiplier")};
  assert(minimum.is_integer());
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::blaze::FLOOR_MULTIPLE_ENUM_VARINT{
      .minimum = minimum.to_integer(),
      .multiplier = static_cast<std::uint64_t>(multiplier.to_integer())};
}

auto ROOF_MULTIPLE_MIRROR_ENUM_VARINT(const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
  assert(options.defines("maximum"));
  assert(options.defines("multiplier"));
  const auto &maximum{options.at("maximum")};
  const auto &multiplier{options.at("multiplier")};
  assert(maximum.is_integer());
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::blaze::ROOF_MULTIPLE_MIRROR_ENUM_VARINT{
      .maximum = maximum.to_integer(),
      .multiplier = static_cast<std::uint64_t>(multiplier.to_integer())};
}

auto ARBITRARY_MULTIPLE_ZIGZAG_VARINT(const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
  assert(options.defines("multiplier"));
  const auto &multiplier{options.at("multiplier")};
  assert(multiplier.is_integer());
  assert(multiplier.is_positive());
  return sourcemeta::blaze::ARBITRARY_MULTIPLE_ZIGZAG_VARINT{
      .multiplier = static_cast<std::uint64_t>(multiplier.to_integer())};
}

// NOLINTEND(readability-identifier-naming)
} // namespace sourcemeta::blaze::v1

#endif
