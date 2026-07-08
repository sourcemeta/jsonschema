#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"

#include <array>       // std::array
#include <charconv>    // std::to_chars
#include <cstdint>     // std::uint32_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

auto escape_component_to_string(std::string &output, std::string_view input,
                                const URIEscapeMode mode, const bool iri,
                                const bool allow_iprivate = false) -> void {
  output.reserve(output.size() + input.size() * 3);

  for (std::string_view::size_type index = 0; index < input.size(); ++index) {
    const char character = input[index];

    // Preserve existing percent-encoded sequences
    if (character == URI_PERCENT && index + 2 < input.size() &&
        hex_digit_value(input[index + 1]) >= 0 &&
        hex_digit_value(input[index + 2]) >= 0) {
      output += input[index];
      output += input[index + 1];
      output += input[index + 2];
      index += 2;
      continue;
    }

    // In IRI mode, the non-ASCII characters permitted by RFC 3987, including
    // private-use characters within the query, are preserved literally rather
    // than percent-encoded
    if (iri && (static_cast<unsigned char>(character) & 0x80U) != 0U) {
      const auto decoded{sourcemeta::core::utf8_decode(input, index)};
      if (decoded.has_value() &&
          (sourcemeta::core::is_ucschar(decoded.value().first) ||
           (allow_iprivate &&
            sourcemeta::core::is_iprivate(decoded.value().first)))) {
        output.append(input.substr(index, decoded.value().second));
        index += decoded.value().second - 1;
        continue;
      }
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

auto recompose_authority(std::string &output,
                         const std::optional<std::string_view> user_info,
                         const std::optional<std::string_view> host,
                         const std::optional<std::uint32_t> port,
                         const bool ip_literal, const bool iri) -> void {
  if (user_info.has_value()) {
    escape_component_to_string(output, user_info.value(),
                               URIEscapeMode::UserInfo, iri);
    output += '@';
  }

  if (host.has_value()) {
    if (ip_literal) {
      output += '[';
      output += host.value();
      output += ']';
    } else {
      escape_component_to_string(output, host.value(),
                                 URIEscapeMode::SkipSubDelims, iri);
    }
  }

  if (port.has_value()) {
    output += ':';
    std::array<char, 20> port_buffer{};
    const auto [end_pointer, error_code] =
        std::to_chars(port_buffer.data(),
                      port_buffer.data() + port_buffer.size(), port.value());
    output.append(port_buffer.data(), end_pointer);
  }
}

// Emit a path, applying the RFC 3986 Section 4.2 protections that stop it from
// re-parsing as something else: a path-noscheme first segment must not contain
// a colon or it looks like a scheme, and a path must not begin with "//" when
// no authority precedes it or it looks like an authority
auto append_disambiguated_path(std::string &output,
                               const std::string_view path_value,
                               const bool has_scheme, const bool has_authority,
                               const bool iri) -> void {
  if (!has_authority && path_value.starts_with("//")) {
    output += "/.";
  }

  if (!has_scheme && !has_authority && !path_value.starts_with('/')) {
    const auto first_slash{path_value.find('/')};
    const auto first_segment_length{first_slash == std::string_view::npos
                                        ? path_value.size()
                                        : first_slash};
    const auto first_segment{path_value.substr(0, first_segment_length)};
    if (first_segment.find(':') != std::string_view::npos) {
      std::string encoded;
      encoded.reserve(first_segment_length + 4);
      for (const char character : first_segment) {
        if (character == ':') {
          encoded += "%3A";
        } else {
          encoded += character;
        }
      }

      escape_component_to_string(output, encoded, URIEscapeMode::Path, iri);
      if (first_slash != std::string_view::npos) {
        escape_component_to_string(output, path_value.substr(first_slash),
                                   URIEscapeMode::Path, iri);
      }

      return;
    }
  }

  escape_component_to_string(output, path_value, URIEscapeMode::Path, iri);
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
                             URIEscapeMode::Fragment, this->iri_);

  return result;
}

auto URI::recompose_relative() const -> std::string {
  std::string result;
  result.reserve(128);

  const auto result_path{this->path()};
  if (result_path.has_value()) {
    append_disambiguated_path(result, result_path.value(), false, false,
                              this->iri_);
  }

  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result += '?';
    escape_component_to_string(result, result_query.value().raw(),
                               URIEscapeMode::Fragment, this->iri_, true);
  }

  if (this->fragment_.has_value()) {
    result += '#';
    escape_component_to_string(result, this->fragment_.value(),
                               URIEscapeMode::Fragment, this->iri_);
  }

  return result;
}

auto URI::authority() const -> std::optional<std::string> {
  if (!this->userinfo().has_value() && !this->host().has_value() &&
      !this->port().has_value()) {
    return std::nullopt;
  }

  std::string result;
  recompose_authority(result, this->userinfo(), this->host(), this->port(),
                      this->ip_literal_, this->iri_);
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
  const bool has_authority{this->userinfo().has_value() ||
                           this->host().has_value() ||
                           this->port().has_value()};
  if (has_authority) {
    result += "//";
    recompose_authority(result, this->userinfo(), this->host(), this->port(),
                        this->ip_literal_, this->iri_);
  }

  // Path
  const auto result_path = this->path();
  if (result_path.has_value()) {
    append_disambiguated_path(result, result_path.value(),
                              result_scheme.has_value(), has_authority,
                              this->iri_);
  }

  // Query
  const auto result_query{this->query()};
  if (result_query.has_value()) {
    result += '?';
    escape_component_to_string(result, result_query.value().raw(),
                               URIEscapeMode::Fragment, this->iri_, true);
  }

  if (result.empty()) {
    return std::nullopt;
  }

  return result;
}

} // namespace sourcemeta::core
