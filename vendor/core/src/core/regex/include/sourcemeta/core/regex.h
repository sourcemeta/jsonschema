#ifndef SOURCEMETA_CORE_REGEX_H_
#define SOURCEMETA_CORE_REGEX_H_

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <boost/regex.hpp>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <cassert>  // assert
#include <cstdint>  // std::uint8_t, std::uint64_t
#include <optional> // std::optional
#include <regex>    // std::regex
#include <string>   // std::stoull
#include <utility>  // std::pair
#include <variant>  // std::variant

/// @defgroup regex Regex
/// @brief An opinionated regex ECMA 262 implementation for JSON Schema
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// ```

namespace sourcemeta::core {

/// @ingroup regex
template <typename T> using RegexTypeBoost = boost::basic_regex<T>;

/// @ingroup regex
template <typename T> using RegexTypePrefix = T;

/// @ingroup regex
struct RegexTypeNonEmpty {
  auto operator==(const RegexTypeNonEmpty &) const noexcept -> bool = default;
};

/// @ingroup regex
using RegexTypeRange = std::pair<std::uint64_t, std::uint64_t>;

/// @ingroup regex
template <typename T> using RegexTypeStd = std::basic_regex<T>;

/// @ingroup regex
struct RegexTypeNoop {
  auto operator==(const RegexTypeNoop &) const noexcept -> bool = default;
};

/// @ingroup regex
template <typename T>
using Regex =
    std::variant<RegexTypeBoost<typename T::value_type>, RegexTypePrefix<T>,
                 RegexTypeNonEmpty, RegexTypeRange,
                 RegexTypeStd<typename T::value_type>, RegexTypeNoop>;
#if !defined(DOXYGEN)
// For fast internal dispatching. It must stay in sync with the variant above
enum class RegexIndex : std::uint8_t {
  Boost = 0,
  Prefix,
  NonEmpty,
  Range,
  Std,
  Noop
};
#endif

/// @ingroup regex
///
/// Compile a regular expression from a string. If the regular expression is
/// invalid, no value is returned. For example:
///
/// ```cpp
/// #include <sourcemeta/core/regex.h>
/// #include <cassert>
///
/// const sourcemeta::core::Regex regex{
///   sourcemeta::core::to_regex("^foo")};
/// assert(regex.has_value());
/// ```
template <typename T>
auto to_regex(const T &pattern) -> std::optional<Regex<T>> {
  if (pattern == ".*" || pattern == "^.*$" || pattern == "^(.*)$" ||
      pattern == "(.*)" || pattern == "[\\s\\S]*" || pattern == "^[\\s\\S]*$") {
    return RegexTypeNoop{};

    // Note that the JSON Schema specification does not impose the use of any
    // regular expression flag. Given popular adoption, we assume `.` matches
    // new line characters (as in the `DOTALL`) option
  } else if (pattern == ".+" || pattern == "^.+$" || pattern == "^(.+)$" ||
             pattern == ".") {
    return RegexTypeNonEmpty{};
  }

  const std::regex PREFIX_REGEX{R"(^\^([a-zA-Z0-9-_/@]+)(\.\*)?)"};
  std::smatch matches_prefix;
  if (std::regex_match(pattern, matches_prefix, PREFIX_REGEX)) {
    return RegexTypePrefix<T>{matches_prefix[1].str()};
  }

  const std::regex RANGE_REGEX{R"(^\^\.\{(\d+),(\d+)\}\$$)"};
  std::smatch matches_range;
  if (std::regex_match(pattern, matches_range, RANGE_REGEX)) {
    const std::uint64_t minimum{std::stoull(matches_range[1].str())};
    const std::uint64_t maximum{std::stoull(matches_range[2].str())};
    assert(minimum <= maximum);
    return RegexTypeRange{minimum, maximum};
  }

  RegexTypeBoost<typename T::value_type> result{
      pattern,
      boost::regex::no_except |
          // See https://en.cppreference.com/w/cpp/regex/basic_regex/constants
          boost::regex::ECMAScript |

          // When performing matches, all marked sub-expressions (expr) are
          // treated as non-marking sub-expressions (?:expr)
          boost::regex::nosubs |

          // Make the `.` character class match new lines
          boost::regex::mod_s |

          // Instructs the regular expression engine to make matching faster,
          // with the potential cost of making construction slower
          boost::regex::optimize};

  // Returns zero if the expression contains a valid regular expression
  // See
  // https://www.boost.org/doc/libs/1_82_0/libs/regex/doc/html/boost_regex/ref/basic_regex.html
  if (result.status() == 0) {
    return result;
  }

  try {
    // Boost seems to sometimes be overly strict, so we still default to
    // the standard implementation
    return RegexTypeStd<typename T::value_type>{
        pattern,
        // See https://en.cppreference.com/w/cpp/regex/basic_regex/constants
        std::regex::ECMAScript |

            // When performing matches, all marked sub-expressions (expr) are
            // treated as non-marking sub-expressions (?:expr)
            std::regex::nosubs |

            // Instructs the regular expression engine to make matching
            // faster, with the potential cost of making construction slower
            std::regex::optimize};
  } catch (const std::regex_error &) {
    return std::nullopt;
  }
}

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
template <typename T>
auto matches(const Regex<T> &regex, const T &value) -> bool {
  switch (static_cast<RegexIndex>(regex.index())) {
    case RegexIndex::Boost:
      return boost::regex_search(
          value, *std::get_if<RegexTypeBoost<typename T::value_type>>(&regex));
    case RegexIndex::Prefix:
      return value.starts_with(*std::get_if<RegexTypePrefix<T>>(&regex));
    case RegexIndex::NonEmpty:
      return !value.empty();
    case RegexIndex::Range:
      return value.size() >= std::get_if<RegexTypeRange>(&regex)->first &&
             value.size() <= std::get_if<RegexTypeRange>(&regex)->second;
    case RegexIndex::Std:
      return std::regex_search(
          value, *std::get_if<RegexTypeStd<typename T::value_type>>(&regex));
    case RegexIndex::Noop:
      return true;
  }

    // See https://en.cppreference.com/w/cpp/utility/unreachable
#if defined(_MSC_VER) && !defined(__clang__)
  __assume(false);
#else
  __builtin_unreachable();
#endif
}

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
template <typename T>
auto matches_if_valid(const T &pattern, const T &value) -> bool {
  const auto regex{to_regex(pattern)};
  return regex.has_value() && matches(regex.value(), value);
}

} // namespace sourcemeta::core

#endif
