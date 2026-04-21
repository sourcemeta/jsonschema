#ifndef SOURCEMETA_CORE_MARKDOWN_H_
#define SOURCEMETA_CORE_MARKDOWN_H_

#ifndef SOURCEMETA_CORE_MARKDOWN_EXPORT
#include <sourcemeta/core/markdown_export.h>
#endif

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
/// Convert a Markdown string to an HTML fragment using GitHub Flavored
/// Markdown (GFM) with all standard extensions enabled (tables, autolinks,
/// strikethrough, tag filtering, and task lists). For example:
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
auto markdown_to_html(const std::string_view input) -> std::string;

} // namespace sourcemeta::core

#endif
