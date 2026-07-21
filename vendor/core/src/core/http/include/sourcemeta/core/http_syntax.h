#ifndef SOURCEMETA_CORE_HTTP_SYNTAX_H_
#define SOURCEMETA_CORE_HTTP_SYNTAX_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/text.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup http
/// Whether a character is optional whitespace, a space or a horizontal tab
/// (RFC 9110 Section 5.6.3). For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_ows(' '));
/// assert(!sourcemeta::core::http_is_ows('x'));
/// ```
inline auto http_is_ows(const char character) noexcept -> bool {
  return character == ' ' || character == '\t';
}

/// @ingroup http
/// Whether a character is a token character (RFC 9110 Section 5.6.2), the set a
/// bare header token draws from. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_tchar('a'));
/// assert(!sourcemeta::core::http_is_tchar('"'));
/// ```
inline auto http_is_tchar(const char character) noexcept -> bool {
  switch (character) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
      return true;
    default:
      return is_alphanum(character);
  }
}

/// @ingroup http
/// Whether a character may appear in the alphabet of a token68 (RFC 7235
/// Section 2.1) or b64token (RFC 6750 Section 2.1) credential, before its
/// padding. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_b64token_char('/'));
/// assert(!sourcemeta::core::http_is_b64token_char('='));
/// ```
inline auto http_is_b64token_char(const char character) noexcept -> bool {
  return is_alphanum(character) || character == '-' || character == '.' ||
         character == '_' || character == '~' || character == '+' ||
         character == '/';
}

/// @ingroup http
/// Whether a string is a well-formed token68 (RFC 7235 Section 2.1) or b64token
/// (RFC 6750 Section 2.1), at least one alphabet character followed by any "="
/// padding. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_b64token("YWJj=="));
/// assert(!sourcemeta::core::http_is_b64token("a b"));
/// ```
inline auto http_is_b64token(const std::string_view value) noexcept -> bool {
  std::size_t position{0};
  while (position < value.size() && http_is_b64token_char(value[position])) {
    position += 1;
  }

  if (position == 0) {
    return false;
  }

  while (position < value.size()) {
    if (value[position] != '=') {
      return false;
    }

    position += 1;
  }

  return true;
}

/// @ingroup http
/// Whether a string is a well-formed token (RFC 9110 Section 5.6.2), at least
/// one token character and nothing else. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_token("Bearer"));
/// assert(!sourcemeta::core::http_is_token("has space"));
/// ```
inline auto http_is_token(const std::string_view value) noexcept -> bool {
  if (value.empty()) {
    return false;
  }

  for (const auto character : value) {
    if (!http_is_tchar(character)) {
      return false;
    }
  }

  return true;
}

/// @ingroup http
/// Append a string to the sink as a quoted-string (RFC 9110 Section 5.6.4),
/// wrapping it in double quotes and escaping any double quote or backslash,
/// returning whether it was encodable. Nothing is appended when the string
/// carries a byte no quoted-string admits, such as a control character, which
/// keeps a value from injecting into a header. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// std::string sink;
/// assert(sourcemeta::core::http_encode_quoted_string(R"(a"b)", sink));
/// assert(sink == R"("a\"b")");
/// ```
inline auto http_encode_quoted_string(const std::string_view value,
                                      std::string &sink) -> bool {
  for (const auto character : value) {
    const auto byte{static_cast<unsigned char>(character)};
    // RFC 9110 Section 5.6.4: qdtext and quoted-pair admit HTAB, SP through
    // VCHAR, and obs-text, so every other control character (CR and LF among
    // them) makes the value unencodable
    if (byte != 0x09 && (byte < 0x20 || byte == 0x7F)) {
      return false;
    }
  }

  sink.push_back('"');
  for (const auto character : value) {
    if (character == '"' || character == '\\') {
      sink.push_back('\\');
    }

    sink.push_back(character);
  }

  sink.push_back('"');
  return true;
}

/// @ingroup http
/// The view with any leading optional whitespace removed (RFC 9110
/// Section 5.6.3). For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_trim_leading_ows("  x") == "x");
/// ```
inline auto http_trim_leading_ows(std::string_view value) noexcept
    -> std::string_view {
  while (!value.empty() && http_is_ows(value.front())) {
    value.remove_prefix(1);
  }

  return value;
}

/// @ingroup http
/// The view with any trailing optional whitespace removed (RFC 9110
/// Section 5.6.3). For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_trim_trailing_ows("x  ") == "x");
/// ```
inline auto http_trim_trailing_ows(std::string_view value) noexcept
    -> std::string_view {
  while (!value.empty() && http_is_ows(value.back())) {
    value.remove_suffix(1);
  }

  return value;
}

/// @ingroup http
/// Scan a quoted-string (RFC 9110 Section 5.6.4) whose opening double quote is
/// at position in input, returning the position just past the closing quote, or
/// no value when it is malformed. The unescaped content is written to value,
/// borrowing from input when it carries no quoted-pair and appending to storage
/// otherwise, whose capacity should be reserved up front so that earlier views
/// into it stay valid. Control characters other than horizontal tab are
/// rejected as a header-injection defense. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
/// #include <string_view>
///
/// std::string storage;
/// std::string_view value;
/// const auto next{sourcemeta::core::http_scan_quoted_string(
///     R"("hello")", 0, storage, value)};
/// assert(next.has_value());
/// assert(value == "hello");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_scan_quoted_string(const std::string_view input,
                             const std::size_t position, std::string &storage,
                             std::string_view &value)
    -> std::optional<std::size_t>;

} // namespace sourcemeta::core

#endif
