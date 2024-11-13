#ifndef SOURCEMETA_JSONTOOLKIT_JSONPOINTER_GRAMMAR_H_
#define SOURCEMETA_JSONTOOLKIT_JSONPOINTER_GRAMMAR_H_

namespace sourcemeta::jsontoolkit::internal {
template <typename CharT> static constexpr CharT token_pointer_slash{'\u002F'};
template <typename CharT> static constexpr CharT token_pointer_tilde{'\u007E'};
template <typename CharT> static constexpr CharT token_pointer_zero{'\u0030'};
template <typename CharT> static constexpr CharT token_pointer_one{'\u0031'};
template <typename CharT> static constexpr CharT token_pointer_quote{'\u0022'};
template <typename CharT>
static constexpr CharT token_pointer_escape_unicode{'\u0075'};
template <typename CharT>
static constexpr CharT token_pointer_escape_backspace{'\u0062'};
template <typename CharT>
static constexpr CharT token_pointer_escape_form_feed{'\u0066'};
template <typename CharT>
static constexpr CharT token_pointer_escape_line_feed{'\u006E'};
template <typename CharT>
static constexpr CharT token_pointer_escape_carriage_return{'\u0072'};
template <typename CharT>
static constexpr CharT token_pointer_escape_tab{'\u0074'};
template <typename CharT>
static constexpr CharT token_pointer_reverse_solidus{'\u005C'};

template <typename CharT>
static constexpr CharT token_pointer_number_zero{'\u0030'};
template <typename CharT>
static constexpr CharT token_pointer_number_one{'\u0031'};
template <typename CharT>
static constexpr CharT token_pointer_number_two{'\u0032'};
template <typename CharT>
static constexpr CharT token_pointer_number_three{'\u0033'};
template <typename CharT>
static constexpr CharT token_pointer_number_four{'\u0034'};
template <typename CharT>
static constexpr CharT token_pointer_number_five{'\u0035'};
template <typename CharT>
static constexpr CharT token_pointer_number_six{'\u0036'};
template <typename CharT>
static constexpr CharT token_pointer_number_seven{'\u0037'};
template <typename CharT>
static constexpr CharT token_pointer_number_eight{'\u0038'};
template <typename CharT>
static constexpr CharT token_pointer_number_nine{'\u0039'};
} // namespace sourcemeta::jsontoolkit::internal

#endif
