#ifndef SOURCEMETA_CORE_URI_ESCAPING_H_
#define SOURCEMETA_CORE_URI_ESCAPING_H_

#include "grammar.h"

#include <array>    // std::array
#include <cctype>   // std::isalnum
#include <charconv> // std::from_chars
#include <cstdint>  // std::uint8_t
#include <istream>  // std::istream
#include <iterator> // std::istream_iterator
#include <ostream>  // std::ostream
#include <string>   // std::string

namespace sourcemeta::core {

enum class URIEscapeMode : std::uint8_t {
  // Escape every characted that is not in the URI "unreserved" ABNF category
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  SkipUnreserved,
  // Escape every characted that is not in either the URI "unreserved" nor
  // "sub-delims" ABNF categories
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  SkipSubDelims,
  // pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
  // path  = *( pchar / "/" )
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  Path,
  // pchar    = unreserved / pct-encoded / sub-delims / ":" / "@"
  // fragment = *( pchar / "/" / "?" )
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  Fragment,
  // Like SkipSubDelims but also preserves ":" for Windows filesystem paths
  // (drive letters like C:)
  Filesystem
};

inline auto uri_escape(std::istream &input, std::ostream &output,
                       const URIEscapeMode mode,
                       const bool preserve_percent_sequences = true) -> void {
  char character = 0;
  while (input.get(character)) {
    // Check if this is an already percent-encoded sequence (%HEXHEX)
    // If so, preserve it as-is to avoid double-encoding
    // (only when preserve_percent_sequences is true)
    if (preserve_percent_sequences && character == URI_PERCENT) {
      const auto position = input.tellg();
      char next_1 = 0;
      char next_2 = 0;

      if (input.get(next_1) && input.get(next_2) &&
          std::isxdigit(static_cast<unsigned char>(next_1)) &&
          std::isxdigit(static_cast<unsigned char>(next_2))) {
        // Valid percent-encoded sequence - preserve it
        output << character << next_1 << next_2;
        continue;
      }

      // Not a valid percent-encoded sequence - restore position and escape %
      input.seekg(position);
    }

    // unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
    // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
    if (uri_is_unreserved(character)) {
      output << character;
      continue;
    }

    if (mode == URIEscapeMode::SkipSubDelims || mode == URIEscapeMode::Path ||
        mode == URIEscapeMode::Fragment || mode == URIEscapeMode::Filesystem) {
      if (uri_is_sub_delim(character)) {
        output << character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Path) {
      if (character == URI_COLON || character == URI_AT ||
          character == URI_SLASH) {
        output << character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Fragment) {
      if (character == URI_COLON || character == URI_AT ||
          character == URI_SLASH || character == URI_QUESTION) {
        output << character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Filesystem) {
      if (character == URI_COLON) {
        output << character;
        continue;
      }
    }

    const auto byte{static_cast<unsigned char>(character)};
    const auto high{(byte >> 4) & 0x0F};
    const auto low{byte & 0x0F};
    output << URI_PERCENT;
    output << static_cast<char>(high < 10 ? '0' + high : 'A' + high - 10);
    output << static_cast<char>(low < 10 ? '0' + low : 'A' + low - 10);
  }
}

inline auto uri_unescape(std::istream &input, std::ostream &output) -> void {
  std::istream_iterator<char> iterator(input);
  std::istream_iterator<char> end;
  auto plus_1 = std::ranges::next(iterator, 1, end);
  auto plus_2 = std::ranges::next(plus_1, 1, end);
  const int hex_base = 16;

  while (iterator != end) {
    if (*iterator == URI_PERCENT && plus_1 != end && plus_2 != end &&
        std::isxdigit(*(plus_1)) && std::isxdigit(*(plus_2))) {
      const std::array<char, 2> hex{{*plus_1, *plus_2}};
      int decoded_value{};
      std::from_chars(hex.data(), hex.data() + hex.size(), decoded_value,
                      hex_base);
      output << static_cast<char>(decoded_value);

      iterator = std::ranges::next(plus_2, 1, end);
      plus_1 = std::ranges::next(iterator, 1, end);
      plus_2 = std::ranges::next(plus_1, 1, end);
    } else {
      output << *iterator;
      iterator = plus_1;
      plus_1 = plus_2;
      plus_2 = std::ranges::next(plus_1, 1, end);
    }
  }
}

inline auto uri_hex_to_int(char character) -> unsigned char {
  if (character >= '0' && character <= '9') {
    return static_cast<unsigned char>(character - '0');
  }

  if (character >= 'A' && character <= 'F') {
    return static_cast<unsigned char>(character - 'A' + 10);
  }

  if (character >= 'a' && character <= 'f') {
    return static_cast<unsigned char>(character - 'a' + 10);
  }

  return 0;
}

inline auto uri_is_percent_encoded(const std::string &input,
                                   std::string::size_type position) -> bool {
  return position < input.size() && input[position] == URI_PERCENT &&
         position + 2 < input.size() &&
         std::isxdigit(static_cast<unsigned char>(input[position + 1])) &&
         std::isxdigit(static_cast<unsigned char>(input[position + 2]));
}

inline auto uri_unescape_all_inplace(std::string &input) -> void {
  std::string::size_type write_position = 0;

  for (std::string::size_type read_position = 0;
       read_position < input.size();) {
    if (uri_is_percent_encoded(input, read_position)) {
      const auto value = static_cast<unsigned char>(
          (uri_hex_to_int(input[read_position + 1]) << 4) |
          uri_hex_to_int(input[read_position + 2]));
      input[write_position++] = static_cast<char>(value);
      read_position += 3;
    } else {
      input[write_position++] = input[read_position++];
    }
  }

  input.resize(write_position);
}

inline auto uri_unescape_unreserved_inplace(std::string &input) -> void {
  std::string::size_type write_position = 0;

  for (std::string::size_type read_position = 0;
       read_position < input.size();) {
    if (uri_is_percent_encoded(input, read_position)) {
      const auto value = static_cast<unsigned char>(
          (uri_hex_to_int(input[read_position + 1]) << 4) |
          uri_hex_to_int(input[read_position + 2]));
      if (uri_is_unreserved(static_cast<char>(value))) {
        input[write_position++] = static_cast<char>(value);
      } else {
        input[write_position++] = input[read_position];
        input[write_position++] = input[read_position + 1];
        input[write_position++] = input[read_position + 2];
      }

      read_position += 3;
    } else {
      input[write_position++] = input[read_position++];
    }
  }

  input.resize(write_position);
}

inline auto uri_normalize_percent_encoding_inplace(std::string &input) -> void {
  for (std::string::size_type position = 0; position < input.size();) {
    if (uri_is_percent_encoded(input, position)) {
      input[position + 1] = static_cast<char>(
          std::toupper(static_cast<unsigned char>(input[position + 1])));
      input[position + 2] = static_cast<char>(
          std::toupper(static_cast<unsigned char>(input[position + 2])));
      position += 3;
    } else {
      ++position;
    }
  }
}

template <typename Predicate>
inline auto uri_unescape_if_inplace(std::string &input, Predicate should_decode)
    -> void {
  std::string::size_type write_position = 0;

  for (std::string::size_type read_position = 0;
       read_position < input.size();) {
    if (uri_is_percent_encoded(input, read_position)) {
      const auto value = static_cast<unsigned char>(
          (uri_hex_to_int(input[read_position + 1]) << 4) |
          uri_hex_to_int(input[read_position + 2]));
      const auto decoded = static_cast<char>(value);

      if (should_decode(decoded)) {
        input[write_position++] = decoded;
      } else {
        input[write_position++] = input[read_position];
        input[write_position++] = input[read_position + 1];
        input[write_position++] = input[read_position + 2];
      }

      read_position += 3;
    } else {
      input[write_position++] = input[read_position++];
    }
  }

  input.resize(write_position);
}

} // namespace sourcemeta::core

#endif
