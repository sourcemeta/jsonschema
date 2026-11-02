#ifndef SOURCEMETA_CORE_JSONPOINTER_GRAMMAR_H_
#define SOURCEMETA_CORE_JSONPOINTER_GRAMMAR_H_

namespace sourcemeta::core::internal {
template <typename CharT> static constexpr CharT TOKEN_POINTER_SLASH{'\u002F'};
template <typename CharT> static constexpr CharT TOKEN_POINTER_TILDE{'\u007E'};
template <typename CharT> static constexpr CharT TOKEN_POINTER_ZERO{'\u0030'};
template <typename CharT> static constexpr CharT TOKEN_POINTER_ONE{'\u0031'};
template <typename CharT> static constexpr CharT TOKEN_POINTER_QUOTE{'\u0022'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_ESCAPE_UNICODE{'\u0075'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_ESCAPE_BACKSPACE{'\u0062'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_ESCAPE_FORM_FEED{'\u0066'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_ESCAPE_LINE_FEED{'\u006E'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_ESCAPE_CARRIAGE_RETURN{'\u0072'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_ESCAPE_TAB{'\u0074'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_REVERSE_SOLIDUS{'\u005C'};

template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_ZERO{'\u0030'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_ONE{'\u0031'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_TWO{'\u0032'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_THREE{'\u0033'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_FOUR{'\u0034'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_FIVE{'\u0035'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_SIX{'\u0036'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_SEVEN{'\u0037'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_EIGHT{'\u0038'};
template <typename CharT>
static constexpr CharT TOKEN_POINTER_NUMBER_NINE{'\u0039'};
} // namespace sourcemeta::core::internal

#endif
