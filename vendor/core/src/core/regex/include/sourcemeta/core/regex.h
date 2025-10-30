#ifndef SOURCEMETA_CORE_REGEX_H_
#define SOURCEMETA_CORE_REGEX_H_

#ifndef SOURCEMETA_CORE_REGEX_EXPORT
#include <sourcemeta/core/regex_export.h>
#endif

#include <cstdint>  // std::uint8_t, std::uint64_t
#include <memory>   // std::shared_ptr
#include <optional> // std::optional
#include <string>   // std::string
#include <utility>  // std::pair
#include <variant>  // std::variant

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
using RegexTypePrefix = std::string;

/// @ingroup regex
struct RegexTypeNonEmpty {
  auto operator==(const RegexTypeNonEmpty &) const noexcept -> bool = default;
};

/// @ingroup regex
using RegexTypeRange = std::pair<std::uint64_t, std::uint64_t>;

/// @ingroup regex
struct RegexTypePCRE2 {
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
///
/// Compile a regular expression from a string. If the regular expression is
/// invalid, no value is returned. In this function
///
/// - Regexes are NOT automatically anchored
/// - Regexes assume `DOTALL`
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
///   sourcemeta::core::to_regex("^foo")};
/// assert(regex.has_value());
/// ```
SOURCEMETA_CORE_REGEX_EXPORT
auto to_regex(const std::string &pattern) -> std::optional<Regex>;

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
auto matches(const Regex &regex, const std::string &value) -> bool;

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
auto matches_if_valid(const std::string &pattern, const std::string &value)
    -> bool;

} // namespace sourcemeta::core

#endif
