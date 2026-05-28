#ifndef SOURCEMETA_CORE_TEXT_H_
#define SOURCEMETA_CORE_TEXT_H_

#ifndef SOURCEMETA_CORE_TEXT_EXPORT
#include <sourcemeta/core/text_export.h>
#endif

#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view

/// @defgroup text Text
/// @brief A collection of general-purpose text manipulation utilities
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// ```

namespace sourcemeta::core {

/// @ingroup text
///
/// Convert a string to Title Case in place. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// std::string value{"hello_world"};
/// sourcemeta::core::to_title_case(value);
/// assert(value == "Hello World");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto to_title_case(std::string &value) -> void;

/// @ingroup text
///
/// Return the ASCII lowercase form of a character. Non-ASCII bytes pass
/// through unchanged. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::to_lowercase('A') == 'a');
/// assert(sourcemeta::core::to_lowercase('a') == 'a');
/// assert(sourcemeta::core::to_lowercase('5') == '5');
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto to_lowercase(const char character) noexcept -> char;

/// @ingroup text
///
/// Truncate a string in place to at most `maximum_length` bytes, appending
/// `marker` on truncation. Rewinds to a UTF-8 code-point boundary so
/// multi-byte characters are never split. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// std::string value{"hello"};
/// sourcemeta::core::truncate(value, 1, "...");
/// assert(value == "h...");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto truncate(std::string &input, const std::size_t maximum_length,
              const std::string_view marker) -> void;

/// @ingroup text
///
/// Return `input` with `suffix` removed from the end under ASCII
/// case-insensitive comparison, or `input` unchanged when the suffix does
/// not match. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string_view>
///
/// const auto trimmed{sourcemeta::core::remove_suffix_ignore_case(
///     "schema.JSON", ".json")};
/// assert(trimmed == "schema");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto remove_suffix_ignore_case(const std::string_view input,
                               const std::string_view suffix) noexcept
    -> std::string_view;

} // namespace sourcemeta::core

#endif
