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
/// This function escapes the five HTML special characters in-place:
/// - `&` becomes `&amp;`
/// - `<` becomes `&lt;`
/// - `>` becomes `&gt;`
/// - `"` becomes `&quot;`
/// - `'` becomes `&#39;`
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/html.h>
/// #include <cassert>
///
/// std::string text{"<script>alert('xss')</script>"};
/// sourcemeta::core::html_escape(text);
/// assert(text == "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;");
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
