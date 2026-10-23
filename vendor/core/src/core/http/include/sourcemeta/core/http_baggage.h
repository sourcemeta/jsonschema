#ifndef SOURCEMETA_CORE_HTTP_BAGGAGE_H_
#define SOURCEMETA_CORE_HTTP_BAGGAGE_H_

#ifndef SOURCEMETA_CORE_HTTP_EXPORT
#include <sourcemeta/core/http_export.h>
#endif

#include <sourcemeta/core/http_syntax.h>
#include <sourcemeta/core/text.h>

#include <cassert>     // assert
#include <concepts>    // std::invocable
#include <optional>    // std::optional
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup http
/// Whether the input is a well-formed baggage value, which is the part of a
/// member after its equals sign. The grammar is in W3C Baggage Section 3.3.1,
/// and this is stricter than it on one point, noted below.
///
/// The permitted octets leave out the control characters, whitespace, the
/// double quote, the comma, the semicolon and the backslash, so a value
/// carrying any of those has to percent-encode it. This also requires that a
/// percent introduces an encoded octet, which is stricter than the grammar,
/// where the percent is an octet like any other. The prose is what forbids a
/// bare one. An empty value is well formed, where a tracestate value has to
/// end in a mandatory non-blank character and so cannot be empty. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_baggage_value("alice"));
/// assert(sourcemeta::core::http_is_baggage_value("DF%2028"));
/// assert(sourcemeta::core::http_is_baggage_value(""));
/// assert(!sourcemeta::core::http_is_baggage_value("DF 28"));
/// assert(!sourcemeta::core::http_is_baggage_value("100%"));
/// ```
///
/// @see https://www.w3.org/TR/baggage/#value
/// @see https://www.w3.org/TR/trace-context/#value
SOURCEMETA_CORE_HTTP_EXPORT
auto http_is_baggage_value(const std::string_view value) noexcept -> bool;

/// @ingroup http
/// Whether the input is a well-formed baggage field value. The grammar is in
/// W3C Baggage Section 3.3.1, and whitespace at the two outer ends of the
/// input is accepted although the list production does not place any there,
/// since a field value does not carry such whitespace to begin with.
///
/// A member is a key and a value, optionally trailed by a semicolon-delimited
/// property set whose own entries may be a bare key. Whitespace around a key
/// or a value is allowed and is not part of it. A repeated key is allowed,
/// since "Uniqueness of keys between multiple list-members in a
/// baggage-string is not guaranteed", where tracestate holds that "Only one
/// entry per key is allowed".
///
/// The grammar caps the list at 180 members. The separate figures of 64
/// members and 8192 bytes are the point past which a platform "MAY drop
/// list-members" rather than a bound on what is well formed, so they are not
/// checked here. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::http_is_baggage(
///     "userId=alice,serverNode=DF%2028,isProduction=false"));
/// assert(sourcemeta::core::http_is_baggage("key1=value1;property1;k=v"));
/// assert(sourcemeta::core::http_is_baggage("userId = alice"));
/// assert(sourcemeta::core::http_is_baggage("a=1,a=2"));
/// assert(!sourcemeta::core::http_is_baggage(""));
/// assert(!sourcemeta::core::http_is_baggage("novalue"));
/// ```
///
/// @see https://www.w3.org/TR/baggage/#definition
/// @see https://www.w3.org/TR/baggage/#baggage-string
/// @see https://www.w3.org/TR/baggage/#limits
/// @see https://www.w3.org/TR/trace-context/#combined-header-value
/// @see https://www.rfc-editor.org/rfc/rfc9110#section-5.5
SOURCEMETA_CORE_HTTP_EXPORT
auto http_is_baggage(const std::string_view value) noexcept -> bool;

/// @ingroup http
/// Visit each baggage member in order as a key, a value, and the property set
/// trailing it, which is empty when the member carries none. The value is
/// handed over still encoded, since decoding it is a separate concern with a
/// rule of its own: "percent-encoded octet sequences that do not match the
/// UTF-8 encoding scheme MUST be replaced with the replacement code point
/// (U+FFFD)". The whole input is
/// checked before anything is visited, so a caller never acts on part of a
/// malformed value, which for baggage means never propagating a pair out of a
/// value it then discards.
///
/// A key may repeat and the order of members is significant, so a caller
/// collecting these wants a sequence rather than a map. Every view borrows the
/// input, so the value they came from must outlive them. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// std::string flat;
/// assert(sourcemeta::core::http_parse_baggage(
///     "userId=alice,serverNode=DF%2028",
///     [&flat](const auto key, const auto value, const auto properties) {
///       flat += std::string{key} + "=" + std::string{value} + "[" +
///               std::string{properties} + "];";
///     }));
/// assert(flat == "userId=alice[];serverNode=DF%2028[];");
/// ```
///
/// @see https://www.w3.org/TR/baggage/#definition
/// @see https://www.w3.org/TR/baggage/#value
template <typename Callback>
  requires std::invocable<Callback, std::string_view, std::string_view,
                          std::string_view>
inline auto http_parse_baggage(const std::string_view value, Callback callback)
    -> bool {
  if (!http_is_baggage(value)) {
    return false;
  }

  auto rest{value};
  while (true) {
    const auto comma{rest.find(',')};
    const auto member{trim(rest.substr(0, comma), http_is_ows)};
    const auto semicolon{member.find(';')};
    const auto pair{trim(member.substr(0, semicolon), http_is_ows)};
    const auto equal{pair.find('=')};

    // Every member holds an equals sign, which the check above has settled, so
    // the position below is never the one that stands for not found
    assert(equal != std::string_view::npos);
    callback(trim(pair.substr(0, equal), http_is_ows),
             trim(pair.substr(equal + 1), http_is_ows),
             semicolon == std::string_view::npos
                 ? member.substr(member.size())
                 : member.substr(semicolon + 1));
    if (comma == std::string_view::npos) {
      return true;
    }

    rest.remove_prefix(comma + 1);
  }
}

/// @ingroup http
/// Visit each entry of a baggage property set in order, as a key and a value
/// that is absent when the entry is a bare key. That distinction is why the
/// value is optional rather than empty: the grammar admits an entry with no
/// value at all and an entry whose value is the empty string, and they are not
/// the same thing.
///
/// The input is the property set handed over when visiting a member, which is
/// what follows the first semicolon and so does not include it. An empty set
/// is a set of no entries and parses. Note that the specification writes a set
/// with that leading semicolon still attached, where this does not take one.
///
/// The input is checked before anything is visited, and nothing is visited
/// when it does not parse. Every view borrows the input. For example:
///
/// ```cpp
/// #include <sourcemeta/core/http.h>
/// #include <cassert>
/// #include <string>
///
/// std::string flat;
/// assert(sourcemeta::core::http_parse_baggage_properties(
///     "property1;propertyKey=propertyValue",
///     [&flat](const auto key, const auto value) {
///       flat += std::string{key} +
///               (value.has_value() ? "=" + std::string{*value} : "") + ";";
///     }));
/// assert(flat == "property1;propertyKey=propertyValue;");
/// ```
///
/// @see https://www.w3.org/TR/baggage/#property
template <typename Callback>
  requires std::invocable<Callback, std::string_view,
                          std::optional<std::string_view>>
inline auto http_parse_baggage_properties(const std::string_view properties,
                                          Callback callback) -> bool {
  // The repetition that carries properties may run zero times, so a member
  // without any hands over nothing, and nothing is a set of no entries rather
  // than a malformed one
  // https://www.w3.org/TR/baggage/#definition
  if (properties.empty()) {
    return true;
  }

  auto pending{properties};
  while (true) {
    const auto semicolon{pending.find(';')};
    const auto entry{trim(pending.substr(0, semicolon), http_is_ows)};
    const auto equal{entry.find('=')};
    if (!http_is_token(trim(entry.substr(0, equal), http_is_ows)) ||
        (equal != std::string_view::npos &&
         !http_is_baggage_value(trim(entry.substr(equal + 1), http_is_ows)))) {
      return false;
    }

    if (semicolon == std::string_view::npos) {
      break;
    }

    pending.remove_prefix(semicolon + 1);
  }

  auto rest{properties};
  while (true) {
    const auto semicolon{rest.find(';')};
    const auto entry{trim(rest.substr(0, semicolon), http_is_ows)};
    const auto equal{entry.find('=')};
    callback(trim(entry.substr(0, equal), http_is_ows),
             equal == std::string_view::npos
                 ? std::optional<std::string_view>{}
                 : trim(entry.substr(equal + 1), http_is_ows));
    if (semicolon == std::string_view::npos) {
      return true;
    }

    rest.remove_prefix(semicolon + 1);
  }
}

} // namespace sourcemeta::core

#endif
