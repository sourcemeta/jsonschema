#include <sourcemeta/jsonschema/http.h>

#include <cassert>  // assert
#include <optional> // std::optional, std::nullopt
#include <sstream>  // std::ostringstream, std::istringstream
#include <string>   // std::string
#include <utility>  // std::move

namespace sourcemeta::jsonschema::http {

ClientResponse::ClientResponse(
    const Status status, std::unordered_map<std::string, std::string> &&headers,
    std::ostringstream &&stream)
    : status_{status}, headers_{std::move(headers)},
      stream_{std::move(stream).str()} {}

auto ClientResponse::status() const noexcept -> Status { return this->status_; }

auto ClientResponse::header(const std::string &key) const
    -> std::optional<std::string> {
  if (!this->headers_.contains(key)) {
    return std::nullopt;
  }

  return this->headers_.at(key);
}

auto ClientResponse::headers() const
    -> const std::unordered_map<std::string, std::string> & {
  return this->headers_;
}

auto ClientResponse::empty() noexcept -> bool {
  return this->stream_.peek() == -1;
}

auto ClientResponse::body() -> std::istringstream & {
  assert(!this->empty());
  return this->stream_;
}

} // namespace sourcemeta::jsonschema::http
