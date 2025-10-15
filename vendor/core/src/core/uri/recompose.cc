#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <cctype>   // std::isxdigit
#include <cstdint>  // std::uint32_t
#include <iomanip>  // std::hex, std::uppercase
#include <optional> // std::optional
#include <sstream>  // std::ostringstream
#include <string>   // std::string

namespace sourcemeta::core {

namespace {

auto escape_component_to_string(std::string &output, std::string_view input,
                                const URIEscapeMode mode) -> void {
  output.reserve(output.size() + input.size() * 3);

  for (const char character : input) {
    if (uri_is_unreserved(character)) {
      output += character;
      continue;
    }

    if (mode == URIEscapeMode::SkipSubDelims ||
        mode == URIEscapeMode::Fragment || mode == URIEscapeMode::Filesystem) {
      if (uri_is_sub_delim(character)) {
        output += character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Fragment) {
      if (character == URI_COLON || character == URI_AT ||
          character == URI_SLASH || character == URI_QUESTION) {
        output += character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Filesystem) {
      if (character == URI_COLON) {
        output += character;
        continue;
      }
    }

    output += URI_PERCENT;
    const auto byte{static_cast<unsigned char>(character)};
    const auto high{(byte >> 4) & 0x0F};
    const auto low{byte & 0x0F};
    output += static_cast<char>(high < 10 ? '0' + high : 'A' + high - 10);
    output += static_cast<char>(low < 10 ? '0' + low : 'A' + low - 10);
  }
}

} // namespace

auto URI::recompose() const -> std::string {
  auto uri{this->recompose_without_fragment()};

  if (!this->fragment_.has_value()) {
    return uri.value_or("");
  }

  std::string result;
  if (uri.has_value()) {
    result = *std::move(uri);
  }

  result.reserve(result.size() + this->fragment_.value().size() * 3 + 1);
  result += '#';
  escape_component_to_string(result, this->fragment_.value(),
                             URIEscapeMode::Fragment);

  return result;
}

auto URI::recompose_without_fragment() const -> std::optional<std::string> {
  std::string result;
  result.reserve(256);

  // Scheme
  const auto result_scheme{this->scheme()};
  if (result_scheme.has_value()) {
    result += result_scheme.value();
    result += ':';
  }

  // Authority
  const auto user_info{this->userinfo()};
  const auto result_host{this->host()};
  const auto result_port{this->port()};
  const bool has_authority{user_info.has_value() || result_host.has_value() ||
                           result_port.has_value()};

  // Add "//" prefix when we have authority (with or without scheme)
  if (has_authority) {
    result += "//";
  }

  if (user_info.has_value()) {
    escape_component_to_string(result, user_info.value(),
                               URIEscapeMode::Fragment);
    result += '@';
  }

  // Host
  if (result_host.has_value()) {
    if (this->is_ipv6()) {
      result += '[';
      result += result_host.value();
      result += ']';
    } else {
      escape_component_to_string(result, result_host.value(),
                                 URIEscapeMode::SkipSubDelims);
    }
  }

  // Port
  if (result_port.has_value()) {
    result += ':';
    result += std::to_string(result_port.value());
  }

  // Path
  const auto result_path = this->path();
  if (result_path.has_value()) {
    const auto &path_value = result_path.value();

    if (result_scheme.has_value() && !has_authority &&
        path_value.starts_with("/") && !path_value.starts_with("//")) {
      escape_component_to_string(result, path_value.substr(1),
                                 URIEscapeMode::Fragment);
    } else {
      escape_component_to_string(result, path_value, URIEscapeMode::Fragment);
    }
  }

  // Query
  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result += '?';
    escape_component_to_string(result, result_query.value(),
                               URIEscapeMode::Fragment);
  }

  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

} // namespace sourcemeta::core
