#include <sourcemeta/hydra/http_error.h>

#include <utility> // std::move

namespace sourcemeta::hydra::http {

Error::Error(std::string message) : message_{std::move(message)} {}

auto Error::what() const noexcept -> const char * {
  return this->message_.c_str();
}

} // namespace sourcemeta::hydra::http
