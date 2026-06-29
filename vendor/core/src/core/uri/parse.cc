#include <sourcemeta/core/ip.h>
#include <sourcemeta/core/unicode.h>
#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "grammar.h"

#include <array>    // std::array
#include <cassert>  // assert
#include <cctype>   // std::isalnum, std::isxdigit, std::isalpha, std::isdigit
#include <charconv> // std::from_chars
#include <cstdint>  // std::uint64_t
#include <limits>   // std::numeric_limits
#include <optional> // std::optional
#include <string>   // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::errc
#include <type_traits>  // std::conditional_t
#include <utility>      // std::pair

namespace {

using namespace sourcemeta::core;

auto validate_percent_encoded_utf8(const std::string_view input,
                                   std::string_view::size_type position)
    -> std::string_view::size_type {
  assert(input[position] == URI_PERCENT);

  if (position + 2 >= input.size()) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }

  const auto first_hex = static_cast<unsigned char>(input[position + 1]);
  const auto second_hex = static_cast<unsigned char>(input[position + 2]);

  if (!std::isxdigit(first_hex) || !std::isxdigit(second_hex)) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }
  return 3;
}

[[maybe_unused]] auto
decode_utf8_codepoint(const std::string_view input,
                      const std::string_view::size_type position)
    -> std::pair<char32_t, std::uint8_t> {
  const auto decoded{sourcemeta::core::utf8_decode(input, position)};
  if (!decoded.has_value()) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }

  return {decoded.value().first,
          static_cast<std::uint8_t>(decoded.value().second)};
}

template <bool IRI, bool AllowIPrivate = false>
auto accept_iri_extension(const std::string_view input,
                          std::string_view::size_type &position) -> bool {
  if constexpr (!IRI) {
    return false;
  } else {
    if ((static_cast<unsigned char>(input[position]) & 0x80U) == 0U) {
      return false;
    }
    const auto [codepoint, length] = decode_utf8_codepoint(input, position);
    if (sourcemeta::core::is_ucschar(codepoint)) {
      position += length;
      return true;
    }
    if constexpr (AllowIPrivate) {
      if (sourcemeta::core::is_iprivate(codepoint)) {
        position += length;
        return true;
      }
    }
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }
}

template <bool CheckOnly>
auto parse_scheme(const std::string_view input,
                  std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, bool, std::optional<std::string>> {
  if (position >= input.size() ||
      !std::isalpha(static_cast<unsigned char>(input[position]))) {
    if constexpr (CheckOnly) {
      return false;
    } else {
      return std::nullopt;
    }
  }

  const auto start = position;
  position += 1;

  while (position < input.size() && uri_is_scheme_char(input[position])) {
    position += 1;
  }

  if (position < input.size() && input[position] == URI_COLON) {
    if constexpr (CheckOnly) {
      position += 1;
      return true;
    } else {
      std::string scheme{input.substr(start, position - start)};
      position += 1;
      return scheme;
    }
  }

  position = start;
  if constexpr (CheckOnly) {
    return false;
  } else {
    return std::nullopt;
  }
}

template <bool CheckOnly>
auto parse_port(const std::string_view input,
                std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, bool, std::optional<unsigned long>> {
  if (position >= input.size() ||
      !std::isdigit(static_cast<unsigned char>(input[position]))) {
    if constexpr (CheckOnly) {
      return false;
    } else {
      return std::nullopt;
    }
  }

  const auto start = position;
  while (position < input.size() &&
         std::isdigit(static_cast<unsigned char>(input[position]))) {
    position += 1;
  }

  if constexpr (CheckOnly) {
    return true;
  } else {
    const auto port_view = input.substr(start, position - start);
    unsigned long port_value{};
    const auto result = std::from_chars(
        port_view.data(), port_view.data() + port_view.size(), port_value);
    if (result.ec != std::errc{}) [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(start + 1)};
    }

    return port_value;
  }
}

template <bool CheckOnly>
auto parse_ipv6(const std::string_view input,
                std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, void, std::string> {
  assert(input[position] == URI_OPEN_BRACKET);

  const auto start = position;
  position += 1;
  const bool is_ipvfuture = position < input.size() &&
                            (input[position] == 'v' || input[position] == 'V');

  // RFC 3986: IP-literal = "[" ( IPv6address / IPvFuture ) "]"
  if (is_ipvfuture) {
    // IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    position += 1;

    // Require 1*HEXDIG for the version
    if (position >= input.size() || input[position] == URI_CLOSE_BRACKET ||
        !std::isxdigit(static_cast<unsigned char>(input[position])))
        [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
    while (position < input.size() && input[position] != URI_CLOSE_BRACKET &&
           std::isxdigit(static_cast<unsigned char>(input[position]))) {
      position += 1;
    }

    // Require "." separator
    if (position >= input.size() || input[position] != URI_DOT) [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
    position += 1;

    // Require 1*( unreserved / sub-delims / ":" )
    if (position >= input.size() || input[position] == URI_CLOSE_BRACKET)
        [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
    while (position < input.size() && input[position] != URI_CLOSE_BRACKET) {
      const auto current = input[position];
      if (!uri_is_unreserved(current) && !uri_is_sub_delim(current) &&
          current != URI_COLON) [[unlikely]] {
        throw sourcemeta::core::URIParseError{
            static_cast<std::uint64_t>(position + 1)};
      }
      position += 1;
    }
  } else {
    // IPv6address: only HEXDIG, ":", and "." are valid
    while (position < input.size() && input[position] != URI_CLOSE_BRACKET) {
      const auto current = input[position];
      if (!std::isxdigit(static_cast<unsigned char>(current)) &&
          current != URI_COLON && current != URI_DOT) [[unlikely]] {
        throw sourcemeta::core::URIParseError{
            static_cast<std::uint64_t>(position + 1)};
      }
      position += 1;
    }
  }

  if (position >= input.size()) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(start + 1)};
  }

  const auto literal{input.substr(start + 1, position - start - 1)};
  if (!is_ipvfuture && !sourcemeta::core::is_ipv6(literal)) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(start + 1)};
  }

  if constexpr (CheckOnly) {
    position += 1;
  } else {
    std::string ipv6{literal};
    position += 1;
    return ipv6;
  }
}

template <bool CheckOnly, bool IRI>
auto parse_host(const std::string_view input,
                std::string_view::size_type &position,
                [[maybe_unused]] bool &ip_literal)
    -> std::conditional_t<CheckOnly, void, std::string> {
  if (position >= input.size()) {
    if constexpr (!CheckOnly) {
      return std::string{};
    } else {
      return;
    }
  }

  if (input[position] == URI_OPEN_BRACKET) {
    ip_literal = true;
    if constexpr (CheckOnly) {
      parse_ipv6<true>(input, position);
      return;
    } else {
      return parse_ipv6<false>(input, position);
    }
  }

  const auto start = position;
  while (position < input.size()) {
    const auto current = input[position];
    if (current == URI_COLON || current == URI_SLASH ||
        current == URI_QUESTION || current == URI_HASH || current == URI_AT) {
      break;
    }

    if (current == URI_PERCENT) {
      const auto skip = validate_percent_encoded_utf8(input, position);
      position += skip;
    } else if (uri_is_unreserved(current) || uri_is_sub_delim(current)) {
      position += 1;
    } else if (!accept_iri_extension<IRI>(input, position)) [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
  }

  if constexpr (!CheckOnly) {
    if (position == start) {
      return std::string{};
    }

    return std::string{input.substr(start, position - start)};
  }
}

template <bool CheckOnly, bool IRI>
auto parse_userinfo(const std::string_view input,
                    std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, bool, std::optional<std::string>> {
  const auto start = position;
  while (position < input.size()) {
    const auto current = input[position];
    if (current == URI_AT) {
      if constexpr (CheckOnly) {
        position += 1;
        return true;
      } else {
        std::string userinfo{input.substr(start, position - start)};
        position += 1;
        return userinfo;
      }
    }

    if (current == URI_PERCENT) {
      const auto skip = validate_percent_encoded_utf8(input, position);
      position += skip;
    } else if (uri_is_unreserved(current) || uri_is_sub_delim(current) ||
               current == URI_COLON) {
      position += 1;
    } else if constexpr (IRI) {
      if ((static_cast<unsigned char>(current) & 0x80U) == 0U) {
        break;
      }
      const auto [codepoint, length] = decode_utf8_codepoint(input, position);
      if (!sourcemeta::core::is_ucschar(codepoint)) {
        break;
      }
      position += length;
    } else {
      break;
    }
  }

  position = start;
  if constexpr (CheckOnly) {
    return false;
  } else {
    return std::nullopt;
  }
}

template <bool CheckOnly, bool IRI>
auto parse_path(const std::string_view input,
                std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, bool, std::optional<std::string>> {
  if (position >= input.size()) {
    if constexpr (CheckOnly) {
      return false;
    } else {
      return std::nullopt;
    }
  }

  const auto first_char = input[position];
  if (first_char == URI_QUESTION || first_char == URI_HASH) {
    if constexpr (CheckOnly) {
      return false;
    } else {
      return std::nullopt;
    }
  }

  const auto start = position;
  while (position < input.size()) {
    const auto current = input[position];
    if (current == URI_QUESTION || current == URI_HASH) {
      break;
    }

    if (current == URI_PERCENT) {
      const auto skip = validate_percent_encoded_utf8(input, position);
      position += skip;
    } else if (uri_is_pchar(current) || current == URI_SLASH) {
      position += 1;
    } else if (!accept_iri_extension<IRI>(input, position)) [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
  }

  if constexpr (CheckOnly) {
    return true;
  } else {
    return std::string{input.substr(start, position - start)};
  }
}

template <bool CheckOnly, bool IRI>
auto parse_query(const std::string_view input,
                 std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, bool, std::optional<std::string>> {
  if (position >= input.size() || input[position] != URI_QUESTION) {
    if constexpr (CheckOnly) {
      return false;
    } else {
      return std::nullopt;
    }
  }

  position += 1;
  const auto start = position;

  while (position < input.size()) {
    const auto current = input[position];
    if (current == URI_HASH) {
      break;
    }

    if (current == URI_PERCENT) {
      const auto skip = validate_percent_encoded_utf8(input, position);
      position += skip;
    } else if (uri_is_pchar(current) || current == URI_SLASH ||
               current == URI_QUESTION) {
      position += 1;
    } else if (!accept_iri_extension<IRI, true>(input, position)) [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
  }

  if constexpr (CheckOnly) {
    return true;
  } else {
    return std::string{input.substr(start, position - start)};
  }
}

template <bool CheckOnly, bool IRI>
auto parse_fragment(const std::string_view input,
                    std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, bool, std::optional<std::string>> {
  if (position >= input.size() || input[position] != URI_HASH) {
    if constexpr (CheckOnly) {
      return false;
    } else {
      return std::nullopt;
    }
  }

  position += 1;
  const auto start = position;

  while (position < input.size()) {
    const auto current = input[position];

    if (current == URI_PERCENT) {
      const auto skip = validate_percent_encoded_utf8(input, position);
      position += skip;
    } else if (uri_is_pchar(current) || current == URI_SLASH ||
               current == URI_QUESTION) {
      position += 1;
    } else if (!accept_iri_extension<IRI>(input, position)) [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
  }

  if constexpr (CheckOnly) {
    return true;
  } else {
    return std::string{input.substr(start, position - start)};
  }
}

template <bool CheckOnly, bool IRI>
auto parse_authority(const std::string_view input,
                     std::string_view::size_type &position,
                     [[maybe_unused]] std::optional<std::string> &userinfo,
                     [[maybe_unused]] std::optional<std::string> &host,
                     [[maybe_unused]] std::optional<std::uint32_t> &port,
                     [[maybe_unused]] bool &ip_literal) -> void {
  if constexpr (CheckOnly) {
    parse_userinfo<true, IRI>(input, position);
    parse_host<true, IRI>(input, position, ip_literal);
  } else {
    auto userinfo_raw = parse_userinfo<false, IRI>(input, position);
    if (userinfo_raw.has_value()) {
      uri_unescape_unreserved_inplace(userinfo_raw.value());
      userinfo = std::move(userinfo_raw.value());
    }

    auto host_raw = parse_host<false, IRI>(input, position, ip_literal);
    uri_unescape_unreserved_inplace(host_raw);
    host = std::move(host_raw);
  }

  // RFC 3986: authority = [ userinfo "@" ] host [ ":" port ]
  // port = *DIGIT (empty port after colon is valid)
  if (position < input.size() && input[position] == URI_COLON) {
    position += 1;
    if constexpr (CheckOnly) {
      parse_port<true>(input, position);
    } else {
      const auto port_start = position;
      const auto port_value = parse_port<false>(input, position);
      if (port_value.has_value()) {
        if (port_value.value() > std::numeric_limits<std::uint32_t>::max())
            [[unlikely]] {
          throw sourcemeta::core::URIParseError{
              static_cast<std::uint64_t>(port_start + 1)};
        }

        port = static_cast<std::uint32_t>(port_value.value());
      }
    }
  }

  if (position < input.size() && input[position] == URI_AT) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }
}

template <bool CheckOnly, bool IRI>
auto do_parse(const std::string_view input,
              [[maybe_unused]] std::optional<std::string> &scheme,
              [[maybe_unused]] std::optional<std::string> &userinfo,
              [[maybe_unused]] std::optional<std::string> &host,
              [[maybe_unused]] std::optional<std::uint32_t> &port,
              [[maybe_unused]] std::optional<std::string> &path,
              [[maybe_unused]] std::optional<std::string> &query,
              [[maybe_unused]] std::optional<std::string> &fragment,
              [[maybe_unused]] bool &ip_literal) -> bool {
  if (input.empty()) {
    return false;
  }

  std::string_view::size_type position{0};

  bool has_scheme;
  if constexpr (CheckOnly) {
    has_scheme = parse_scheme<true>(input, position);
  } else {
    scheme = parse_scheme<false>(input, position);
    has_scheme = scheme.has_value();
  }

  const auto has_authority = position + 1 < input.size() &&
                             input[position] == URI_SLASH &&
                             input[position + 1] == URI_SLASH;

  if (has_authority) {
    position += 2;
    parse_authority<CheckOnly, IRI>(input, position, userinfo, host, port,
                                    ip_literal);

    // RFC 3986: hier-part = "//" authority path-abempty
    // path-abempty = *( "/" segment ), so after authority the next character
    // must be "/", "?", "#", or end-of-input
    if (position < input.size() && input[position] != URI_SLASH &&
        input[position] != URI_QUESTION && input[position] != URI_HASH)
        [[unlikely]] {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
  }

  const auto path_start = position;
  bool has_path;
  if constexpr (CheckOnly) {
    has_path = parse_path<true, IRI>(input, position);
  } else {
    auto parsed_path = parse_path<false, IRI>(input, position);
    has_path = parsed_path.has_value();

    if (has_path) {
      // RFC 3986: relative-ref without authority uses path-noscheme,
      // where the first segment must not contain a colon
      if (!has_scheme && !has_authority) {
        const auto &path_value = parsed_path.value();
        if (!path_value.empty() && path_value[0] != URI_SLASH) {
          const auto first_slash = path_value.find(URI_SLASH);
          const auto colon_pos = path_value.find(URI_COLON);
          if (colon_pos != std::string::npos &&
              (first_slash == std::string::npos || colon_pos < first_slash))
              [[unlikely]] {
            throw sourcemeta::core::URIParseError{
                static_cast<std::uint64_t>(colon_pos + 1)};
          }
        }
      }

      uri_unescape_unreserved_inplace(parsed_path.value());
      path = std::move(parsed_path.value());
    }
  }

  if constexpr (CheckOnly) {
    if (has_path && !has_scheme && !has_authority) {
      if (input[path_start] != URI_SLASH) {
        const auto path_view = input.substr(path_start, position - path_start);
        const auto first_slash = path_view.find(URI_SLASH);
        const auto colon_pos = path_view.find(URI_COLON);
        if (colon_pos != std::string_view::npos &&
            (first_slash == std::string_view::npos || colon_pos < first_slash))
            [[unlikely]] {
          throw sourcemeta::core::URIParseError{
              static_cast<std::uint64_t>(path_start + colon_pos + 1)};
        }
      }
    }
  }

  if constexpr (CheckOnly) {
    parse_query<true, IRI>(input, position);
    parse_fragment<true, IRI>(input, position);
  } else {
    auto parsed_query = parse_query<false, IRI>(input, position);
    if (parsed_query.has_value()) {
      uri_unescape_unreserved_inplace(parsed_query.value());
      query = std::move(parsed_query.value());
    }

    auto parsed_fragment = parse_fragment<false, IRI>(input, position);
    if (parsed_fragment.has_value()) {
      uri_unescape_unreserved_inplace(parsed_fragment.value());
      fragment = std::move(parsed_fragment.value());
    }
  }

  if (position < input.size()) [[unlikely]] {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }

  return has_scheme;
}

} // namespace

namespace sourcemeta::core {

auto URI::parse(const std::string_view input) -> void {
  assert(!this->scheme_.has_value());
  assert(!this->userinfo_.has_value());
  assert(!this->host_.has_value());
  assert(!this->port_.has_value());
  assert(!this->path_.has_value());
  assert(!this->query_.has_value());
  assert(!this->fragment_.has_value());
  if (this->iri_) {
    do_parse<false, true>(input, this->scheme_, this->userinfo_, this->host_,
                          this->port_, this->path_, this->query_,
                          this->fragment_, this->ip_literal_);
  } else {
    do_parse<false, false>(input, this->scheme_, this->userinfo_, this->host_,
                           this->port_, this->path_, this->query_,
                           this->fragment_, this->ip_literal_);
  }
}

auto URI::is_scheme(const std::string_view input) noexcept -> bool {
  if (input.empty() || !is_alpha(input.front())) {
    return false;
  }
  for (const auto character : input) {
    if (!uri_is_scheme_char(character)) {
      return false;
    }
  }
  return true;
}

auto URI::is_gen_delim(const char character) noexcept -> bool {
  return uri_is_gen_delim(character);
}

auto URI::is_uri(const std::string_view input) noexcept -> bool {
  try {
    std::optional<std::string> scheme, userinfo, host, path, query, fragment;
    std::optional<std::uint32_t> port;
    bool ip_literal{false};
    return do_parse<true, false>(input, scheme, userinfo, host, port, path,
                                 query, fragment, ip_literal);
  } catch (...) {
    return false;
  }
}

auto URI::is_uri_reference(const std::string_view input) noexcept -> bool {
  try {
    std::optional<std::string> scheme, userinfo, host, path, query, fragment;
    std::optional<std::uint32_t> port;
    bool ip_literal{false};
    do_parse<true, false>(input, scheme, userinfo, host, port, path, query,
                          fragment, ip_literal);
    return true;
  } catch (...) {
    return false;
  }
}

auto URI::is_iri(const std::string_view input) noexcept -> bool {
  try {
    std::optional<std::string> scheme, userinfo, host, path, query, fragment;
    std::optional<std::uint32_t> port;
    bool ip_literal{false};
    return do_parse<true, true>(input, scheme, userinfo, host, port, path,
                                query, fragment, ip_literal);
  } catch (...) {
    return false;
  }
}

auto URI::is_iri_reference(const std::string_view input) noexcept -> bool {
  try {
    std::optional<std::string> scheme, userinfo, host, path, query, fragment;
    std::optional<std::uint32_t> port;
    bool ip_literal{false};
    do_parse<true, true>(input, scheme, userinfo, host, port, path, query,
                         fragment, ip_literal);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace sourcemeta::core
