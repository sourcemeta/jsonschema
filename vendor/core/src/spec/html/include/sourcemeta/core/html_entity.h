#ifndef SOURCEMETA_CORE_HTML_ENTITY_H_
#define SOURCEMETA_CORE_HTML_ENTITY_H_

#ifndef SOURCEMETA_CORE_HTML_EXPORT
#include <sourcemeta/core/html_export.h>
#endif

#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup html
/// Look up the characters of a named character reference per the HTML Living
/// Standard, which lists every one of them at
/// https://html.spec.whatwg.org/entities.json. The name excludes the leading
/// ampersand and includes the trailing semicolon, which the standard lets a
/// few legacy names omit. The result is the UTF-8 encoding of the characters,
/// or an empty view if the name is not a named character reference.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/html.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::html_entity("copy;") == "\xC2\xA9");
/// assert(sourcemeta::core::html_entity("copy") == "\xC2\xA9");
/// assert(sourcemeta::core::html_entity("ne").empty());
/// ```
SOURCEMETA_CORE_HTML_EXPORT
auto html_entity(const std::string_view name) noexcept -> std::string_view;

} // namespace sourcemeta::core

#endif
