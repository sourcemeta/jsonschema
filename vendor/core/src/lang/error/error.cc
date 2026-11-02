#include <sourcemeta/core/error.h>

namespace sourcemeta::core {

auto Error::what() const noexcept -> const char * {
  return this->message_.data();
}

} // namespace sourcemeta::core
