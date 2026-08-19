#ifndef SOURCEMETA_CORE_JSONL_GRAMMAR_H_
#define SOURCEMETA_CORE_JSONL_GRAMMAR_H_

namespace sourcemeta::core::internal {
template <typename CharT>
static constexpr CharT TOKEN_JSONL_LINE_FEED{'\u000A'};

// Whitespace is any sequence of one or more of the following code points:
// character tabulation (U+0009), line feed (U+000A), carriage return (U+000D),
// and space (U+0020).
// See
// https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
template <typename CharT>
static constexpr CharT TOKEN_JSONL_WHITESPACE_TABULATION{'\u0009'};
template <typename CharT>
static constexpr CharT TOKEN_JSONL_WHITESPACE_CARRIAGE_RETURN{'\u000D'};
template <typename CharT>
static constexpr CharT TOKEN_JSONL_WHITESPACE_SPACE{'\u0020'};

} // namespace sourcemeta::core::internal

#endif
