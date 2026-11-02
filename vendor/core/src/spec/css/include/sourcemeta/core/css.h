#ifndef SOURCEMETA_CORE_CSS_H_
#define SOURCEMETA_CORE_CSS_H_

#ifndef SOURCEMETA_CORE_CSS_EXPORT
#include <sourcemeta/core/css_export.h>
#endif

#include <string_view> // std::string_view

/// @defgroup css CSS
/// @brief A growing implementation of CSS-related utilities.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/css.h>
/// ```

namespace sourcemeta::core {

/// @ingroup css
/// Check whether the given string is a valid CSS 2.1 hex color per §4.3.6.
/// Accepts only the two hex notations defined by CSS 2.1:
///
/// ```
/// "#" 3HEXDIG    ; e.g. "#C89"
/// "#" 6HEXDIG    ; e.g. "#CC8899"
/// ```
///
/// Hex digits are case-insensitive. The 4-digit and 8-digit alpha forms
/// from CSS Color Module Level 4 are not accepted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/css.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_css2_hex_color("#CC8899"));
/// assert(sourcemeta::core::is_css2_hex_color("#C89"));
/// assert(!sourcemeta::core::is_css2_hex_color("#00332520"));
/// assert(!sourcemeta::core::is_css2_hex_color("CC8899"));
/// ```
SOURCEMETA_CORE_CSS_EXPORT
auto is_css2_hex_color(const std::string_view value) noexcept -> bool;

/// @ingroup css
/// Check whether the given string is one of the 17 CSS 2.1 color keywords
/// defined in §4.3.6:
///
/// ```
/// aqua, black, blue, fuchsia, gray, green, lime, maroon, navy,
/// olive, orange, purple, red, silver, teal, white, yellow
/// ```
///
/// Matching is case-insensitive. The `transparent` keyword and the
/// deprecated system colors (`ButtonFace`, `ActiveBorder`, etc.) are out
/// of scope, as are the extended X11 names introduced by CSS Color Module
/// Level 3. For example:
///
/// ```cpp
/// #include <sourcemeta/core/css.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_css2_color_keyword("fuchsia"));
/// assert(sourcemeta::core::is_css2_color_keyword("RED"));
/// assert(!sourcemeta::core::is_css2_color_keyword("puce"));
/// assert(!sourcemeta::core::is_css2_color_keyword("papayawhip"));
/// ```
SOURCEMETA_CORE_CSS_EXPORT
auto is_css2_color_keyword(const std::string_view value) noexcept -> bool;

/// @ingroup css
/// Check whether the given string is a valid CSS 2.1 functional RGB color
/// per §4.3.6. Accepts both the integer and percentage forms:
///
/// ```
/// rgb( <integer> , <integer> , <integer> )
/// rgb( <percentage> , <percentage> , <percentage> )
/// ```
///
/// All three values must be the same type. The function name `rgb` is
/// case-insensitive, and CSS whitespace (space, tab, CR, LF, FF) is
/// permitted between tokens. Out-of-range values are accepted (CSS 2.1
/// clamps at use time). The 4-argument `rgba(...)` form from CSS Color
/// Module Level 3 is not accepted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/css.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_css2_rgb_function("rgb(255, 0, 128)"));
/// assert(sourcemeta::core::is_css2_rgb_function("rgb(100%, 0%, 50%)"));
/// assert(sourcemeta::core::is_css2_rgb_function("rgb(300, -5, 128)"));
/// assert(!sourcemeta::core::is_css2_rgb_function("rgb(100%, 0, 50%)"));
/// assert(!sourcemeta::core::is_css2_rgb_function("rgba(0, 0, 0, 1)"));
/// ```
SOURCEMETA_CORE_CSS_EXPORT
auto is_css2_rgb_function(const std::string_view value) noexcept -> bool;

/// @ingroup css
/// Check whether the given string is a valid CSS 2.1 `<color>` value per
/// §4.3.6. The accept set is the union of:
///
/// - `is_css2_hex_color`
/// - `is_css2_color_keyword`
/// - `is_css2_rgb_function`
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/css.h>
///
/// #include <cassert>
///
/// assert(sourcemeta::core::is_css2_color("fuchsia"));
/// assert(sourcemeta::core::is_css2_color("#CC8899"));
/// assert(sourcemeta::core::is_css2_color("rgb(255, 0, 0)"));
/// assert(!sourcemeta::core::is_css2_color("puce"));
/// assert(!sourcemeta::core::is_css2_color("#00332520"));
/// ```
SOURCEMETA_CORE_CSS_EXPORT
auto is_css2_color(const std::string_view value) noexcept -> bool;

} // namespace sourcemeta::core

#endif
