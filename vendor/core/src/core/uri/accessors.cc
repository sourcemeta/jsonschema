#include <sourcemeta/core/uri.h>

#include <cstdint>  // std::uint32_t
#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::core {

auto URI::is_relative() const -> bool {
  return !this->scheme().has_value() ||
         (this->path_.has_value() && this->path_.value().starts_with("."));
}

auto URI::is_absolute() const noexcept -> bool {
  return this->scheme_.has_value();
}

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

auto URI::is_ipv6() const -> bool {
  return this->host_.has_value() &&
         this->host_.value().find(':') != std::string::npos;
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

auto URI::path() const -> std::optional<std::string> { return this->path_; }

auto URI::fragment() const -> std::optional<std::string_view> {
  return this->fragment_;
}

auto URI::query() const -> std::optional<std::string_view> {
  return this->query_;
}

auto URI::userinfo() const -> std::optional<std::string_view> {
  return this->userinfo_;
}

} // namespace sourcemeta::core
