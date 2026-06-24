#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "normalize.h"

#include <array>    // std::array
#include <cstdint>  // std::uint8_t
#include <optional> // std::optional
#include <string>   // std::string

namespace sourcemeta::core {

namespace {

// Percent-decode the octets that an IRI may carry unencoded: the URI unreserved
// characters plus the non-ASCII characters permitted by RFC 3987. A
// percent-encoded ASCII unreserved octet is decoded on its own. A run of
// percent-encoded octets that together form a valid non-ASCII UTF-8 character
// is decoded to its literal bytes. Everything else, including private-use and
// reserved characters, is left percent-encoded.
// See https://www.rfc-editor.org/rfc/rfc3987#section-5.3.2.3
auto unescape_iunreserved_inplace(std::string &input) -> void {
  std::string output;
  output.reserve(input.size());

  for (std::string::size_type position{0}; position < input.size();) {
    if (!uri_is_percent_encoded(input, position)) {
      output += input[position];
      position += 1;
      continue;
    }

    const auto lead{
        static_cast<unsigned char>((uri_hex_to_int(input[position + 1]) << 4) |
                                   uri_hex_to_int(input[position + 2]))};
    const auto length{sourcemeta::core::utf8_lead_byte_size(lead)};

    if (length == 1) {
      if (uri_is_unreserved(static_cast<char>(lead))) {
        output += static_cast<char>(lead);
      } else {
        output += input[position];
        output += input[position + 1];
        output += input[position + 2];
      }
      position += 3;
      continue;
    }

    std::array<unsigned char, 4> bytes{};
    bytes[0] = lead;
    char32_t codepoint{0};
    if (length == 2) {
      codepoint = static_cast<char32_t>(lead & 0x1FU);
    } else if (length == 3) {
      codepoint = static_cast<char32_t>(lead & 0x0FU);
    } else {
      codepoint = static_cast<char32_t>(lead & 0x07U);
    }

    bool decodable{length >= 2 && length <= 4};
    for (std::uint8_t offset{1}; decodable && offset < length; offset += 1) {
      const auto continuation_position{
          position + (static_cast<std::string::size_type>(offset) * 3)};
      if (!uri_is_percent_encoded(input, continuation_position)) {
        decodable = false;
        break;
      }
      const auto continuation{static_cast<unsigned char>(
          (uri_hex_to_int(input[continuation_position + 1]) << 4) |
          uri_hex_to_int(input[continuation_position + 2]))};
      if (!sourcemeta::core::is_utf8_continuation(continuation)) {
        decodable = false;
        break;
      }
      bytes[offset] = continuation;
      codepoint =
          (codepoint << 6) | static_cast<char32_t>(continuation & 0x3FU);
    }

    if (decodable && sourcemeta::core::is_valid_codepoint(codepoint) &&
        sourcemeta::core::utf8_codepoint_byte_count(codepoint) == length &&
        sourcemeta::core::is_ucschar(codepoint)) {
      for (std::uint8_t offset{0}; offset < length; offset += 1) {
        output += static_cast<char>(bytes[offset]);
      }
      position += static_cast<std::string::size_type>(length) * 3;
    } else {
      output += input[position];
      output += input[position + 1];
      output += input[position + 2];
      position += 3;
    }
  }

  input = std::move(output);
}

auto normalize_component(std::string &component, const bool iri) -> void {
  uri_normalize_percent_encoding_inplace(component);
  if (iri) {
    unescape_iunreserved_inplace(component);
  } else {
    uri_unescape_unreserved_inplace(component);
  }
}

} // namespace

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

  // Percent-decode octets that form unreserved characters, or, for an IRI, the
  // wider set that also includes the non-ASCII characters permitted by RFC 3987
  // See https://www.rfc-editor.org/rfc/rfc3986#section-6.2.2.2
  // See https://www.rfc-editor.org/rfc/rfc3987#section-5.3.2.3
  if (this->path_.has_value()) {
    normalize_component(this->path_.value(), this->iri_);
  }

  if (this->query_.has_value()) {
    normalize_component(this->query_.value(), this->iri_);
  }

  if (this->fragment_.has_value()) {
    normalize_component(this->fragment_.value(), this->iri_);
  }

  if (this->userinfo_.has_value()) {
    normalize_component(this->userinfo_.value(), this->iri_);
  }

  // Hostnames are case-insensitive per RFC 3986, and the lowercasing must come
  // after decoding so that a percent-encoded uppercase letter is also folded
  if (this->host_.has_value()) {
    normalize_component(this->host_.value(), this->iri_);
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
