#include <sourcemeta/core/io.h>
#include <sourcemeta/core/uri.h>

#include <tuple> // std::tie

namespace sourcemeta::core {

URI::URI(std::istream &input) {
  this->parse(sourcemeta::core::read_to_string(input));
}

auto URI::from_fragment(const std::string_view fragment) -> URI {
  URI result;
  result.fragment(fragment);
  return result;
}

auto URI::from_iri(const std::string_view input) -> URI {
  URI result;
  result.iri_ = true;
  result.parse(input);
  return result;
}

auto URI::operator==(const URI &other) const noexcept -> bool {
  return std::tie(this->scheme_, this->userinfo_, this->host_, this->port_,
                  this->path_, this->query_, this->fragment_,
                  this->ip_literal_) ==
         std::tie(other.scheme_, other.userinfo_, other.host_, other.port_,
                  other.path_, other.query_, other.fragment_,
                  other.ip_literal_);
}

auto URI::operator<(const URI &other) const noexcept -> bool {
  return std::tie(this->scheme_, this->userinfo_, this->host_, this->port_,
                  this->path_, this->query_, this->fragment_,
                  this->ip_literal_) <
         std::tie(other.scheme_, other.userinfo_, other.host_, other.port_,
                  other.path_, other.query_, other.fragment_,
                  other.ip_literal_);
}

} // namespace sourcemeta::core
