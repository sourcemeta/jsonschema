#ifndef SOURCEMETA_BLAZE_BINPACK_LOADER_V1_NUMBER_H_
#define SOURCEMETA_BLAZE_BINPACK_LOADER_V1_NUMBER_H_

#include <sourcemeta/blaze/binpack.h>

#include <sourcemeta/core/json.h>

namespace sourcemeta::blaze::v1 {

// The encoding names are the ones the JSON BinPack specification defines, so
// they keep their casing rather than following the C++ naming convention
// NOLINTBEGIN(readability-identifier-naming)

auto DOUBLE_VARINT_TUPLE([[maybe_unused]] const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
  return sourcemeta::blaze::DOUBLE_VARINT_TUPLE{};
}

auto NUMBER_VARINT_TUPLE([[maybe_unused]] const sourcemeta::core::JSON &options)
    -> BinPackEncoding {
  return sourcemeta::blaze::NUMBER_VARINT_TUPLE{};
}

// NOLINTEND(readability-identifier-naming)
} // namespace sourcemeta::blaze::v1

#endif
