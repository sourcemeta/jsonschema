#ifndef SOURCEMETA_CORE_MARKDOWN_H_
#define SOURCEMETA_CORE_MARKDOWN_H_

#ifndef SOURCEMETA_CORE_MARKDOWN_EXPORT
#include <sourcemeta/core/markdown_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/markdown_error.h>
// NOLINTEND(misc-include-cleaner)

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint32_t
#include <limits>      // std::numeric_limits
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup markdown Markdown
/// @brief A growing implementation of Markdown-related utilities based on
/// GitHub Flavored Markdown (GFM).
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/markdown.h>
/// ```

namespace sourcemeta::core {

/// @ingroup markdown
/// The most input bytes accepted when a caller names no bound of its own,
/// which is what the offsets the parser addresses its input through can reach,
/// so a caller may name a smaller bound but never a larger one.
inline constexpr std::size_t MARKDOWN_MAXIMUM_INPUT_BYTES{
    std::numeric_limits<std::uint32_t>::max()};

/// @ingroup markdown
/// Convert a Markdown string to an HTML fragment using GitHub Flavored
/// Markdown (GFM) with all standard extensions enabled (tables, autolinks,
/// strikethrough, tag filtering, and task lists). Raw HTML and dangerous
/// links are suppressed by default, so the result is safe to render from
/// untrusted input. Passing a false safe argument lets raw HTML and unsafe
/// links pass through unchanged, which must only be done for trusted input.
/// The specification lets link references and short table rows expand to an
/// output far larger than the input, so the conversion throws `MarkdownError`
/// once the rendered link references expand past the larger of 16 times the
/// input size and 1,048,576 bytes, or once a table inserts more than 524,288
/// empty cells. A bound smaller than the default refuses a larger input
/// outright, counted after a NUL or a malformed byte sequence has been
/// replaced, so a repaired input may exceed a bound the written one meets.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/markdown.h>
///
/// #include <cassert>
///
/// const auto result{sourcemeta::core::markdown_to_html("Hello **world**")};
/// assert(result == "<p>Hello <strong>world</strong></p>\n");
/// ```
SOURCEMETA_CORE_MARKDOWN_EXPORT
auto markdown_to_html(const std::string_view input, const bool safe = true,
                      std::size_t maximum_input_bytes =
                          MARKDOWN_MAXIMUM_INPUT_BYTES) -> std::string;

} // namespace sourcemeta::core

#endif
