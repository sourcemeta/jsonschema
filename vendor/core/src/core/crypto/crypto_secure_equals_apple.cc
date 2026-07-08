#include <sourcemeta/core/crypto.h>

#include <cstring> // timingsafe_bcmp

#include <string_view> // std::string_view

namespace sourcemeta::core {

auto secure_equals(const std::string_view left,
                   const std::string_view right) noexcept -> bool {
  if (left.size() != right.size()) {
    return false;
  }

  // An empty view may hold a null data pointer, which is undefined to pass to
  // the C API even with a zero length
  if (left.empty()) {
    return true;
  }

  return timingsafe_bcmp(left.data(), right.data(), left.size()) == 0;
}

} // namespace sourcemeta::core
