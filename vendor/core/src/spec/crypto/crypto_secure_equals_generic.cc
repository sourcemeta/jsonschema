#include <sourcemeta/core/crypto.h>

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto secure_equals(const std::string_view left,
                   const std::string_view right) noexcept -> bool {
  if (left.size() != right.size()) {
    return false;
  }

  // Accumulate the difference across every byte rather than returning on the
  // first mismatch, so that the running time depends only on the length. The
  // accumulator is volatile so that the compiler cannot reintroduce a
  // short-circuit that would leak the position of the first difference
  volatile unsigned char difference{0};
  for (std::size_t index = 0; index < left.size(); ++index) {
    difference = static_cast<unsigned char>(
        difference | (static_cast<unsigned char>(left[index]) ^
                      static_cast<unsigned char>(right[index])));
  }

  return difference == 0;
}

} // namespace sourcemeta::core
