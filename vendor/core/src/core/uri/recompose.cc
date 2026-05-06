#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <array>    // std::array
#include <cctype>   // std::isxdigit
#include <charconv> // std::to_chars
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

  for (std::string_view::size_type index = 0; index < input.size(); ++index) {
    const char character = input[index];

    // Preserve existing percent-encoded sequences
    if (character == URI_PERCENT && index + 2 < input.size() &&
        std::isxdigit(static_cast<unsigned char>(input[index + 1])) &&
        std::isxdigit(static_cast<unsigned char>(input[index + 2]))) {
      output += input[index];
      output += input[index + 1];
      output += input[index + 2];
      index += 2;
      continue;
    }

    if (uri_is_unreserved(character)) {
      output += character;
      continue;
    }

    if (mode == URIEscapeMode::SkipSubDelims || mode == URIEscapeMode::Path ||
        mode == URIEscapeMode::Fragment || mode == URIEscapeMode::Filesystem ||
        mode == URIEscapeMode::UserInfo) {
      if (uri_is_sub_delim(character)) {
        output += character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Path) {
      if (character == URI_COLON || character == URI_AT ||
          character == URI_SLASH) {
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

    if (mode == URIEscapeMode::Filesystem || mode == URIEscapeMode::UserInfo) {
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
                               URIEscapeMode::UserInfo);
    result += '@';
  }

  // Host
  if (result_host.has_value()) {
    if (this->ip_literal_) {
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
    std::array<char, 20> port_buffer{};
    const auto [end_pointer, error_code] = std::to_chars(
        port_buffer.data(), port_buffer.data() + port_buffer.size(),
        result_port.value());
    result.append(port_buffer.data(), end_pointer);
  }

  // Path
  const auto result_path = this->path();
  if (result_path.has_value()) {
    const auto &path_value = result_path.value();

    escape_component_to_string(result, path_value, URIEscapeMode::Path);
  }

  // Query
  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result += '?';
    escape_component_to_string(result, result_query.value().raw(),
                               URIEscapeMode::Fragment);
  }

  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

} // namespace sourcemeta::core
