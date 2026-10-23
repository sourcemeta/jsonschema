#ifndef SOURCEMETA_BLAZE_BINPACK_LOADER_V1_STRING_H_
#define SOURCEMETA_BLAZE_BINPACK_LOADER_V1_STRING_H_

#include <sourcemeta/blaze/binpack.h>

#include <sourcemeta/core/json.h>

#include <cassert> // assert
#include <cstdint> // std::uint64_t

namespace sourcemeta::blaze::v1 {

// The encoding names are the ones the JSON BinPack specification defines, so
// they keep their casing rather than following the C++ naming convention
// NOLINTBEGIN(readability-identifier-naming)

auto UTF8_STRING_NO_LENGTH(const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
  assert(options.defines("size"));
  const auto &size{options.at("size")};
  assert(size.is_integer());
  assert(size.is_positive());
  return sourcemeta::blaze::UTF8_STRING_NO_LENGTH{
      .size = static_cast<std::uint64_t>(size.to_integer())};
}

auto FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &options) -> BinPackEncoding {
  assert(options.defines("minimum"));
  const auto &minimum{options.at("minimum")};
  assert(minimum.is_integer());
  assert(minimum.is_positive());
  return sourcemeta::blaze::FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED{
      .minimum = static_cast<std::uint64_t>(minimum.to_integer())};
}

auto ROOF_VARINT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &options) -> BinPackEncoding {
  assert(options.defines("maximum"));
  const auto &maximum{options.at("maximum")};
  assert(maximum.is_integer());
  assert(maximum.is_positive());
  return sourcemeta::blaze::ROOF_VARINT_PREFIX_UTF8_STRING_SHARED{
      .maximum = static_cast<std::uint64_t>(maximum.to_integer())};
}

auto BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED(
    const sourcemeta::core::JSON &options) -> BinPackEncoding {
  assert(options.defines("minimum"));
  assert(options.defines("maximum"));
  const auto &minimum{options.at("minimum")};
  const auto &maximum{options.at("maximum")};
  assert(minimum.is_integer());
  assert(maximum.is_integer());
  assert(minimum.is_positive());
  assert(maximum.is_positive());
  return sourcemeta::blaze::BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED{
      .minimum = static_cast<std::uint64_t>(minimum.to_integer()),
      .maximum = static_cast<std::uint64_t>(maximum.to_integer())};
}

auto RFC3339_DATE_INTEGER_TRIPLET(
    [[maybe_unused]] const sourcemeta::core::JSON &options) -> BinPackEncoding {
  return sourcemeta::blaze::RFC3339_DATE_INTEGER_TRIPLET{};
}

auto PREFIX_VARINT_LENGTH_STRING_SHARED(
    [[maybe_unused]] const sourcemeta::core::JSON &options) -> BinPackEncoding {
  return sourcemeta::blaze::PREFIX_VARINT_LENGTH_STRING_SHARED{};
}

// NOLINTEND(readability-identifier-naming)
} // namespace sourcemeta::blaze::v1

#endif
