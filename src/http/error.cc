#include <sourcemeta/jsonschema/http_error.h>

#include <utility> // std::move

namespace sourcemeta::jsonschema::http {

Error::Error(std::string message) : message_{std::move(message)} {}

auto Error::what() const noexcept -> const char * {
  return this->message_.c_str();
}

} // namespace sourcemeta::jsonschema::http
