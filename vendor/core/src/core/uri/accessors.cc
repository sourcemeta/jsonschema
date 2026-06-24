#include <sourcemeta/core/ip.h>
#include <sourcemeta/core/uri.h>

#include <cstdint>     // std::uint32_t
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto URI::is_relative() const -> bool { return !this->scheme().has_value(); }

auto URI::is_absolute() const noexcept -> bool {
  return this->scheme_.has_value();
}

auto URI::is_internationalized() const noexcept -> bool { return this->iri_; }

auto URI::is_urn() const -> bool {
  const auto scheme{this->scheme()};
  return scheme.has_value() && scheme.value() == "urn";
}

auto URI::is_tag() const -> bool {
  const auto scheme{this->scheme()};
  return scheme.has_value() && scheme.value() == "tag";
}

auto URI::is_mailto() const -> bool {
  const auto scheme{this->scheme()};
  return scheme.has_value() && scheme.value() == "mailto";
}

auto URI::is_file() const -> bool {
  const auto scheme{this->scheme()};
  return scheme.has_value() && scheme.value() == "file";
}

auto URI::is_ipv4() const -> bool {
  return this->host_.has_value() &&
         sourcemeta::core::is_ipv4(this->host_.value());
}

auto URI::is_ipv6() const -> bool {
  return this->host_.has_value() &&
         sourcemeta::core::is_ipv6(this->host_.value());
}

auto URI::is_fragment_only() const -> bool {
  return !this->scheme().has_value() && !this->host().has_value() &&
         !this->port().has_value() && !this->path().has_value() &&
         this->fragment().has_value() && !this->query().has_value();
}

auto URI::empty() const -> bool {
  return !this->path_.has_value() && !this->userinfo_.has_value() &&
         !this->host_.has_value() && !this->port_.has_value() &&
         !this->scheme_.has_value() && !this->fragment_.has_value() &&
         !this->query_.has_value();
}

auto URI::scheme() const -> std::optional<std::string_view> {
  return this->scheme_;
}

auto URI::host() const -> std::optional<std::string_view> {
  return this->host_;
}

auto URI::port() const -> std::optional<std::uint32_t> { return this->port_; }

auto URI::path() const -> std::optional<std::string_view> {
  if (this->path_.has_value()) {
    return this->path_.value();
  }

  return std::nullopt;
}

auto URI::fragment() const -> std::optional<std::string_view> {
  return this->fragment_;
}

auto URI::userinfo() const -> std::optional<std::string_view> {
  return this->userinfo_;
}

auto URI::has_same_authority(const URI &other) const noexcept -> bool {
  return this->userinfo_ == other.userinfo_ && this->host_ == other.host_ &&
         this->port_ == other.port_;
}

} // namespace sourcemeta::core
