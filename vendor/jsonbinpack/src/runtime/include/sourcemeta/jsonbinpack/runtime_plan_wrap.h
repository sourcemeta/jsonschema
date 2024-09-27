#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_ENCODING_WRAP_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_ENCODING_WRAP_H_

#include <sourcemeta/jsonbinpack/runtime_plan.h>

#include <algorithm>        // std::transform
#include <initializer_list> // std::initializer_list
#include <iterator>         // std::back_inserter, std::forward_iterator
#include <memory>           // std::make_shared
#include <utility>          // std::move, std::forward
#include <vector>           // std::vector

namespace {

// Adapter to make smart pointers using struct aggregate initialization
// From https://stackoverflow.com/a/35300172/1641422
template <typename T> struct aggregate_adapter : public T {
  template <typename... Args>
  aggregate_adapter(Args &&...args) : T{std::forward<Args>(args)...} {}
};

} // namespace

namespace sourcemeta::jsonbinpack {

// We define all wrap helper functions inline in this header file,
// as we want to avoid including their definitions in if the user
// doesn't import this header.

inline auto wrap(Plan &&encoding) -> SinglePlan {
  return std::make_shared<aggregate_adapter<__internal_encoding_wrapper>>(
      std::move(encoding));
}

// clang-format off
template <typename Iterator>
requires std::forward_iterator<Iterator>
// clang-format on
inline auto wrap(Iterator begin, Iterator end) -> MultiplePlans {
  MultiplePlans result;
  std::transform(begin, end, std::back_inserter(result), [](Plan encoding) {
    return __internal_encoding_wrapper{std::move(encoding)};
  });
  return result;
}

inline auto wrap(std::initializer_list<Plan> encodings) -> MultiplePlans {
  return wrap(encodings.begin(), encodings.end());
}

} // namespace sourcemeta::jsonbinpack

#endif
