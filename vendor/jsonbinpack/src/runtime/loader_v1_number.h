#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_NUMBER_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_NUMBER_H_

#include <sourcemeta/jsonbinpack/runtime.h>

#include <sourcemeta/core/json.h>

namespace sourcemeta::jsonbinpack::v1 {

// The encoding names are the ones the JSON BinPack specification defines, so
// they keep their casing rather than following the C++ naming convention
// NOLINTBEGIN(readability-identifier-naming)

auto DOUBLE_VARINT_TUPLE([[maybe_unused]] const sourcemeta::core::JSON &options)
    -> Encoding {
  return sourcemeta::jsonbinpack::DOUBLE_VARINT_TUPLE{};
}

// NOLINTEND(readability-identifier-naming)
} // namespace sourcemeta::jsonbinpack::v1

#endif
