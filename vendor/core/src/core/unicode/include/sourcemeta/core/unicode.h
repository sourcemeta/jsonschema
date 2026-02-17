#ifndef SOURCEMETA_CORE_UNICODE_H_
#define SOURCEMETA_CORE_UNICODE_H_

#ifndef SOURCEMETA_CORE_UNICODE_EXPORT
#include <sourcemeta/core/unicode_export.h>
#endif

#include <istream>     // std::istream
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <string>      // std::string, std::u32string
#include <string_view> // std::string_view

/// @defgroup unicode Unicode
/// @brief Unicode encoding utilities.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/unicode.h>
/// ```

namespace sourcemeta::core {

/// @ingroup unicode
/// Encode a single Unicode codepoint as a UTF-8 string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/unicode.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::codepoint_to_utf8(0x41) == "A");
/// ```
SOURCEMETA_CORE_UNICODE_EXPORT
auto codepoint_to_utf8(const char32_t codepoint) -> std::string;

/// @ingroup unicode
/// Encode a single Unicode codepoint as UTF-8 into an output stream.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/unicode.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::ostringstream output;
/// sourcemeta::core::codepoint_to_utf8(0x41, output);
/// assert(output.str() == "A");
/// ```
SOURCEMETA_CORE_UNICODE_EXPORT
auto codepoint_to_utf8(const char32_t codepoint, std::ostream &output) -> void;

/// @ingroup unicode
/// Encode a single Unicode codepoint as UTF-8, appending to an existing string.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/unicode.h>
/// #include <cassert>
///
/// std::string output;
/// sourcemeta::core::codepoint_to_utf8(0x41, output);
/// assert(output == "A");
/// ```
SOURCEMETA_CORE_UNICODE_EXPORT
auto codepoint_to_utf8(const char32_t codepoint, std::string &output) -> void;

/// @ingroup unicode
/// Decode a UTF-8 byte stream into a sequence of Unicode codepoints (UTF-32).
/// Returns std::nullopt if the input contains invalid UTF-8. For example:
///
/// ```cpp
/// #include <sourcemeta/core/unicode.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::istringstream input{"A"};
/// const auto result{sourcemeta::core::utf8_to_utf32(input)};
/// assert(result.has_value());
/// assert(result.value() == std::u32string{0x41});
/// ```
SOURCEMETA_CORE_UNICODE_EXPORT
auto utf8_to_utf32(std::istream &input) -> std::optional<std::u32string>;

/// @ingroup unicode
/// Decode a UTF-8 string into a sequence of Unicode codepoints (UTF-32).
/// Returns std::nullopt if the input contains invalid UTF-8. For example:
///
/// ```cpp
/// #include <sourcemeta/core/unicode.h>
/// #include <cassert>
///
/// const auto result{sourcemeta::core::utf8_to_utf32("A")};
/// assert(result.has_value());
/// assert(result.value() == std::u32string{0x41});
/// ```
SOURCEMETA_CORE_UNICODE_EXPORT
auto utf8_to_utf32(const std::string_view input)
    -> std::optional<std::u32string>;

} // namespace sourcemeta::core

#endif
