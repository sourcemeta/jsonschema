#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "grammar.h"

#include <cassert>   // assert
#include <cctype>    // std::isalnum, std::isxdigit, std::isalpha, std::isdigit
#include <cstdint>   // std::uint64_t
#include <limits>    // std::numeric_limits
#include <optional>  // std::optional
#include <stdexcept> // std::out_of_range
#include <string>    // std::string, std::stoul
#include <string_view> // std::string_view
#include <type_traits> // std::conditional_t

namespace {

using namespace sourcemeta::core;

auto validate_percent_encoded_utf8(const std::string_view input,
                                   std::string_view::size_type position)
    -> std::string_view::size_type {
  if (input[position] != URI_PERCENT) {
    return 3;
  }

  if (position + 2 >= input.size()) {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }

  const auto first_hex = static_cast<unsigned char>(input[position + 1]);
  const auto second_hex = static_cast<unsigned char>(input[position + 2]);

  if (!std::isxdigit(first_hex) || !std::isxdigit(second_hex)) {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }

  std::string hex{input[position + 1], input[position + 2]};
  const auto value = static_cast<unsigned char>(std::stoi(hex, nullptr, 16));

  if ((value & 0x80) == 0) {
    return 3;
  }

  if ((value & 0xC0) == 0x80) {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }

  std::string::size_type continuation_count = 0;
  if ((value & 0xE0) == 0xC0) {
    continuation_count = 1;
  } else if ((value & 0xF0) == 0xE0) {
    continuation_count = 2;
  } else if ((value & 0xF8) == 0xF0) {
    continuation_count = 3;
  }

  for (std::string::size_type index = 1; index <= continuation_count; ++index) {
    const std::string::size_type next_position = position + (index * 3);
    if (next_position + 2 >= input.size() ||
        input[next_position] != URI_PERCENT) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }

    const auto cont_first =
        static_cast<unsigned char>(input[next_position + 1]);
    const auto cont_second =
        static_cast<unsigned char>(input[next_position + 2]);

    if (!std::isxdigit(cont_first) || !std::isxdigit(cont_second)) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(next_position + 1)};
    }

    std::string cont_hex{input[next_position + 1], input[next_position + 2]};
    const auto cont_value =
        static_cast<unsigned char>(std::stoi(cont_hex, nullptr, 16));

    if ((cont_value & 0xC0) != 0x80) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(next_position + 1)};
    }
  }

  return 3 * (1 + continuation_count);
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
    try {
      const std::string port_string{input.substr(start, position - start)};
      return std::stoul(port_string);
    } catch (const std::out_of_range &) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(start + 1)};
    }
  }
}

template <bool CheckOnly>
auto parse_ipv6(const std::string_view input,
                std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, void, std::string> {
  assert(input[position] == URI_OPEN_BRACKET);

  const auto start = position;
  position += 1;

  // RFC 3986: IP-literal = "[" ( IPv6address / IPvFuture ) "]"
  if (position < input.size() &&
      (input[position] == 'v' || input[position] == 'V')) {
    // IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    position += 1;

    // Require 1*HEXDIG for the version
    if (position >= input.size() || input[position] == URI_CLOSE_BRACKET ||
        !std::isxdigit(static_cast<unsigned char>(input[position]))) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
    while (position < input.size() && input[position] != URI_CLOSE_BRACKET &&
           std::isxdigit(static_cast<unsigned char>(input[position]))) {
      position += 1;
    }

    // Require "." separator
    if (position >= input.size() || input[position] != URI_DOT) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
    position += 1;

    // Require 1*( unreserved / sub-delims / ":" )
    if (position >= input.size() || input[position] == URI_CLOSE_BRACKET) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
    while (position < input.size() && input[position] != URI_CLOSE_BRACKET) {
      const auto current = input[position];
      if (!uri_is_unreserved(current) && !uri_is_sub_delim(current) &&
          current != URI_COLON) {
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
          current != URI_COLON && current != URI_DOT) {
        throw sourcemeta::core::URIParseError{
            static_cast<std::uint64_t>(position + 1)};
      }
      position += 1;
    }
  }

  if (position >= input.size()) {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(start + 1)};
  }

  if constexpr (CheckOnly) {
    position += 1;
  } else {
    std::string ipv6{input.substr(start + 1, position - start - 1)};
    position += 1;
    return ipv6;
  }
}

template <bool CheckOnly>
auto parse_host(const std::string_view input,
                std::string_view::size_type &position)
    -> std::conditional_t<CheckOnly, void, std::string> {
  if (position >= input.size()) {
    if constexpr (!CheckOnly) {
      return std::string{};
    } else {
      return;
    }
  }

  if (input[position] == URI_OPEN_BRACKET) {
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
    } else {
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

template <bool CheckOnly>
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

template <bool CheckOnly>
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
    } else {
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

template <bool CheckOnly>
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
    } else {
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

template <bool CheckOnly>
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
    } else {
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

template <bool CheckOnly>
auto parse_authority(const std::string_view input,
                     std::string_view::size_type &position,
                     [[maybe_unused]] std::optional<std::string> &userinfo,
                     [[maybe_unused]] std::optional<std::string> &host,
                     [[maybe_unused]] std::optional<std::uint32_t> &port)
    -> void {
  if constexpr (CheckOnly) {
    parse_userinfo<true>(input, position);
    parse_host<true>(input, position);
  } else {
    auto userinfo_raw = parse_userinfo<false>(input, position);
    if (userinfo_raw.has_value()) {
      uri_unescape_unreserved_inplace(userinfo_raw.value());
      userinfo = std::move(userinfo_raw.value());
    }

    auto host_raw = parse_host<false>(input, position);
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
        if (port_value.value() > std::numeric_limits<std::uint32_t>::max()) {
          throw sourcemeta::core::URIParseError{
              static_cast<std::uint64_t>(port_start + 1)};
        }

        port = static_cast<std::uint32_t>(port_value.value());
      }
    }
  }

  if (position < input.size() && input[position] == URI_AT) {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(position + 1)};
  }
}

template <bool CheckOnly>
auto do_parse(const std::string_view input,
              [[maybe_unused]] std::optional<std::string> &scheme,
              [[maybe_unused]] std::optional<std::string> &userinfo,
              [[maybe_unused]] std::optional<std::string> &host,
              [[maybe_unused]] std::optional<std::uint32_t> &port,
              [[maybe_unused]] std::optional<std::string> &path,
              [[maybe_unused]] std::optional<std::string> &query,
              [[maybe_unused]] std::optional<std::string> &fragment) -> bool {
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
    parse_authority<CheckOnly>(input, position, userinfo, host, port);

    // RFC 3986: hier-part = "//" authority path-abempty
    // path-abempty = *( "/" segment ), so after authority the next character
    // must be "/", "?", "#", or end-of-input
    if (position < input.size() && input[position] != URI_SLASH &&
        input[position] != URI_QUESTION && input[position] != URI_HASH) {
      throw sourcemeta::core::URIParseError{
          static_cast<std::uint64_t>(position + 1)};
    }
  }

  const auto path_start = position;
  bool has_path;
  if constexpr (CheckOnly) {
    has_path = parse_path<true>(input, position);
  } else {
    auto parsed_path = parse_path<false>(input, position);
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
              (first_slash == std::string::npos || colon_pos < first_slash)) {
            throw sourcemeta::core::URIParseError{
                static_cast<std::uint64_t>(colon_pos + 1)};
          }
        }
      }

      uri_unescape_unreserved_inplace(parsed_path.value());
      path = std::move(parsed_path.value());
    } else if (has_authority || has_scheme) {
      if (input.ends_with(URI_SLASH) || input == "/") {
        path = "/";
      }
    }
  }

  if constexpr (CheckOnly) {
    if (has_path && !has_scheme && !has_authority) {
      if (input[path_start] != URI_SLASH) {
        const auto path_view = input.substr(path_start, position - path_start);
        const auto first_slash = path_view.find(URI_SLASH);
        const auto colon_pos = path_view.find(URI_COLON);
        if (colon_pos != std::string_view::npos &&
            (first_slash == std::string_view::npos ||
             colon_pos < first_slash)) {
          throw sourcemeta::core::URIParseError{
              static_cast<std::uint64_t>(path_start + colon_pos + 1)};
        }
      }
    }
  }

  if constexpr (CheckOnly) {
    parse_query<true>(input, position);
    parse_fragment<true>(input, position);
  } else {
    auto parsed_query = parse_query<false>(input, position);
    if (parsed_query.has_value()) {
      uri_unescape_unreserved_inplace(parsed_query.value());
      query = std::move(parsed_query.value());
    }

    auto parsed_fragment = parse_fragment<false>(input, position);
    if (parsed_fragment.has_value()) {
      uri_unescape_unreserved_inplace(parsed_fragment.value());
      fragment = std::move(parsed_fragment.value());
    }
  }

  if (position < input.size()) {
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
  do_parse<false>(input, this->scheme_, this->userinfo_, this->host_,
                  this->port_, this->path_, this->query_, this->fragment_);
}

auto URI::is_uri(const std::string_view input) noexcept -> bool {
  try {
    std::optional<std::string> scheme, userinfo, host, path, query, fragment;
    std::optional<std::uint32_t> port;
    return do_parse<true>(input, scheme, userinfo, host, port, path, query,
                          fragment);
  } catch (...) {
    return false;
  }
}

auto URI::is_uri_reference(const std::string_view input) noexcept -> bool {
  try {
    std::optional<std::string> scheme, userinfo, host, path, query, fragment;
    std::optional<std::uint32_t> port;
    do_parse<true>(input, scheme, userinfo, host, port, path, query, fragment);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace sourcemeta::core
