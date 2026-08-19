#ifndef SOURCEMETA_CORE_HTTP_H_
#define SOURCEMETA_CORE_HTTP_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/http_aws_sigv4.h>
#include <sourcemeta/core/http_error.h>
#include <sourcemeta/core/http_message.h>
#include <sourcemeta/core/http_method.h>
#include <sourcemeta/core/http_problem.h>
#include <sourcemeta/core/http_status.h>
#include <sourcemeta/core/http_syntax.h>
#include <sourcemeta/core/http_system.h>
// NOLINTEND(misc-include-cleaner)

#include <chrono>           // std::chrono::system_clock
#include <cstddef>          // std::size_t
#include <cstdint>          // std::uint8_t
#include <initializer_list> // std::initializer_list
#include <optional>         // std::optional
#include <span>             // std::span
#include <string>           // std::string
#include <string_view>      // std::string_view
#include <utility>          // std::pair
#include <vector>           // std::vector

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
  /// The identity coding that applies no transformation.
  Identity,
  /// The gzip coding per RFC 9110 §8.4.1.3.
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
/// per RFC 9110 §12.5.4, which lets an implementation choose its matching
/// scheme. This uses a q-aware Basic-Filtering-style scheme (RFC 4647 §3.3.1,
/// the scheme RFC 9110 §12.5.4 points to) that also honors `q=0` exclusions.
/// Returns an empty value when no candidate is acceptable. The returned view
/// borrows from `candidates`. For example:
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
/// Read the `max-age` response directive from a `Cache-Control` header value
/// per RFC 9111 §5.2.2.1. Returns an empty value when the directive is absent
/// or malformed. A value larger than the cache can represent saturates to
/// `2147483648` seconds as mandated by RFC 9111 §1.2.2. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <chrono>
///
/// const auto max_age{sourcemeta::core::http_cache_control_max_age(
///     "public, max-age=600")};
/// assert(max_age.has_value());
/// assert(max_age.value() == std::chrono::seconds{600});
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_cache_control_max_age(const std::string_view cache_control) noexcept
    -> std::optional<std::chrono::seconds>;

/// @ingroup http
/// A typed RFC 8288 §3 link-value. The caller owns the backing storage for
/// every field, must URI-escape `target`, and must ensure parameter values are
/// valid `quoted-string` content.
struct HTTPLink {
  /// The link target reference
  std::string_view target;
  /// The link relation type
  std::string_view rel;
  /// The additional target attributes of the link
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
/// Whether a cached response may be stored by a shared cache or only by a
/// private one (RFC 9111 §5.2.2.7 and §5.2.2.9). The two are alternatives
/// rather than flags, so a response cannot be marked both.
enum class HTTPCacheVisibility : std::uint8_t {
  /// A cache may store the response even where it would otherwise be
  /// prohibited, which includes a shared cache reusing a response to an
  /// authorized request.
  Public,
  /// A shared cache must not store the response, the response being intended
  /// for a single user.
  Private
};

/// @ingroup http
/// The response directives to serialise into an RFC 9111 §5.2 `Cache-Control`
/// header value. The caller owns the backing storage for every field.
///
/// Whether a response may be stored by a shared cache is an access decision as
/// much as a performance one, so it is carried as one alternative rather than
/// as two independent flags that could both be set.
struct HTTPCacheControl {
  /// Which caches may store the response
  std::optional<HTTPCacheVisibility> visibility{};
  /// How long the response stays fresh (RFC 9111 §5.2.2.1)
  std::optional<std::chrono::seconds> max_age{};
  /// How long the response stays fresh for a shared cache, overriding the
  /// above for one (RFC 9111 §5.2.2.10)
  std::optional<std::chrono::seconds> shared_max_age{};
  /// Whether no cache may store any part of the exchange (RFC 9111 §5.2.2.5)
  bool no_store{false};
  /// Whether the response must be validated before every reuse (RFC 9111
  /// §5.2.2.4), ignored when a field list qualifies it below
  bool no_cache{false};
  /// Whether a stale response must be validated before reuse (RFC 9111
  /// §5.2.2.2)
  bool must_revalidate{false};
  /// The same for a shared cache alone (RFC 9111 §5.2.2.8)
  bool proxy_revalidate{false};
  /// Whether storing is limited to a cache that implements the status code's
  /// caching requirements (RFC 9111 §5.2.2.3)
  bool must_understand{false};
  /// Whether the payload must not be transformed (RFC 9111 §5.2.2.6)
  bool no_transform{false};
  /// Whether the response will not be updated while fresh (RFC 8246)
  bool immutable{false};
  /// The fields a shared cache must not store, which qualifies the private
  /// directive and so requires it (RFC 9111 §5.2.2.7)
  std::span<const std::string_view> private_fields{};
  /// The fields that must be revalidated before reuse, which qualifies the
  /// no-cache directive (RFC 9111 §5.2.2.4)
  std::span<const std::string_view> no_cache_fields{};
};

/// @ingroup http
/// Test whether a set of directives can be serialised into a valid RFC 9111
/// §5.2 `Cache-Control` header value: every duration is the non-negative
/// integer §1.2.2 defines, every qualifying field name is an RFC 9110 §5.6.2
/// token, a private field list is accompanied by the directive it qualifies,
/// and at least one directive is named, since §5.2 lists them and RFC 9110
/// §5.6.1.1 forbids a sender from generating an empty list element.
SOURCEMETA_CORE_HTTP_EXPORT
auto http_cache_control_valid(const HTTPCacheControl &directives) -> bool;

/// @ingroup http
/// Append an RFC 9111 §5.2 `Cache-Control` header value to `out`, returning
/// `true` on success. When the directives are not `http_cache_control_valid`,
/// `out` is left unchanged and this returns `false`.
///
/// A duration is written in the token form §5.2.2.1 requires, never quoted,
/// while a field list is written in the quoted-string form §5.2.2.4 and
/// §5.2.2.7 ask a sender to use even for a single entry. A qualifying field
/// list replaces the bare directive rather than joining it. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <chrono>
/// #include <string>
///
/// std::string buffer;
/// const auto ok{sourcemeta::core::http_serialize_cache_control(
///     {.visibility = sourcemeta::core::HTTPCacheVisibility::Private,
///      .max_age = std::chrono::seconds{60}}, buffer)};
/// assert(ok);
/// assert(buffer == "private, max-age=60");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_cache_control(const HTTPCacheControl &directives,
                                  std::string &out) -> bool;

/// @ingroup http
/// Serialise an RFC 9111 §5.2 `Cache-Control` header value, returning no value
/// when the directives are not `http_cache_control_valid`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto value{sourcemeta::core::http_serialize_cache_control(
///     {.no_store = true})};
/// assert(value == "no-store");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_cache_control(const HTTPCacheControl &directives)
    -> std::optional<std::string>;

/// @ingroup http
/// A challenge to serialise into an RFC 9110 §11.6.1 `WWW-Authenticate`
/// response header value. The caller owns the backing storage for every field.
///
/// RFC 9110 §11.3 gives `challenge = auth-scheme [ 1*SP ( token68 /
/// #auth-param ) ]`, so a challenge carries either a credential or a parameter
/// list and never both.
struct HTTPChallenge {
  /// The authentication scheme
  std::string_view scheme{};
  /// The credential the challenge carries in place of parameters
  std::optional<std::string_view> token68{};
  /// The authentication parameters
  std::span<const std::pair<std::string_view, std::string_view>> parameters{};
};

/// @ingroup http
/// Test whether a challenge can be serialised into a valid RFC 9110 §11.6.1
/// `WWW-Authenticate` header value: the scheme is a non-empty RFC 9110 §5.6.2
/// token, a present credential is a §11.2 token68 and stands alone, every
/// parameter name is a token, no parameter name repeats under the
/// case-insensitive matching §11.2 mandates, and every value is encodable as a
/// §5.6.4 quoted-string.
///
/// A challenge naming the Bearer scheme is held to RFC 6750 §3 as well, which
/// requires at least one parameter and bounds the octets `scope`, `error`,
/// `error_description` and `error_uri` may carry. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
/// #include <utility>
///
/// const std::array<std::pair<std::string_view, std::string_view>, 1>
///     parameters{{{"realm", "example"}}};
/// assert(sourcemeta::core::http_challenge_valid(
///     {.scheme = "Bearer", .parameters = parameters}));
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_challenge_valid(const HTTPChallenge &challenge) -> bool;

/// @ingroup http
/// Append an RFC 9110 §11.6.1 `WWW-Authenticate` challenge to `out`, returning
/// `true` on success. When the challenge is not `http_challenge_valid`, `out`
/// is left unchanged and this returns `false`.
///
/// Every parameter value is spelled as a §5.6.4 quoted-string, since §11.5
/// leaves a sender no other choice for a realm, and the encoding escapes a
/// quote or a backslash rather than letting either close the value early. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <array>
/// #include <cassert>
/// #include <string>
/// #include <string_view>
/// #include <utility>
///
/// const std::array<std::pair<std::string_view, std::string_view>, 1>
///     parameters{{{"realm", "example"}}};
/// std::string buffer;
/// const auto ok{sourcemeta::core::http_serialize_challenge(
///     {.scheme = "Bearer", .parameters = parameters}, buffer)};
/// assert(ok);
/// assert(buffer == "Bearer realm=\"example\"");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_challenge(const HTTPChallenge &challenge, std::string &out)
    -> bool;

/// @ingroup http
/// Serialise an RFC 9110 §11.6.1 `WWW-Authenticate` challenge, returning no
/// value when the challenge is not `http_challenge_valid`.
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_challenge(const HTTPChallenge &challenge)
    -> std::optional<std::string>;

/// @ingroup http
/// Append a whole RFC 9110 §11.6.1 `WWW-Authenticate` header value to `out`,
/// which is a list of challenges. Returns `false` without touching `out` when
/// the list is empty, since §11.6.1 requires a 401 to carry at least one
/// challenge, or when any challenge is not `http_challenge_valid`.
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_challenges(std::span<const HTTPChallenge> challenges,
                               std::string &out) -> bool;

/// @ingroup http
/// Serialise a whole RFC 9110 §11.6.1 `WWW-Authenticate` header value,
/// returning no value when the list is empty or carries an invalid challenge.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
/// #include <utility>
///
/// const std::array<std::pair<std::string_view, std::string_view>, 1>
///     parameters{{{"realm", "api"}}};
/// const std::array<sourcemeta::core::HTTPChallenge, 1> challenges{
///     {{.scheme = "Bearer", .parameters = parameters}}};
/// const auto value{sourcemeta::core::http_serialize_challenges(challenges)};
/// assert(value == "Bearer realm=\"api\"");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_challenges(std::span<const HTTPChallenge> challenges)
    -> std::optional<std::string>;

/// @ingroup http
/// A challenge read out of an RFC 9110 §11.6.1 `WWW-Authenticate` header
/// value. Unlike the challenge a caller hands to the serialiser, this one owns
/// its strings, since a quoted-pair means a value does not always appear
/// verbatim in the field it was read from.
struct HTTPParsedChallenge {
  /// The authentication scheme, as spelled in the field
  std::string scheme;
  /// The credential the challenge carried in place of parameters
  std::optional<std::string> token68;
  /// The authentication parameters, in the order they appeared
  std::vector<std::pair<std::string, std::string>> parameters;
};

/// @ingroup http
/// Parse an RFC 9110 §11.6.1 `WWW-Authenticate` header value, given without
/// the field name, into its challenges, returning `false` and leaving the
/// container empty when the value is malformed or carries no challenge at all.
///
/// The field is a list of challenges whose parameters are themselves
/// comma-separated, so §11.6.1 warns recipients to take special care. A token
/// followed by an equals sign continues the challenge being read, while one
/// that is not opens the next, and a run of token characters closed by equals
/// padding and nothing else is a §11.2 token68 rather than a parameter without
/// a value. Empty list elements are ignored as §5.6.1.2 requires.
///
/// This applies the RFC 9110 grammar alone. A recipient judges nothing beyond
/// it, so a challenge that `http_challenge_valid` would refuse a sender, such
/// as a bare Bearer, still parses. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <vector>
///
/// std::vector<sourcemeta::core::HTTPParsedChallenge> challenges;
/// const auto ok{sourcemeta::core::http_parse_challenges(
///     "Bearer realm=\"example\"", challenges)};
/// assert(ok);
/// assert(challenges.size() == 1);
/// assert(challenges.at(0).scheme == "Bearer");
/// assert(challenges.at(0).parameters.at(0).second == "example");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_parse_challenges(const std::string_view input,
                           std::vector<HTTPParsedChallenge> &challenges)
    -> bool;

/// @ingroup http
/// The `SameSite` attribute of a cookie per RFC 6265bis §5.2.
enum class HTTPCookieSameSite : std::uint8_t {
  /// The cookie is withheld from every cross-site request.
  Strict,
  /// The cookie is sent on top-level cross-site navigations only.
  Lax,
  /// The cookie is sent on every cross-site request.
  None
};

/// @ingroup http
/// A cookie to serialise into an RFC 6265 §4.1 `Set-Cookie` response header
/// value. The caller owns the backing storage for every field. A valid cookie
/// has a name that is an RFC 9110 §5.6.2 token and a value made of RFC 6265
/// §4.1.1 cookie-octets. RFC 6265bis §5.7 requires a cookie with a same-site
/// mode of none to also be secure.
struct HTTPCookie {
  /// The cookie name
  std::string_view name{};
  /// The cookie value
  std::string_view value{};
  /// The path the cookie is scoped to
  std::optional<std::string_view> path{};
  /// The host the cookie is scoped to
  std::optional<std::string_view> domain{};
  /// The cookie lifetime
  std::optional<std::chrono::seconds> max_age{};
  /// Whether the cookie is withheld from scripts
  bool http_only{false};
  /// Whether the cookie is only sent over secure channels
  bool secure{false};
  /// The cross-site request policy for the cookie
  std::optional<HTTPCookieSameSite> same_site{};
};

/// @ingroup http
/// The RFC 6265bis §5.7 ceiling on the sum of the lengths of a cookie name and
/// its value, past which a user agent ignores the cookie entirely. Note that
/// the RFC 6265 §6.1 minimum capability names the same number but measures the
/// attributes into it as well, so a cookie sized against this one is not
/// necessarily within that older bound.
inline constexpr std::size_t HTTP_COOKIE_MAXIMUM_NAME_VALUE_LENGTH{4096};

/// @ingroup http
/// The RFC 6265bis §5.6 ceiling on the length of a single cookie attribute
/// value, past which a user agent ignores that attribute while keeping the
/// cookie, silently widening or narrowing the scope the server asked for.
inline constexpr std::size_t HTTP_COOKIE_MAXIMUM_ATTRIBUTE_VALUE_LENGTH{1024};

/// @ingroup http
/// Test whether a cookie can be serialised into a valid RFC 6265 §4.1
/// `Set-Cookie` header value: the name is a non-empty RFC 9110 §5.6.2 token,
/// the value is made of RFC 6265 §4.1.1 cookie-octets, any path is made of RFC
/// 6265bis av-octets, any domain is a valid RFC 1123 host name allowing an
/// ignorable leading dot, a present `max_age` is not negative, per RFC 6265bis
/// §5.7 a `HTTPCookieSameSite::None` cookie is also `secure`, and the RFC
/// 6265bis §4.1.3 `__Secure-` and `__Host-` name prefixes carry their required
/// attributes. The RFC 6265bis §5.6 and §5.7 length ceilings are enforced too,
/// since a cookie past either is ignored in whole or in part by the user
/// agent. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_cookie_valid({.name = "a", .value = "b"}));
/// assert(!sourcemeta::core::http_cookie_valid({.name = "a", .value = "b;c"}));
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_cookie_valid(const HTTPCookie &cookie) -> bool;

/// @ingroup http
/// Append an RFC 6265 §4.1 `Set-Cookie` header value to `out`, returning `true`
/// on success. When the cookie is not `http_cookie_valid`, `out` is left
/// unchanged and this returns `false`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// std::string buffer;
/// const auto ok{sourcemeta::core::http_serialize_cookie(
///     {.name = "session", .value = "abc", .http_only = true}, buffer)};
/// assert(ok);
/// assert(buffer == "session=abc; HttpOnly");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_cookie(const HTTPCookie &cookie, std::string &out) -> bool;

/// @ingroup http
/// Serialise an RFC 6265 §4.1 `Set-Cookie` header value, returning no value
/// when the cookie is not `http_cookie_valid`. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto value{sourcemeta::core::http_serialize_cookie(
///     {.name = "session", .value = "abc", .secure = true})};
/// assert(value == "session=abc; Secure");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_serialize_cookie(const HTTPCookie &cookie)
    -> std::optional<std::string>;

/// @ingroup http
/// Derive the cookie that removes a cookie, carrying its name and every
/// attribute across so that the RFC 6265 §3.1 requirement to match the path
/// and the domain of the original is met by construction. The value is
/// dropped, and the lifetime is set to zero, which RFC 6265bis §5.6.2 maps to
/// "the earliest representable date and time". For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// const auto value{sourcemeta::core::http_serialize_cookie(
///     sourcemeta::core::http_expire_cookie(
///         {.name = "session", .value = "abc", .http_only = true}))};
/// assert(value == "session=; Max-Age=0; HttpOnly");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_expire_cookie(const HTTPCookie &cookie) -> HTTPCookie;

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
/// Append an RFC 9110 §12.5.5 `Vary` header value to `out`, returning `true`
/// on success. Every member must be a non-empty RFC 9110 §5.6.2 token, which
/// the wildcard also is, since §5.1 defines a field name as one and §5.6.1.1
/// forbids a sender from generating an empty list element. An empty list has
/// nothing to send, so the header is left to be omitted rather than emitted
/// blank. The spelling of each name is kept as given, as §5.1 makes field
/// names case-insensitive. Nothing is appended and this returns `false` when
/// any member is refused, which keeps a name from injecting a further field.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <array>
/// #include <cassert>
/// #include <string>
/// #include <string_view>
///
/// const std::array<std::string_view, 2> names{{"Accept", "Accept-Encoding"}};
/// std::string buffer;
/// const auto ok{sourcemeta::core::http_format_vary(names, buffer)};
/// assert(ok);
/// assert(buffer == "Accept, Accept-Encoding");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_format_vary(std::span<const std::string_view> field_names,
                      std::string &out) -> bool;

/// @ingroup http
/// Compose an RFC 9110 §12.5.5 `Vary` header value, returning no value when
/// any member is not an RFC 9110 §5.6.2 token or when there are none.
///
/// Note that RFC 9110 §12.5.5 forbids a proxy from generating the wildcard
/// member, so only an origin server may compose one. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// const std::array<std::string_view, 2> names{{"Accept", "Accept-Encoding"}};
/// const auto value{sourcemeta::core::http_format_vary(names)};
/// assert(value == "Accept, Accept-Encoding");
/// ```
SOURCEMETA_CORE_HTTP_EXPORT
auto http_format_vary(std::span<const std::string_view> field_names)
    -> std::optional<std::string>;

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
