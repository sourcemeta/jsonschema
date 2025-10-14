#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <cstdint>  // std::uint32_t
#include <optional> // std::optional
#include <sstream>  // std::istringstream, std::ostringstream
#include <string>   // std::string

namespace sourcemeta::core {

namespace {

auto escape_component(std::string_view input, URIEscapeMode mode)
    -> std::string {
  std::istringstream in{std::string{input}};
  std::ostringstream out;
  uri_escape(in, out, mode, false);
  return out.str();
}

} // namespace

auto URI::recompose() const -> std::string {
  const auto uri = this->recompose_without_fragment();

  // Fragment
  if (!this->fragment_.has_value()) {
    return uri.value_or("");
  }

  std::ostringstream result;
  if (uri.has_value()) {
    result << uri.value();
  }

  result << '#';

  // Escape fragment using stream-based escaping
  // Don't preserve percent sequences since internal storage is fully decoded
  std::istringstream fragment_input{std::string{this->fragment_.value()}};
  uri_escape(fragment_input, result, URIEscapeMode::Fragment, false);

  return result.str();
}

auto URI::recompose_without_fragment() const -> std::optional<std::string> {
  std::ostringstream result;

  // Scheme
  const auto result_scheme{this->scheme()};
  if (result_scheme.has_value()) {
    result << result_scheme.value() << ":";
  }

  // Authority
  const auto user_info{this->userinfo()};
  const auto result_host{this->host()};
  const auto result_port{this->port()};
  const bool has_authority{user_info.has_value() || result_host.has_value() ||
                           result_port.has_value()};

  // Add "//" prefix when we have authority (with or without scheme)
  if (has_authority) {
    result << "//";
  }

  if (user_info.has_value()) {
    result << escape_component(user_info.value(), URIEscapeMode::Fragment)
           << "@";
  }

  // Host
  if (result_host.has_value()) {
    if (this->is_ipv6()) {
      // By default uriparser will parse the IPv6 address without brackets
      // so we need to add them manually, as said in the RFC 2732:
      // "To use a literal IPv6 address in a URL, the literal address should
      // be enclosed in "[" and "]" characters." See
      // https://tools.ietf.org/html/rfc2732#section-2
      result << '[' << result_host.value() << ']';
    } else {
      result << escape_component(result_host.value(),
                                 URIEscapeMode::SkipSubDelims);
    }
  }

  // Port
  if (result_port.has_value()) {
    result << ':' << result_port.value();
  }

  // Path
  const auto result_path = this->path();
  if (result_path.has_value()) {
    const auto &path_value = result_path.value();

    // RFC 3986: If there's a scheme but no authority, the path cannot start
    // with "//" to avoid confusion with network-path references
    // Strip the leading "/" in this case for paths like "g:h" (path should be
    // "h" not "/h")
    if (result_scheme.has_value() && !has_authority &&
        path_value.starts_with("/") && !path_value.starts_with("//")) {
      result << escape_component(path_value.substr(1), URIEscapeMode::Fragment);
    } else {
      result << escape_component(path_value, URIEscapeMode::Fragment);
    }
  }

  // Query
  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result << '?'
           << escape_component(result_query.value(), URIEscapeMode::Fragment);
  }

  if (result.tellp() == 0) {
    return std::nullopt;
  }

  return result.str();
}

} // namespace sourcemeta::core
