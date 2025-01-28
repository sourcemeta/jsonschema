#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_ANY_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_LOADER_V1_ANY_H_

#include <sourcemeta/jsonbinpack/runtime.h>

#include <sourcemeta/core/json.h>

#include <cassert> // assert
#include <utility> // std::move
#include <vector>  // std::vector

namespace sourcemeta::jsonbinpack::v1 {

auto BYTE_CHOICE_INDEX(const sourcemeta::core::JSON &options) -> Encoding {
  assert(options.defines("choices"));
  const auto &choices{options.at("choices")};
  assert(choices.is_array());
  const auto &array{choices.as_array()};
  std::vector<sourcemeta::core::JSON> elements{array.cbegin(), array.cend()};
  return sourcemeta::jsonbinpack::BYTE_CHOICE_INDEX({std::move(elements)});
}

auto LARGE_CHOICE_INDEX(const sourcemeta::core::JSON &options) -> Encoding {
  assert(options.defines("choices"));
  const auto &choices{options.at("choices")};
  assert(choices.is_array());
  const auto &array{choices.as_array()};
  std::vector<sourcemeta::core::JSON> elements{array.cbegin(), array.cend()};
  return sourcemeta::jsonbinpack::LARGE_CHOICE_INDEX({std::move(elements)});
}

auto TOP_LEVEL_BYTE_CHOICE_INDEX(const sourcemeta::core::JSON &options)
    -> Encoding {
  assert(options.defines("choices"));
  const auto &choices{options.at("choices")};
  assert(choices.is_array());
  const auto &array{choices.as_array()};
  std::vector<sourcemeta::core::JSON> elements{array.cbegin(), array.cend()};
  return sourcemeta::jsonbinpack::TOP_LEVEL_BYTE_CHOICE_INDEX(
      {std::move(elements)});
}

auto CONST_NONE(const sourcemeta::core::JSON &options) -> Encoding {
  assert(options.defines("value"));
  return sourcemeta::jsonbinpack::CONST_NONE({options.at("value")});
}

auto ANY_PACKED_TYPE_TAG_BYTE_PREFIX(const sourcemeta::core::JSON &)
    -> Encoding {
  return sourcemeta::jsonbinpack::ANY_PACKED_TYPE_TAG_BYTE_PREFIX{};
}

} // namespace sourcemeta::jsonbinpack::v1

#endif
