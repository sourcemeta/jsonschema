#ifndef SOURCEMETA_CORE_HTML_ESCAPE_H_
#define SOURCEMETA_CORE_HTML_ESCAPE_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <string> // std::string

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

} // namespace sourcemeta::core

#endif
