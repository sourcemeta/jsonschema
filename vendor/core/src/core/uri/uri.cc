#include <sourcemeta/core/uri.h>

#include <sstream> // std::ostringstream
#include <tuple>   // std::tie
#include <utility> // std::move

namespace sourcemeta::core {

URI::URI(const std::string &input) { this->parse(input); }

URI::URI(std::istream &input) {
  std::ostringstream output;
  output << input.rdbuf();
  this->parse(output.str());
}

auto URI::from_fragment(std::string_view fragment) -> URI {
  URI result;
  result.fragment(fragment);
  return result;
}

auto URI::operator<(const URI &other) const noexcept -> bool {
  return std::tie(this->scheme_, this->userinfo_, this->host_, this->port_,
                  this->path_, this->query_, this->fragment_) <
         std::tie(other.scheme_, other.userinfo_, other.host_, other.port_,
                  other.path_, other.query_, other.fragment_);
}

} // namespace sourcemeta::core
