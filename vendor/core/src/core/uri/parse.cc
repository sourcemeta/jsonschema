#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "grammar.h"

#include <cassert>  // assert
#include <cctype>   // std::isalnum, std::isxdigit, std::isalpha, std::isdigit
#include <cstdint>  // std::uint64_t
#include <optional> // std::optional
#include <string>   // std::string, std::stoul

namespace {

using namespace sourcemeta::core;

auto validate_percent_encoded_utf8(const std::string &input,
                                   std::string::size_type position)
    -> std::string::size_type {
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

auto parse_scheme(const std::string &input, std::string::size_type &position)
    -> std::optional<std::string> {
  if (position >= input.size() ||
      !std::isalpha(static_cast<unsigned char>(input[position]))) {
    return std::nullopt;
  }

  const auto start = position;
  position += 1;

  while (position < input.size() && uri_is_scheme_char(input[position])) {
    position += 1;
  }

  if (position < input.size() && input[position] == URI_COLON) {
    auto scheme = input.substr(start, position - start);
    position += 1;
    return scheme;
  }

  position = start;
  return std::nullopt;
}

auto parse_port(const std::string &input, std::string::size_type &position)
    -> std::optional<unsigned long> {
  if (position >= input.size() ||
      !std::isdigit(static_cast<unsigned char>(input[position]))) {
    return std::nullopt;
  }

  const auto start = position;
  while (position < input.size() &&
         std::isdigit(static_cast<unsigned char>(input[position]))) {
    position += 1;
  }

  const auto port_string = input.substr(start, position - start);
  return std::stoul(port_string);
}

auto parse_ipv6(const std::string &input, std::string::size_type &position)
    -> std::string {
  assert(input[position] == URI_OPEN_BRACKET);

  const auto start = position;
  position += 1;

  while (position < input.size() && input[position] != URI_CLOSE_BRACKET) {
    position += 1;
  }

  if (position >= input.size()) {
    throw sourcemeta::core::URIParseError{
        static_cast<std::uint64_t>(start + 1)};
  }

  auto ipv6 = input.substr(start + 1, position - start - 1);
  position += 1;
  return ipv6;
}

auto parse_host(const std::string &input, std::string::size_type &position)
    -> std::string {
  if (position >= input.size()) {
    return std::string{};
  }

  if (input[position] == URI_OPEN_BRACKET) {
    return parse_ipv6(input, position);
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

  if (position == start) {
    return std::string{};
  }

  return input.substr(start, position - start);
}

auto parse_userinfo(const std::string &input, std::string::size_type &position)
    -> std::optional<std::string> {
  const auto start = position;
  while (position < input.size()) {
    const auto current = input[position];
    if (current == URI_AT) {
      auto userinfo = input.substr(start, position - start);
      position += 1;
      return userinfo;
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
  return std::nullopt;
}

auto parse_path(const std::string &input, std::string::size_type &position)
    -> std::optional<std::string> {
  if (position >= input.size()) {
    return std::nullopt;
  }

  const auto first_char = input[position];
  if (first_char == URI_QUESTION || first_char == URI_HASH) {
    return std::nullopt;
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

  return input.substr(start, position - start);
}

auto parse_query(const std::string &input, std::string::size_type &position)
    -> std::optional<std::string> {
  if (position >= input.size() || input[position] != URI_QUESTION) {
    return std::nullopt;
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

  return input.substr(start, position - start);
}

auto parse_fragment(const std::string &input, std::string::size_type &position)
    -> std::optional<std::string> {
  if (position >= input.size() || input[position] != URI_HASH) {
    return std::nullopt;
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

  return input.substr(start, position - start);
}

} // namespace

namespace sourcemeta::core {

auto parse_authority(const std::string &input, std::string::size_type &position,
                     std::optional<std::string> &userinfo,
                     std::optional<std::string> &host,
                     std::optional<std::uint32_t> &port) -> void {
  auto userinfo_raw = parse_userinfo(input, position);
  if (userinfo_raw.has_value()) {
    uri_unescape_selective_inplace(userinfo_raw.value());
    userinfo = std::move(userinfo_raw.value());
  }

  auto host_raw = parse_host(input, position);
  uri_unescape_selective_inplace(host_raw);
  host = std::move(host_raw);

  if (position < input.size() && input[position] == URI_COLON) {
    const auto colon_position = position;
    position += 1;
    const auto port_value = parse_port(input, position);
    if (port_value.has_value()) {
      port = port_value.value();
    } else {
      position = colon_position;
    }
  }

  if (position < input.size() && input[position] == URI_AT) {
    throw URIParseError{static_cast<std::uint64_t>(position + 1)};
  }
}

auto URI::parse(const std::string &input) -> void {
  assert(!this->scheme_.has_value());
  assert(!this->userinfo_.has_value());
  assert(!this->host_.has_value());
  assert(!this->port_.has_value());
  assert(!this->path_.has_value());
  assert(!this->query_.has_value());
  assert(!this->fragment_.has_value());

  if (input.empty()) {
    return;
  }

  auto position = std::string::size_type{0};

  this->scheme_ = parse_scheme(input, position);

  const auto has_authority = position + 1 < input.size() &&
                             input[position] == URI_SLASH &&
                             input[position + 1] == URI_SLASH;

  if (has_authority) {
    position += 2;
    parse_authority(input, position, this->userinfo_, this->host_, this->port_);
  }

  auto path = parse_path(input, position);

  if (path.has_value()) {
    uri_unescape_selective_inplace(path.value());
    this->path_ = std::move(path.value());
  } else if (has_authority || this->scheme_.has_value()) {
    if (input.ends_with(URI_SLASH) || input == "/") {
      this->path_ = "/";
    }
  }

  auto query = parse_query(input, position);
  if (query.has_value()) {
    uri_unescape_selective_inplace(query.value());
    this->query_ = std::move(query.value());
  }

  auto fragment = parse_fragment(input, position);
  if (fragment.has_value()) {
    uri_unescape_selective_inplace(fragment.value());
    this->fragment_ = std::move(fragment.value());
  }

  if (position < input.size()) {
    throw URIParseError{static_cast<std::uint64_t>(position + 1)};
  }
}

} // namespace sourcemeta::core
