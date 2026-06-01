#ifndef SOURCEMETA_CORE_TEXT_H_
#define SOURCEMETA_CORE_TEXT_H_

#ifndef SOURCEMETA_CORE_TEXT_EXPORT
#include <sourcemeta/core/text_export.h>
#endif

#include <concepts>    // std::same_as
#include <cstddef>     // std::size_t
#include <filesystem>  // std::filesystem::path
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair

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
/// Return the ASCII lowercase form of a character. Non-ASCII code units pass
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
template <typename Character>
  requires std::same_as<Character, char> ||
           std::same_as<Character, signed char> ||
           std::same_as<Character, unsigned char> ||
           std::same_as<Character, wchar_t>
inline constexpr auto to_lowercase(const Character character) noexcept
    -> Character {
  return (character >= 'A' && character <= 'Z')
             ? static_cast<Character>(character + ('a' - 'A'))
             : character;
}

/// @ingroup text
///
/// Convert a string to ASCII lowercase in place. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// std::string value{"Hello WORLD"};
/// sourcemeta::core::to_lowercase(value);
/// assert(value == "hello world");
/// ```
template <typename Character, typename Traits, typename Allocator>
  requires requires(Character character) {
    { to_lowercase(character) } -> std::same_as<Character>;
  }
inline auto to_lowercase(std::basic_string<Character, Traits, Allocator> &value)
    -> void {
  for (auto &character : value) {
    character = to_lowercase(character);
  }
}

/// @ingroup text
///
/// Convert a filesystem path to ASCII lowercase in place. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <filesystem>
///
/// std::filesystem::path value{"/Foo/Bar.JSON"};
/// sourcemeta::core::to_lowercase(value);
/// assert(value == std::filesystem::path{"/foo/bar.json"});
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto to_lowercase(std::filesystem::path &value) -> void;

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
/// Return `input` with leading and trailing ASCII whitespace removed. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::trim("  hello  ") == "hello");
/// assert(sourcemeta::core::trim("\t\nfoo\r\n") == "foo");
/// assert(sourcemeta::core::trim("   ").empty());
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto trim(const std::string_view input) noexcept -> std::string_view;

/// @ingroup text
///
/// Return the prefix of `input` up to (but excluding) the first occurrence
/// of `marker`, or the full input when `marker` is absent. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::take_until("foo # bar", '#') == "foo ");
/// assert(sourcemeta::core::take_until("no marker", '#') == "no marker");
/// assert(sourcemeta::core::take_until("#leading", '#').empty());
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto take_until(const std::string_view input, const char marker) noexcept
    -> std::string_view;

/// @ingroup text
///
/// Split `input` at the first occurrence of `delimiter`, returning the
/// parts before and after it. Return `std::nullopt` when the delimiter is
/// absent. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// const auto parts{sourcemeta::core::split_once("key=value", '=')};
/// assert(parts.has_value());
/// assert(parts->first == "key");
/// assert(parts->second == "value");
/// assert(!sourcemeta::core::split_once("no separator", '=').has_value());
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto split_once(const std::string_view input, const char delimiter) noexcept
    -> std::optional<std::pair<std::string_view, std::string_view>>;

/// @ingroup text
///
/// Split `input` at the first occurrence of `delimiter`, returning the
/// parts before and after it. Return `std::nullopt` when the delimiter is
/// absent or empty. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// const auto parts{sourcemeta::core::split_once("1..5", "..")};
/// assert(parts.has_value());
/// assert(parts->first == "1");
/// assert(parts->second == "5");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto split_once(const std::string_view input,
                const std::string_view delimiter) noexcept
    -> std::optional<std::pair<std::string_view, std::string_view>>;

/// @ingroup text
///
/// Iterate the parts of `input` separated by `delimiter`, invoking
/// `callback` with each part. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <iostream>
///
/// sourcemeta::core::split("alpha;beta;gamma", ';',
///     [](const std::string_view part) {
///       std::cout << part << '\n';
///     });
/// ```
template <typename Callback>
auto split(const std::string_view input, const char delimiter,
           Callback callback) -> void {
  std::string_view rest{input};
  while (true) {
    const auto next{sourcemeta::core::split_once(rest, delimiter)};
    if (!next.has_value()) {
      callback(rest);
      return;
    }
    callback(next->first);
    rest = next->second;
  }
}

/// @ingroup text
///
/// Stream each item of `items` to `stream`, separated by `separator`. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <array>
/// #include <iostream>
///
/// constexpr std::array<int, 3> values{1, 2, 3};
/// sourcemeta::core::join_to(std::cout, values, ", ");
/// // prints: 1, 2, 3
/// ```
template <typename Range>
auto join_to(std::ostream &stream, const Range &items,
             const std::string_view separator) -> void {
  bool first{true};
  for (const auto &item : items) {
    if (!first) {
      stream << separator;
    }
    stream << item;
    first = false;
  }
}

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
