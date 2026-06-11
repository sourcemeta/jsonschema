#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "normalize.h"

#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::core {

auto URI::canonicalize() -> URI & {
  // Lowercase scheme (schemes are case-insensitive per RFC 3986)
  if (this->scheme_.has_value()) {
    sourcemeta::core::to_lowercase(this->scheme_.value());
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

  // Only unreserved characters may be percent-decoded during normalization
  // See https://www.rfc-editor.org/rfc/rfc3986#section-6.2.2.2
  if (this->path_.has_value()) {
    uri_normalize_percent_encoding_inplace(this->path_.value());
    uri_unescape_unreserved_inplace(this->path_.value());
  }

  if (this->query_.has_value()) {
    uri_normalize_percent_encoding_inplace(this->query_.value());
    uri_unescape_unreserved_inplace(this->query_.value());
  }

  if (this->fragment_.has_value()) {
    uri_normalize_percent_encoding_inplace(this->fragment_.value());
    uri_unescape_unreserved_inplace(this->fragment_.value());
  }

  if (this->userinfo_.has_value()) {
    uri_normalize_percent_encoding_inplace(this->userinfo_.value());
    uri_unescape_unreserved_inplace(this->userinfo_.value());
  }

  // Hostnames are case-insensitive per RFC 3986, and the lowercasing must come
  // after decoding so that a percent-encoded uppercase letter is also folded
  if (this->host_.has_value()) {
    uri_normalize_percent_encoding_inplace(this->host_.value());
    uri_unescape_unreserved_inplace(this->host_.value());
    sourcemeta::core::to_lowercase(this->host_.value());
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

auto URI::canonicalize(const std::string_view input) -> std::string {
  return URI{input}.canonicalize().recompose();
}

} // namespace sourcemeta::core
