#ifndef SOURCEMETA_CORE_HTTP_MESSAGE_H_
#define SOURCEMETA_CORE_HTTP_MESSAGE_H_

#include <sourcemeta/core/http_syntax.h>
#include <sourcemeta/core/text.h>

#include <concepts>    // std::convertible_to, std::invocable
#include <cstddef>     // std::size_t
#include <optional>    // std::nullopt, std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace sourcemeta::core {

/// @ingroup http
/// Test whether a raw line opens a message header block per RFC 9112 §4.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_status_line("HTTP/1.1 200 OK"));
/// assert(!sourcemeta::core::http_is_status_line("Content-Type: text/html"));
/// ```
constexpr auto http_is_status_line(const std::string_view line) noexcept
    -> bool {
  // The prefix cannot open a field line, as RFC 9110 §5.6.2 defines tchar
  // as "any VCHAR, except delimiters" where delimiters include the slash,
  // and the protocol name is case-sensitive per RFC 9112 §2.3
  return line.starts_with("HTTP/");
}

/// @ingroup http
/// Accumulate raw header lines into a buffer, retaining only the block of
/// the most recent message, given that transparently following redirects or
/// receiving interim responses produces one header block per message. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// std::string buffer;
/// sourcemeta::core::http_accumulate_header_line(buffer,
///     "HTTP/1.1 301 Moved Permanently\r\n");
/// sourcemeta::core::http_accumulate_header_line(buffer,
///     "HTTP/1.1 200 OK\r\n");
/// assert(buffer == "HTTP/1.1 200 OK\r\n");
/// ```
template <typename Buffer>
  requires requires(Buffer buffer, std::string_view line) {
    buffer.clear();
    buffer.append(line);
  }
inline auto http_accumulate_header_line(Buffer &buffer,
                                        const std::string_view line) -> void {
  if (http_is_status_line(line)) {
    buffer.clear();
  }

  buffer.append(line);
}

/// @ingroup http
/// Parse the field lines of a raw message header block per RFC 9112 §5,
/// skipping the start line and invoking the callback with each raw field
/// name and its value with optional whitespace excluded. A continuation of
/// the previous value through deprecated line folding is reported with an
/// empty name. Malformed field lines are discarded. Neither argument
/// allocates, as both are views into the input. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string_view>
///
/// sourcemeta::core::http_parse_headers(
///     "HTTP/1.1 200 OK\r\nServer: test\r\n\r\n",
///     [](const std::string_view name, const std::string_view value) {
///       assert(name == "Server");
///       assert(value == "test");
///     });
/// ```
template <typename Callback>
  requires std::invocable<Callback, std::string_view, std::string_view>
inline auto http_parse_headers(const std::string_view input, Callback callback)
    -> void {
  std::size_t cursor{input.find("\r\n")};
  while (cursor != std::string_view::npos) {
    cursor += 2;
    const auto end{input.find("\r\n", cursor)};
    if (end == std::string_view::npos || end == cursor) {
      break;
    }

    auto line{input.substr(cursor, end - cursor)};
    cursor = end;

    // RFC 9112 §5.2 deprecates obs-fold, defined as "OWS CRLF RWS", and
    // mandates that a user agent "MUST replace each received obs-fold
    // with one or more SP octets prior to interpreting the field value",
    // so a line opening with whitespace continues the previous field
    // line value and is reported with an empty name for the caller to join
    if (line.front() == ' ' || line.front() == '\t') {
      while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
        line.remove_prefix(1);
      }

      while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
        line.remove_suffix(1);
      }

      // RFC 9110 §5.5: "a recipient of CR, LF, or NUL within a field value
      // MUST either reject the message or replace each of those characters
      // with SP", so a continuation carrying a bare one is discarded
      if (http_field_line_has_forbidden_byte(line)) {
        continue;
      }

      callback(std::string_view{}, line);
      continue;
    }

    const auto parts{split_once(line, ':')};
    if (!parts.has_value() || parts->first.empty()) {
      continue;
    }

    const auto name{parts->first};
    // RFC 9112 §5.1 mandates that "no whitespace is allowed between the
    // field name and colon" because in the past such whitespace has "led
    // to security vulnerabilities", so the field line is discarded
    if (name.back() == ' ' || name.back() == '\t') {
      continue;
    }

    auto value{parts->second};
    // RFC 9112 §5 defines a field line as
    // `field-name ":" OWS field-value OWS` where RFC 9110 §5.6.3 defines
    // optional whitespace as `*( SP / HTAB )`
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
      value.remove_prefix(1);
    }

    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) {
      value.remove_suffix(1);
    }

    // RFC 9110 §5.5: "a recipient of CR, LF, or NUL within a field value MUST
    // either reject the message or replace each of those characters with SP",
    // so a field line whose name or value carries a bare one that survived the
    // split on the line terminator is discarded
    if (http_field_line_has_forbidden_byte(name) ||
        http_field_line_has_forbidden_byte(value)) {
      continue;
    }

    callback(name, value);
  }
}

/// @ingroup http
/// Parse the value of an RFC 6265 §4.2 `Cookie` request header, given without
/// the field name, into its cookie-pairs, invoking the callback once per pair
/// with the name and value. A request cookie header carries only names and
/// values, never attributes. Surrounding whitespace is trimmed, values are
/// otherwise reported verbatim, and pairs that lack a `=` or have an empty name
/// are skipped. Neither argument allocates, as both are borrowed from the
/// input, so anything the callback keeps must not outlive it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string_view>
///
/// std::size_t count{0};
/// sourcemeta::core::http_parse_cookies(
///     "session=abc; theme=dark",
///     [&count](const std::string_view, const std::string_view) {
///       count += 1;
///     });
/// assert(count == 2);
/// ```
///
/// A header may carry several cookies under one name, so a callback that
/// assigns to a single variable keeps whichever happens to come last. Use
/// `http_cookie_values` to look one up by name.
template <typename Callback>
  requires std::invocable<Callback, std::string_view, std::string_view>
inline auto http_parse_cookies(const std::string_view input, Callback callback)
    -> void {
  std::string_view rest{input};
  while (!rest.empty()) {
    std::string_view pair;
    const auto separator{rest.find(';')};
    if (separator == std::string_view::npos) {
      pair = rest;
      rest = {};
    } else {
      pair = rest.substr(0, separator);
      rest = rest.substr(separator + 1);
    }

    const auto parts{split_once(trim(pair), '=')};
    if (!parts.has_value()) {
      continue;
    }

    const auto name{trim(parts->first)};
    if (name.empty()) {
      continue;
    }

    callback(name, trim(parts->second));
  }
}

/// @ingroup http
/// Parse the value of an RFC 6265 §4.2 `Cookie` request header, given without
/// the field name, into any container of name and value pairs. The names and
/// values are borrowed from `input` rather than copied, so a container of views
/// must not outlive it, while an owning container may. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string_view>
/// #include <utility>
/// #include <vector>
///
/// std::vector<std::pair<std::string_view, std::string_view>> cookies;
/// sourcemeta::core::http_parse_cookies("session=abc; theme=dark", cookies);
/// assert(cookies.size() == 2);
/// assert(cookies.at(0).first == "session");
/// assert(cookies.at(0).second == "abc");
/// ```
template <typename Container>
  requires requires(Container container, std::string_view entry) {
    container.emplace_back(entry, entry);
  }
inline auto http_parse_cookies(const std::string_view input, Container &cookies)
    -> void {
  http_parse_cookies(input,
                     [&cookies](const std::string_view name,
                                const std::string_view value) -> void {
                       cookies.emplace_back(name, value);
                     });
}

/// @ingroup http
/// Collect every value carried under the given cookie name, in the order the
/// header presents them. A request may carry several cookies with one name,
/// since a parent domain and the host itself can each set one and RFC 6265
/// §4.2.2 notes the server "cannot determine from the Cookie header alone [...]
/// for which hosts the cookie is valid". That section also warns that servers
/// "SHOULD NOT rely upon the order in which these cookies appear", so a caller
/// verifying a signed cookie tries every value rather than any single one.
/// Naming a cookie with the RFC 6265bis §4.1.3.2 `__Host-` prefix prevents the
/// collision at the source.
///
/// The values are borrowed from `input` rather than copied, so a container of
/// views must not outlive it, while an owning container may. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string_view>
/// #include <vector>
///
/// std::vector<std::string_view> values;
/// sourcemeta::core::http_cookie_values("session=abc; theme=dark; session=xyz",
///                                      "session", values);
/// assert(values.size() == 2);
/// assert(values.at(0) == "abc");
/// assert(values.at(1) == "xyz");
/// ```
template <typename Container>
  requires requires(Container container, std::string_view entry) {
    container.emplace_back(entry);
  }
inline auto http_cookie_values(const std::string_view input,
                               const std::string_view name, Container &values)
    -> void {
  http_parse_cookies(input,
                     [&name, &values](const std::string_view cookie,
                                      const std::string_view value) -> void {
                       if (cookie == name) {
                         values.emplace_back(value);
                       }
                     });
}

/// @ingroup http
/// Parse the field lines of a raw message header block, skipping the start
/// line, into any container of name and value pairs, normalising names to
/// lowercase given that RFC 9110 §5.1 mandates that "field names are
/// case-insensitive", preserving repeated fields as separate entries, and
/// joining deprecated line folding per RFC 9112 §5.2. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
/// #include <utility>
/// #include <vector>
///
/// std::vector<std::pair<std::string, std::string>> headers;
/// sourcemeta::core::http_parse_headers(
///     "HTTP/1.1 200 OK\r\nServer: test\r\n\r\n", headers);
/// assert(headers.size() == 1);
/// assert(headers.at(0).first == "server");
/// assert(headers.at(0).second == "test");
/// ```
template <typename Container>
  requires requires(Container container, std::string name, std::string value,
                    const char character) {
    container.emplace_back(std::move(name), std::move(value));
    { container.empty() } -> std::convertible_to<bool>;
    container.back().second += character;
  }
inline auto http_parse_headers(const std::string_view input, Container &headers)
    -> void {
  http_parse_headers(input,
                     [&headers](const std::string_view name,
                                const std::string_view value) -> auto {
                       if (name.empty()) {
                         // RFC 9112 §5.2 mandates replacing "each received
                         // obs-fold with one or more SP octets prior to
                         // interpreting the field value"
                         if (!headers.empty()) {
                           auto &previous_value{headers.back().second};
                           previous_value += ' ';
                           previous_value += value;
                         }

                         return;
                       }

                       std::string header_name{name};
                       to_lowercase(header_name);
                       headers.emplace_back(std::move(header_name),
                                            std::string{value});
                     });
}

/// @ingroup http
/// Serialise headers, given as any range of name and value pairs, into
/// CRLF-delimited field lines per RFC 9112 §5. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
/// #include <utility>
/// #include <vector>
///
/// const std::vector<std::pair<std::string, std::string>> headers{
///     {"Accept", "application/json"}};
/// assert(sourcemeta::core::http_serialize_headers(headers) ==
///        "Accept: application/json\r\n");
/// ```
template <typename Headers>
inline auto http_serialize_headers(const Headers &headers) -> std::string {
  std::size_t total_size{0};
  for (const auto &[name, value] : headers) {
    // A field dropped for a forbidden byte contributes no bytes, so the reserve
    // is sized from only the fields that are actually emitted
    if (http_field_line_has_forbidden_byte(name)) {
      continue;
    }
    // Account for the colon, the space, and the trailing CRLF
    if constexpr (requires { value.bytes(); }) {
      if (http_field_line_has_forbidden_byte(value.bytes())) {
        continue;
      }
      total_size += name.size() + value.bytes().size() + 4;
    } else {
      if (http_field_line_has_forbidden_byte(value)) {
        continue;
      }
      total_size += name.size() + value.size() + 4;
    }
  }

  std::string result;
  result.reserve(total_size);
  for (const auto &[name, value] : headers) {
    // RFC 9110 §5.5: "a recipient of CR, LF, or NUL within a field value MUST
    // either reject the message or replace each of those characters with SP
    // before further processing or forwarding of that message", so a field
    // whose name carries one is dropped rather than written to the wire as a
    // header-injection defense
    if (http_field_line_has_forbidden_byte(name)) {
      continue;
    }

    // RFC 9112 §5.1 notes that "a single SP preceding the field line
    // value is preferred for consistent readability by humans"
    if constexpr (requires { value.bytes(); }) {
      if (http_field_line_has_forbidden_byte(value.bytes())) {
        continue;
      }

      result += name;
      result += ": ";
      result += value.bytes();
    } else {
      if (http_field_line_has_forbidden_byte(value)) {
        continue;
      }

      result += name;
      result += ": ";
      result += value;
    }

    result += "\r\n";
  }

  return result;
}

/// @ingroup http
/// Find the value of the first header with the given lowercase name in any
/// range of name and value pairs, returning no result when absent. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
/// #include <utility>
/// #include <vector>
///
/// const std::vector<std::pair<std::string, std::string>> headers{
///     {"server", "test"}};
/// assert(sourcemeta::core::http_header_find(headers, "server").has_value());
/// assert(!sourcemeta::core::http_header_find(headers, "date").has_value());
/// ```
template <typename Headers>
inline auto http_header_find(const Headers &headers,
                             const std::string_view name)
    -> std::optional<std::string_view> {
  // Prefer the container's own lookup when it supports searching by the
  // given name without converting it, as associative containers do it in
  // logarithmic or constant time instead of a linear scan
  if constexpr (requires { headers.find(name) != headers.end(); }) {
    const auto match{headers.find(name)};
    if (match != headers.end()) {
      return std::string_view{match->second};
    }

    return std::nullopt;
  } else {
    for (const auto &[header_name, header_value] : headers) {
      if (header_name == name) {
        return std::string_view{header_value};
      }
    }

    return std::nullopt;
  }
}

} // namespace sourcemeta::core

#endif
