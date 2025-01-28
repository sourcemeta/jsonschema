#ifndef SOURCEMETA_CORE_JSON_GRAMMAR_H_
#define SOURCEMETA_CORE_JSON_GRAMMAR_H_

#include <string_view> // std::basic_string_view

namespace sourcemeta::core::internal {

// The six structural tokens:
// [ U+005B  left square bracket
// { U+007B  left curly bracket
// ] U+005D  right square bracket
// } U+007D  right curly bracket
// : U+003A  colon
// , U+002C  comma
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf

// A string is a sequence of Unicode code points wrapped with quotation marks
// (U+0022). All code points may be placed within the quotation marks except
// for the code points that must be escaped: quotation mark (U+0022), reverse
// solidus (U+005C), and the control characters U+0000 to U+001F.
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
template <typename CharT> static constexpr CharT token_string_quote{'\u0022'};
template <typename CharT> static constexpr CharT token_string_escape{'\u005C'};
template <typename CharT> static constexpr CharT token_string_solidus{'\u002F'};

// There are two-character escape sequence representations of some characters.
//
// \" represents the quotation mark character (U+0022).
// \\ represents the reverse solidus character (U+005C).
// \/ represents the solidus character (U+002F).
// \b represents the backspace character (U+0008).
// \f represents the form feed character (U+000C).
// \n represents the line feed character (U+000A).
// \r represents the carriage return character (U+000D).
// \t represents the character tabulation character (U+0009).
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
template <typename CharT>
static constexpr CharT token_string_escape_backspace{'\u0062'};
template <typename CharT>
static constexpr CharT token_string_escape_form_feed{'\u0066'};
template <typename CharT>
static constexpr CharT token_string_escape_line_feed{'\u006E'};
template <typename CharT>
static constexpr CharT token_string_escape_carriage_return{'\u0072'};
template <typename CharT>
static constexpr CharT token_string_escape_tabulation{'\u0074'};
template <typename CharT>
static constexpr CharT token_string_escape_unicode{'\u0075'};

// Array
template <typename CharT> static constexpr CharT token_array_begin{'\u005B'};
template <typename CharT> static constexpr CharT token_array_end{'\u005D'};
template <typename CharT>
static constexpr CharT token_array_delimiter{'\u002C'};

// Object
template <typename CharT> static constexpr CharT token_object_begin{'\u007B'};
template <typename CharT> static constexpr CharT token_object_end{'\u007D'};
template <typename CharT>
static constexpr CharT token_object_key_delimiter{'\u003A'};
template <typename CharT>
static constexpr CharT token_object_delimiter{'\u002C'};

// These are the three literal name tokens:
// true  U+0074 U+0072 U+0075 U+0065
// false U+0066 U+0061 U+006C U+0073 U+0065
// null  U+006E U+0075 U+006C U+006C
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf

// Boolean
template <typename CharT, typename Traits>
static constexpr std::basic_string_view<CharT, Traits> constant_true{
    "\u0074\u0072\u0075\u0065"};
template <typename CharT, typename Traits>
static constexpr std::basic_string_view<CharT, Traits> constant_false{
    "\u0066\u0061\u006C\u0073\u0065"};

// Null
template <typename CharT, typename Traits>
static constexpr std::basic_string_view<CharT, Traits> constant_null{
    "\u006E\u0075\u006C\u006C"};

// A number is a sequence of decimal digits with no superfluous leading zero.
// It may have a preceding minus sign (U+002D). It may have a fractional part
// prefixed by a decimal point (U+002E). It may have an exponent, prefixed by e
// (U+0065) or E (U+0045) and optionally + (U+002B) or – (U+002D). The digits
// are the code points U+0030 through U+0039.
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
template <typename CharT>
static constexpr CharT token_number_decimal_point{'\u002E'};
template <typename CharT>
static constexpr CharT token_number_exponent_uppercase{'\u0045'};
template <typename CharT>
static constexpr CharT token_number_exponent_lowercase{'\u0065'};
template <typename CharT> static constexpr CharT token_number_plus{'\u002B'};
template <typename CharT> static constexpr CharT token_number_minus{'\u002D'};
template <typename CharT> static constexpr CharT token_number_zero{'\u0030'};
template <typename CharT> static constexpr CharT token_number_one{'\u0031'};
template <typename CharT> static constexpr CharT token_number_two{'\u0032'};
template <typename CharT> static constexpr CharT token_number_three{'\u0033'};
template <typename CharT> static constexpr CharT token_number_four{'\u0034'};
template <typename CharT> static constexpr CharT token_number_five{'\u0035'};
template <typename CharT> static constexpr CharT token_number_six{'\u0036'};
template <typename CharT> static constexpr CharT token_number_seven{'\u0037'};
template <typename CharT> static constexpr CharT token_number_eight{'\u0038'};
template <typename CharT> static constexpr CharT token_number_nine{'\u0039'};

// Whitespace is any sequence of one or more of the following code points:
// character tabulation (U+0009), line feed (U+000A), carriage return (U+000D),
// and space (U+0020).
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
template <typename CharT>
static constexpr CharT token_whitespace_tabulation{'\u0009'};
template <typename CharT>
static constexpr CharT token_whitespace_line_feed{'\u000A'};
template <typename CharT>
static constexpr CharT token_whitespace_carriage_return{'\u000D'};
template <typename CharT>
static constexpr CharT token_whitespace_space{'\u0020'};

} // namespace sourcemeta::core::internal

#endif
