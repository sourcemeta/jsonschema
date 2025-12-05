#ifndef SOURCEMETA_CORE_PUNYCODE_H_
#define SOURCEMETA_CORE_PUNYCODE_H_

#ifndef SOURCEMETA_CORE_PUNYCODE_EXPORT
#include <sourcemeta/core/punycode_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/punycode_error.h>
// NOLINTEND(misc-include-cleaner)

#include <istream>     // std::istream
#include <ostream>     // std::ostream
#include <string>      // std::string, std::u32string
#include <string_view> // std::string_view, std::u32string_view

/// @defgroup punycode Punycode
/// @brief An implementation of RFC 3492 Punycode.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// ```

namespace sourcemeta::core {

// See https://www.rfc-editor.org/rfc/rfc3492

/// @ingroup punycode
/// Encode Unicode code points (UTF-32) to Punycode. For example:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// #include <cassert>
///
/// const std::u32string input{0x0048, 0x0065, 0x006C, 0x006C, 0x006F,
///                            0x002D, 0x305D, 0x308C, 0x305E, 0x308C,
///                            0x306E, 0x5834, 0x6240};
/// assert(sourcemeta::core::utf32_to_punycode(input) ==
///        "Hello--fc4qua05auwb3674vfr0b");
/// ```
///
/// Note that stream-based overloads for UTF-32 are not provided
/// because the C++ standard library does not define the required locale facets
/// (`std::ctype<char32_t>`) for `std::basic_istream<char32_t>` and
/// `std::basic_ostream<char32_t>` to function properly.
SOURCEMETA_CORE_PUNYCODE_EXPORT
auto utf32_to_punycode(std::u32string_view input) -> std::string;

/// @ingroup punycode
/// Encode UTF-8 to Punycode using streams. For example:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::istringstream input{"M\xC3\xBCnchen"};
/// std::ostringstream output;
/// sourcemeta::core::utf8_to_punycode(input, output);
/// assert(output.str() == "Mnchen-3ya");
/// ```
SOURCEMETA_CORE_PUNYCODE_EXPORT
auto utf8_to_punycode(std::istream &input, std::ostream &output) -> void;

/// @ingroup punycode
/// Encode UTF-8 to Punycode. For example:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::utf8_to_punycode("M\xC3\xBCnchen") ==
/// "Mnchen-3ya");
/// ```
SOURCEMETA_CORE_PUNYCODE_EXPORT
auto utf8_to_punycode(std::string_view input) -> std::string;

/// @ingroup punycode
/// Decode Punycode to Unicode code points (UTF-32). For example:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// #include <cassert>
///
/// const std::u32string expected{0x0048, 0x0065, 0x006C, 0x006C, 0x006F,
///                               0x002D, 0x305D, 0x308C, 0x305E, 0x308C,
///                               0x306E, 0x5834, 0x6240};
/// assert(sourcemeta::core::punycode_to_utf32("Hello--fc4qua05auwb3674vfr0b")
/// ==
///        expected);
/// ```
///
/// Note that stream-based overloads for UTF-32 are not provided
/// because the C++ standard library does not define the required locale facets
/// (`std::ctype<char32_t>`) for `std::basic_istream<char32_t>` and
/// `std::basic_ostream<char32_t>` to function properly.
SOURCEMETA_CORE_PUNYCODE_EXPORT
auto punycode_to_utf32(std::string_view input) -> std::u32string;

/// @ingroup punycode
/// Decode Punycode to UTF-8 using streams. For example:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// #include <sstream>
/// #include <cassert>
///
/// std::istringstream input{"Mnchen-3ya"};
/// std::ostringstream output;
/// sourcemeta::core::punycode_to_utf8(input, output);
/// assert(output.str() == "M\xC3\xBCnchen");
/// ```
SOURCEMETA_CORE_PUNYCODE_EXPORT
auto punycode_to_utf8(std::istream &input, std::ostream &output) -> void;

/// @ingroup punycode
/// Decode Punycode to UTF-8. For example:
///
/// ```cpp
/// #include <sourcemeta/core/punycode.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::punycode_to_utf8("Mnchen-3ya") ==
/// "M\xC3\xBCnchen");
/// ```
SOURCEMETA_CORE_PUNYCODE_EXPORT
auto punycode_to_utf8(std::string_view input) -> std::string;

} // namespace sourcemeta::core

#endif
