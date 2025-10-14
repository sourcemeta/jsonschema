#include <sourcemeta/core/uri.h>

#include "normalize.h"

#include <cctype>   // std::tolower
#include <cstdint>  // std::uint32_t
#include <optional> // std::optional
#include <string>   // std::string

namespace {

auto to_lowercase(std::string_view input) -> std::string {
  std::string result;
  result.reserve(input.size());
  for (const auto character : input) {
    result +=
        static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return result;
}

} // namespace

namespace sourcemeta::core {

auto URI::canonicalize() -> URI & {
  // Lowercase scheme (schemes are case-insensitive per RFC 3986)
  if (this->scheme_.has_value()) {
    this->scheme_ = to_lowercase(this->scheme_.value());
  }

  // Lowercase host (hostnames are case-insensitive per RFC 3986)
  if (this->host_.has_value()) {
    this->host_ = to_lowercase(this->host_.value());
  }

  // Canonicalize path by removing "." and ".." segments
  if (this->path_.has_value() && !this->path_.value().empty()) {
    auto &current_path{this->path_.value()};
    normalize_path(current_path);
    if (current_path.empty()) {
      this->path_ = std::nullopt;
    }
  }

  // Remove empty fragment (empty fragments are optional per RFC 3986)
  if (this->fragment_.has_value() && this->fragment_.value().empty()) {
    this->fragment_ = std::nullopt;
  }

  // Remove default ports (80 for http, 443 for https)
  if (this->port_.has_value() && this->scheme_.has_value()) {
    const auto port_value = this->port_.value();
    const auto scheme_value = this->scheme_.value();

    if ((scheme_value == "http" && port_value == 80) ||
        (scheme_value == "https" && port_value == 443)) {
      this->port_ = std::nullopt;
    }
  }

  return *this;
}

auto URI::canonicalize(const std::string &input) -> std::string {
  return URI{input}.canonicalize().recompose();
}

} // namespace sourcemeta::core
