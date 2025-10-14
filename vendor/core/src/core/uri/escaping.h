#ifndef SOURCEMETA_CORE_URI_ESCAPING_H_
#define SOURCEMETA_CORE_URI_ESCAPING_H_

#include "grammar.h"

#include <cctype>   // std::isalnum
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
  // Escape every characted that is not in either the URI "fragment" category
  //
  // unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
  // pct-encoded = "%" HEXDIG HEXDIG
  // sub-delims  = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" /
  // "="
  // pchar       = unreserved / pct-encoded / sub-delims / ":" / "@"
  // fragment    = *( pchar / "/" / "?" )
  //
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

    if (mode == URIEscapeMode::SkipSubDelims ||
        mode == URIEscapeMode::Fragment || mode == URIEscapeMode::Filesystem) {
      // sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";"
      // / "="
      // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
      if (uri_is_sub_delim(character)) {
        output << character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Fragment) {
      // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
      // pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
      // fragment = *( pchar / "/" / "?" )
      if (character == URI_COLON || character == URI_AT ||
          character == URI_SLASH || character == URI_QUESTION) {
        output << character;
        continue;
      }
    }

    if (mode == URIEscapeMode::Filesystem) {
      // Preserve ":" for Windows drive letters (e.g., C:)
      if (character == URI_COLON) {
        output << character;
        continue;
      }
    }

    // Percent encode this character
    output << URI_PERCENT << std::hex << std::uppercase
           << +(static_cast<unsigned char>(character));
  }

  // Reset stream format flags
  output << std::dec << std::nouppercase;
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
      std::string hex{*plus_1, *plus_2};
      char decoded_char = static_cast<char>(std::stoi(hex, nullptr, hex_base));
      output << decoded_char;

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

// Full unescaping for URI normalization (in-place modification)
// Decodes all percent-encoded sequences
// Modifies the input string in-place for zero-copy performance
inline auto uri_unescape_selective_inplace(std::string &str) -> void {
  std::string::size_type write_pos = 0;

  for (std::string::size_type read_pos = 0; read_pos < str.size();) {
    if (str[read_pos] == URI_PERCENT && read_pos + 2 < str.size() &&
        std::isxdigit(static_cast<unsigned char>(str[read_pos + 1])) &&
        std::isxdigit(static_cast<unsigned char>(str[read_pos + 2]))) {
      // Parse the hex value
      const auto first_digit = str[read_pos + 1];
      const auto second_digit = str[read_pos + 2];

      const auto hex_to_int = [](char c) -> unsigned char {
        if (c >= '0' && c <= '9') {
          return static_cast<unsigned char>(c - '0');
        }
        if (c >= 'A' && c <= 'F') {
          return static_cast<unsigned char>(c - 'A' + 10);
        }
        if (c >= 'a' && c <= 'f') {
          return static_cast<unsigned char>(c - 'a' + 10);
        }
        return 0;
      };

      const auto value = static_cast<unsigned char>(
          (hex_to_int(first_digit) << 4) | hex_to_int(second_digit));

      // Decode all percent-encoded sequences
      // Internal storage is always fully decoded
      str[write_pos++] = static_cast<char>(value);
      read_pos += 3;
    } else {
      str[write_pos++] = str[read_pos++];
    }
  }

  str.resize(write_pos);
}

// Full unescaping for URI normalization (copy version for compatibility)
// Decodes all percent-encoded sequences
inline auto uri_unescape_selective(std::string_view input) -> std::string {
  std::string result{input};
  uri_unescape_selective_inplace(result);
  return result;
}

} // namespace sourcemeta::core

#endif
