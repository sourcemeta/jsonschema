#ifndef SOURCEMETA_CORE_HTML_ESCAPE_H_
#define SOURCEMETA_CORE_HTML_ESCAPE_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <sourcemeta/core/html_buffer.h>

#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup html
/// HTML character escaping implementation per HTML Living Standard.
/// See: https://html.spec.whatwg.org/multipage/parsing.html#escapingString
///
/// This function escapes the five HTML special characters in-place: the
/// ampersand, less-than sign, greater-than sign, double quote, and apostrophe
/// each become their corresponding HTML entity.
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/html.h>
/// #include <cassert>
///
/// std::string text{"1 < 2 & 3 > 0 'x' \"y\""};
/// sourcemeta::core::html_escape(text);
/// assert(text == "1 &lt; 2 &amp; 3 &gt; 0 &#39;x&#39; &quot;y&quot;");
/// ```
SOURCEMETA_CORE_HTML_EXPORT
auto html_escape(std::string &text) -> void;

/// @ingroup html
/// Append the HTML-escaped form of `input` directly to `output`,
/// without allocating a temporary string.
SOURCEMETA_CORE_HTML_EXPORT
auto html_escape_append(std::string &output, std::string_view input) -> void;

/// @ingroup html
/// Append the HTML-escaped form of `input` directly to a buffer.
SOURCEMETA_CORE_HTML_EXPORT
auto html_escape_append(HTMLBuffer &output, std::string_view input) -> void;

} // namespace sourcemeta::core

#endif
