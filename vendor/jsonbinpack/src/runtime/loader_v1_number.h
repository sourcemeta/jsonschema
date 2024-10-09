#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_NUMBER_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_NUMBER_H_

#include <sourcemeta/jsonbinpack/runtime.h>

#include <sourcemeta/jsontoolkit/json.h>

namespace sourcemeta::jsonbinpack::v1 {

auto DOUBLE_VARINT_TUPLE(const sourcemeta::jsontoolkit::JSON &) -> Encoding {
  return sourcemeta::jsonbinpack::DOUBLE_VARINT_TUPLE{};
}

} // namespace sourcemeta::jsonbinpack::v1

#endif
