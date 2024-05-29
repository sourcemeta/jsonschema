#include <sourcemeta/hydra/bucket_error.h>

#include <utility> // std::move

namespace sourcemeta::hydra {

BucketError::BucketError(std::string message) : message_{std::move(message)} {}

auto BucketError::what() const noexcept -> const char * {
  return this->message_.c_str();
}

} // namespace sourcemeta::hydra
