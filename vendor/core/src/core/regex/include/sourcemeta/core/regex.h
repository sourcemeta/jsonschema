#ifndef SOURCEMETA_CORE_REGEX_H_
#define SOURCEMETA_CORE_REGEX_H_

#ifndef SOURCEMETA_CORE_REGEX_EXPORT
#include <sourcemeta/core/regex_export.h>
#endif

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <memory>      // std::shared_ptr
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <variant>     // std::variant

/// @defgroup regex Regex
/// @brief An opinionated and permissive ECMA 262 + RFC 9485 (best effort) regex
/// implementation for JSON Schema
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// ```

namespace sourcemeta::core {

/// @ingroup regex
/// Matches any string that begins with a fixed prefix.
using RegexTypePrefix = std::string;

/// @ingroup regex
struct RegexTypeNonEmpty {
  auto operator==(const RegexTypeNonEmpty &) const noexcept -> bool = default;
};

/// @ingroup regex
/// Matches any string whose length falls within an inclusive range.
using RegexTypeRange = std::pair<std::size_t, std::size_t>;

/// @ingroup regex
/// A regular expression compiled through the PCRE2 engine.
struct RegexTypePCRE2 {
  /// The opaque handle to the compiled expression.
  std::shared_ptr<void> code;
  auto operator==(const RegexTypePCRE2 &other) const noexcept -> bool {
    return this->code == other.code;
  }
};

/// @ingroup regex
struct RegexTypeNoop {
  auto operator==(const RegexTypeNoop &) const noexcept -> bool = default;
};

/// @ingroup regex
/// A compiled regular expression in one of its supported representations.
using Regex = std::variant<RegexTypePrefix, RegexTypeNonEmpty, RegexTypeRange,
                           RegexTypePCRE2, RegexTypeNoop>;
#if !defined(DOXYGEN)
// For fast internal dispatching. It must stay in sync with the variant above
enum class RegexIndex : std::uint8_t {
  Prefix = 0,
  NonEmpty,
  Range,
  PCRE2,
  Noop
};
#endif

/// @ingroup regex
/// The dialects that a regular expression pattern can be interpreted with.
enum class RegexDialect : std::uint8_t {
  /// A permissive superset of ECMA 262 with PCRE2 extensions
  Permissive,
  /// Strict RFC 9485 I-Regexp, where any pattern outside the grammar is
  /// rejected, matching considers the whole input, and an unescaped caret
  /// or dollar sign outside a character class is an ordinary character
  IRegexp,
  /// Like the strict RFC 9485 dialect, except that matching considers any
  /// substring of the input
  IRegexpSearch
};

/// @ingroup regex
///
/// Compile a regular expression from a string. If the regular expression is
/// invalid, no value is returned. In this function:
///
/// - Permissive regexes are NOT automatically anchored
/// - Permissive regexes assume `DOTALL`
/// - RFC 9485 regexes match the whole input, except in the search dialect,
///   which matches any substring
/// - Regexes assume Unicode
/// - Regexes are case sensitive
/// - No matching happens (only boolean validation)
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// #include <cassert>
///
/// const sourcemeta::core::Regex regex{
///   sourcemeta::core::to_regex("^foo",
///     sourcemeta::core::RegexDialect::Permissive)};
/// assert(regex.has_value());
/// ```
SOURCEMETA_CORE_REGEX_EXPORT
auto to_regex(const std::string_view pattern,
              const RegexDialect dialect = RegexDialect::Permissive)
    -> std::optional<Regex>;

/// @ingroup regex
///
/// Validate a string against a regular expression. For example:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// #include <cassert>
///
/// const sourcemeta::core::Regex regex{
///   sourcemeta::core::to_regex("^foo")};
/// assert(regex.has_value());
/// assert(sourcemeta::core::matches(regex.value(), "foo bar"));
/// ```
SOURCEMETA_CORE_REGEX_EXPORT
auto matches(const Regex &regex, const std::string_view value) -> bool;

/// @ingroup regex
///
/// Validate a string against a regular expression pattern if the pattern
/// represents a valid regular expression, compiling it along the way. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::matches_if_valid("^foo", "foo bar"));
/// ```
SOURCEMETA_CORE_REGEX_EXPORT
auto matches_if_valid(const std::string_view pattern,
                      const std::string_view value,
                      const RegexDialect dialect = RegexDialect::Permissive)
    -> bool;

/// @ingroup regex
///
/// Check whether the given string is a valid ECMA-262 regular expression. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_regex_ecma("([abc])+\\s+$"));
/// assert(!sourcemeta::core::is_regex_ecma("^(abc]"));
/// assert(!sourcemeta::core::is_regex_ecma("\\a"));
/// ```
SOURCEMETA_CORE_REGEX_EXPORT
auto is_regex_ecma(const std::string_view pattern) -> bool;

} // namespace sourcemeta::core

#endif
