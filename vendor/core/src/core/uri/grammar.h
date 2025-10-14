#ifndef SOURCEMETA_CORE_URI_GRAMMAR_H_
#define SOURCEMETA_CORE_URI_GRAMMAR_H_

#include <cctype> // std::isalnum, std::isalpha, std::isdigit

namespace sourcemeta::core {

// URI Grammar Constants
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.2
// See https://www.rfc-editor.org/rfc/rfc3986#appendix-A

// gen-delims = ":" / "/" / "?" / "#" / "[" / "]" / "@"
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.2
constexpr char URI_COLON = ':';
constexpr char URI_SLASH = '/';
constexpr char URI_QUESTION = '?';
constexpr char URI_HASH = '#';
constexpr char URI_AT = '@';
constexpr char URI_OPEN_BRACKET = '[';
constexpr char URI_CLOSE_BRACKET = ']';

// sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.2
constexpr char URI_EXCLAMATION = '!';
constexpr char URI_DOLLAR = '$';
constexpr char URI_AMPERSAND = '&';
constexpr char URI_APOSTROPHE = '\'';
constexpr char URI_OPEN_PAREN = '(';
constexpr char URI_CLOSE_PAREN = ')';
constexpr char URI_ASTERISK = '*';
constexpr char URI_PLUS = '+';
constexpr char URI_COMMA = ',';
constexpr char URI_SEMICOLON = ';';
constexpr char URI_EQUALS = '=';

// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.3
constexpr char URI_HYPHEN = '-';
constexpr char URI_DOT = '.';
constexpr char URI_UNDERSCORE = '_';
constexpr char URI_TILDE = '~';

// pct-encoded = "%" HEXDIG HEXDIG
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.1
constexpr char URI_PERCENT = '%';

// Character Classification Helper Functions
// See https://www.rfc-editor.org/rfc/rfc3986#section-2

// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.3
inline auto uri_is_unreserved(const char character) -> bool {
  if (std::isalnum(static_cast<unsigned char>(character))) {
    return true;
  }

  switch (character) {
    case URI_HYPHEN:
    case URI_DOT:
    case URI_UNDERSCORE:
    case URI_TILDE:
      return true;
    default:
      return false;
  }
}

// sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
// See https://www.rfc-editor.org/rfc/rfc3986#section-2.2
inline auto uri_is_sub_delim(const char character) -> bool {
  switch (character) {
    case URI_EXCLAMATION:
    case URI_DOLLAR:
    case URI_AMPERSAND:
    case URI_APOSTROPHE:
    case URI_OPEN_PAREN:
    case URI_CLOSE_PAREN:
    case URI_ASTERISK:
    case URI_PLUS:
    case URI_COMMA:
    case URI_SEMICOLON:
    case URI_EQUALS:
      return true;
    default:
      return false;
  }
}

// Scheme characters: ALPHA / DIGIT / "+" / "-" / "."
// See https://www.rfc-editor.org/rfc/rfc3986#section-3.1
inline auto uri_is_scheme_char(const char character) -> bool {
  if (std::isalnum(static_cast<unsigned char>(character))) {
    return true;
  }

  switch (character) {
    case URI_PLUS:
    case URI_HYPHEN:
    case URI_DOT:
      return true;
    default:
      return false;
  }
}

// pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
// See https://www.rfc-editor.org/rfc/rfc3986#section-3.3
inline auto uri_is_pchar(const char character) -> bool {
  if (uri_is_unreserved(character) || uri_is_sub_delim(character)) {
    return true;
  }

  switch (character) {
    case URI_COLON:
    case URI_AT:
    case URI_PERCENT:
      return true;
    default:
      return false;
  }
}

} // namespace sourcemeta::core

#endif
