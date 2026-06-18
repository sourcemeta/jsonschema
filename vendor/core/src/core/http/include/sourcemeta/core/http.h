#ifndef SOURCEMETA_CORE_HTTP_H_
#define SOURCEMETA_CORE_HTTP_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/http_error.h>
#include <sourcemeta/core/http_message.h>
#include <sourcemeta/core/http_method.h>
#include <sourcemeta/core/http_problem.h>
#include <sourcemeta/core/http_status.h>
#include <sourcemeta/core/http_system.h>
// NOLINTEND(misc-include-cleaner)

#include <chrono>           // std::chrono::system_clock
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <optional>         // std::optional
#include <span>             // std::span
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <utility>          // std::pair

/// @defgroup http HTTP
/// @brief An implementation of HTTP-protocol parsing, formatting, and
/// validation primitives per RFC 9110.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// ```

namespace sourcemeta::core {

/// @ingroup http
/// A content coding supported by this implementation.
enum class HTTPContentEncoding : std::uint8_t {
  Identity,
  GZIP,
};

/// @ingroup http
/// Pick the best media-type candidate against an `Accept` header per RFC 9110
/// §12.5.1. Returns an empty value when no candidate is acceptable. The
/// returned view borrows from `candidates`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto best{sourcemeta::core::http_match_accept(
///     "text/html, application/json;q=0.9",
///     {"text/html", "application/json"})};
/// assert(best == "text/html");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_match_accept(const std::string_view accept_header,
                       std::initializer_list<std::string_view> candidates)
    -> std::string_view;

/// @ingroup http
/// Test whether every media type is individually acceptable under an `Accept`
/// header per RFC 9110 §12.5.1. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_accept_includes_all(
///     "text/html, application/json",
///     {"text/html", "application/json"}));
/// assert(!sourcemeta::core::http_accept_includes_all(
///     "text/html;q=0, application/json", {"text/html"}));
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_accept_includes_all(
    const std::string_view accept_header,
    std::initializer_list<std::string_view> media_types) noexcept -> bool;

/// @ingroup http
/// Test whether a `Content-Type` header denotes the given media type per RFC
/// 9110 §8.3.1. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_content_type_matches(
///     "application/json; charset=UTF-8", "application/json"));
/// assert(!sourcemeta::core::http_content_type_matches(
///     "application/xml", "application/json"));
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_content_type_matches(const std::string_view content_type_header,
                               const std::string_view media_type) noexcept
    -> bool;

/// @ingroup http
/// Pick the best language-tag candidate against an `Accept-Language` header
/// per RFC 9110 §12.5.4 using the RFC 4647 §3.4 Lookup scheme. Returns an
/// empty value when no candidate is acceptable. The returned view borrows from
/// `candidates`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto best{sourcemeta::core::http_match_accept_language(
///     "fr-CA;q=0.9, en;q=0.8", {"en", "fr"})};
/// assert(best == "fr");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_match_accept_language(
    const std::string_view accept_language_header,
    std::initializer_list<std::string_view> candidates) -> std::string_view;

/// @ingroup http
/// Resolve a content coding against an `Accept-Encoding` header per RFC 9110
/// §12.5.3. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto chosen{sourcemeta::core::http_negotiate_encoding(
///     "gzip, identity;q=0.5", sourcemeta::core::HTTPContentEncoding::GZIP)};
/// assert(chosen.has_value());
/// assert(chosen.value() == sourcemeta::core::HTTPContentEncoding::GZIP);
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_negotiate_encoding(
    const std::string_view accept_encoding_header,
    const HTTPContentEncoding server_preference) noexcept
    -> std::optional<HTTPContentEncoding>;

/// @ingroup http
/// Parse an HTTP-date string per RFC 9110 §5.6.7. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_from_date(
///     "Sun, 06 Nov 1994 08:49:37 GMT").has_value());
/// assert(sourcemeta::core::http_from_date(
///     "Sunday, 06-Nov-94 08:49:37 GMT").has_value());
/// assert(sourcemeta::core::http_from_date(
///     "Sun Nov  6 08:49:37 1994").has_value());
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_from_date(const std::string_view value) noexcept
    -> std::optional<std::chrono::system_clock::time_point>;

/// @ingroup http
/// A typed RFC 8288 §3 link-value. The caller owns the backing storage for
/// every field, must URI-escape `target`, and must ensure parameter values are
/// valid `quoted-string` content.
struct HTTPLink {
  std::string_view target;
  std::string_view rel;
  std::span<const std::pair<std::string_view, std::string_view>> parameters{};
};

/// @ingroup http
/// Append an RFC 8288 §3 link-value to `out`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// std::string buffer{"prefix:"};
/// sourcemeta::core::http_format_link(
///     {.target = "/schema.json", .rel = "describedby"}, buffer);
/// assert(buffer == "prefix:</schema.json>; rel=\"describedby\"");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_format_link(const HTTPLink &link, std::string &out) -> void;

/// @ingroup http
/// Format an RFC 8288 §3 link-value. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto value{sourcemeta::core::http_format_link(
///     {.target = "https://example.com/schema.json", .rel = "describedby"})};
/// assert(value ==
///   "<https://example.com/schema.json>; rel=\"describedby\"");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_format_link(const HTTPLink &link) -> std::string;

/// @ingroup http
/// Append an RFC 8288 §3.5 comma-separated multi-link value to `out`. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// const sourcemeta::core::HTTPLink links[]{
///     {.target = "/here", .rel = "self"},
///     {.target = "/next", .rel = "next"}};
/// std::string buffer;
/// sourcemeta::core::http_format_links(links, buffer);
/// assert(buffer ==
///   "</here>; rel=\"self\", </next>; rel=\"next\"");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_format_links(std::span<const HTTPLink> links, std::string &out)
    -> void;

/// @ingroup http
/// Format an RFC 8288 §3.5 comma-separated multi-link value. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const sourcemeta::core::HTTPLink links[]{
///     {.target = "/here", .rel = "self"},
///     {.target = "/next", .rel = "next"}};
/// const auto value{sourcemeta::core::http_format_links(links)};
/// assert(value ==
///   "</here>; rel=\"self\", </next>; rel=\"next\"");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_format_links(std::span<const HTTPLink> links) -> std::string;

/// @ingroup http
/// Test whether a comma-separated header value per RFC 9110 §5.6.1 lists any
/// of the given tokens. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_field_list_contains_any(
///     "\"abc\", W/\"def\", *", {"*"}));
/// assert(!sourcemeta::core::http_field_list_contains_any(
///     "\"abc\", \"def\"", {"\"xyz\""}));
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_field_list_contains_any(
    const std::string_view header_value,
    std::initializer_list<std::string_view> tokens) noexcept -> bool;

/// @ingroup http
/// Extract the credential from an `Authorization` header that uses the Bearer
/// scheme per RFC 6750 §2.1, matching the scheme case-insensitively per RFC
/// 9110 §11.1 and tolerating optional whitespace around the token. Returns an
/// empty view when the header is absent, uses another scheme, or does not carry
/// a well-formed `b64token` credential. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_parse_bearer("Bearer abc123") == "abc123");
/// assert(sourcemeta::core::http_parse_bearer("Basic abc123").empty());
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_parse_bearer(const std::string_view authorization) noexcept
    -> std::string_view;

} // namespace sourcemeta::core

#endif
