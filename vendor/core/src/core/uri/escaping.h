#ifndef SOURCEMETA_CORE_URI_ESCAPING_H_
#define SOURCEMETA_CORE_URI_ESCAPING_H_

#include "grammar.h"

#include <cctype>  // std::isxdigit, std::toupper
#include <cstdint> // std::uint8_t
#include <string>  // std::string

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
  Filesystem,
  // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
  // See https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  UserInfo
};

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
