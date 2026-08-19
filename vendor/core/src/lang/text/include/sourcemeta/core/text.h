#ifndef SOURCEMETA_CORE_TEXT_H_
#define SOURCEMETA_CORE_TEXT_H_

#ifndef SOURCEMETA_CORE_TEXT_EXPORT
#include <sourcemeta/core/text_export.h>
#endif

#include <algorithm>   // std::max
#include <array>       // std::array
#include <charconv>    // std::to_chars
#include <concepts>    // std::same_as, std::integral
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int8_t, std::uint64_t
#include <filesystem>  // std::filesystem::path
#include <ios>         // std::streamsize
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional
#include <ostream>     // std::ostream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::remove_cv_t
#include <utility>     // std::pair
#include <vector>      // std::vector

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
constexpr auto to_lowercase(const Character character) noexcept -> Character {
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
/// Return whether a character is not ASCII uppercase. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_lowercase('a'));
/// assert(!sourcemeta::core::is_lowercase('A'));
/// assert(sourcemeta::core::is_lowercase('5'));
/// ```
template <typename Character>
  requires std::same_as<Character, char> ||
           std::same_as<Character, signed char> ||
           std::same_as<Character, unsigned char> ||
           std::same_as<Character, wchar_t>
constexpr auto is_lowercase(const Character character) noexcept -> bool {
  return character < 'A' || character > 'Z';
}

/// @ingroup text
///
/// Return whether every code unit of a string is not ASCII uppercase. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// assert(sourcemeta::core::is_lowercase(std::string{"hello"}));
/// assert(!sourcemeta::core::is_lowercase(std::string{"Hello"}));
/// ```
template <typename String>
  requires requires(const String &value) {
    { is_lowercase(*value.begin()) } -> std::same_as<bool>;
  }
inline auto is_lowercase(const String &value) noexcept -> bool {
  for (const auto character : value) {
    if (!is_lowercase(character)) {
      return false;
    }
  }
  return true;
}

/// @ingroup text
///
/// Return whether every code unit of a filesystem path is not ASCII
/// uppercase. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <filesystem>
///
/// assert(sourcemeta::core::is_lowercase(std::filesystem::path{"/foo/bar"}));
/// assert(!sourcemeta::core::is_lowercase(std::filesystem::path{"/Foo/Bar"}));
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto is_lowercase(const std::filesystem::path &value) noexcept -> bool;

/// @ingroup text
///
/// Return whether a character is an ASCII letter (A-Z or a-z). For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_alpha('a'));
/// assert(sourcemeta::core::is_alpha('Z'));
/// assert(!sourcemeta::core::is_alpha('5'));
/// ```
template <typename Character>
  requires std::same_as<Character, char> ||
           std::same_as<Character, signed char> ||
           std::same_as<Character, unsigned char> ||
           std::same_as<Character, wchar_t>
constexpr auto is_alpha(const Character character) noexcept -> bool {
  return (character >= 'a' && character <= 'z') ||
         (character >= 'A' && character <= 'Z');
}

/// @ingroup text
///
/// Return whether a string is non-empty and consists entirely of ASCII letters
/// (A-Z or a-z). An empty string is not considered a match. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_alpha("abc"));
/// assert(!sourcemeta::core::is_alpha("ab1"));
/// assert(!sourcemeta::core::is_alpha(""));
/// ```
constexpr auto is_alpha(const std::string_view value) noexcept -> bool {
  if (value.empty()) {
    return false;
  }
  for (const auto character : value) {
    if (!is_alpha(character)) {
      return false;
    }
  }
  return true;
}

/// @ingroup text
///
/// Return whether a character is an ASCII digit (0-9). For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_digit('5'));
/// assert(!sourcemeta::core::is_digit('a'));
/// ```
template <typename Character>
  requires std::same_as<Character, char> ||
           std::same_as<Character, signed char> ||
           std::same_as<Character, unsigned char> ||
           std::same_as<Character, wchar_t>
constexpr auto is_digit(const Character character) noexcept -> bool {
  return character >= '0' && character <= '9';
}

/// @ingroup text
///
/// Return whether a string is non-empty and consists entirely of ASCII digits
/// (0-9). An empty string is not considered a match. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_digit("123"));
/// assert(!sourcemeta::core::is_digit("12a"));
/// assert(!sourcemeta::core::is_digit(""));
/// ```
constexpr auto is_digit(const std::string_view value) noexcept -> bool {
  if (value.empty()) {
    return false;
  }
  for (const auto character : value) {
    if (!is_digit(character)) {
      return false;
    }
  }
  return true;
}

/// @ingroup text
///
/// Return whether a character is an ASCII letter or digit. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_alphanum('a'));
/// assert(sourcemeta::core::is_alphanum('5'));
/// assert(!sourcemeta::core::is_alphanum('-'));
/// ```
template <typename Character>
  requires std::same_as<Character, char> ||
           std::same_as<Character, signed char> ||
           std::same_as<Character, unsigned char> ||
           std::same_as<Character, wchar_t>
constexpr auto is_alphanum(const Character character) noexcept -> bool {
  return is_alpha(character) || is_digit(character);
}

/// @ingroup text
///
/// Return whether a string is non-empty and consists entirely of ASCII letters
/// or digits. An empty string is not considered a match. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_alphanum("abc123"));
/// assert(!sourcemeta::core::is_alphanum("abc-123"));
/// assert(!sourcemeta::core::is_alphanum(""));
/// ```
constexpr auto is_alphanum(const std::string_view value) noexcept -> bool {
  if (value.empty()) {
    return false;
  }
  for (const auto character : value) {
    if (!is_alphanum(character)) {
      return false;
    }
  }
  return true;
}

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
/// Return `input` with every occurrence of `target` replaced by `replacement`.
/// An empty target matches nothing. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::replace("a.b.c", ".", "/") == "a/b/c");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto replace(const std::string_view input, const std::string_view target,
             const std::string_view replacement) -> std::string;

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
/// Return `input` with leading occurrences of `character` removed. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::strip_left("000123", '0') == "123");
/// assert(sourcemeta::core::strip_left("abc", '0') == "abc");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto strip_left(const std::string_view input, const char character) noexcept
    -> std::string_view;

/// @ingroup text
///
/// Return `input` with trailing occurrences of `character` removed. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::strip_right("hello\r\r", '\r') == "hello");
/// assert(sourcemeta::core::strip_right("abc", '\r') == "abc");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto strip_right(const std::string_view input, const char character) noexcept
    -> std::string_view;

/// @ingroup text
///
/// Return the content of `input` with a single matched pair of surrounding
/// `quote` characters removed, or `input` unchanged when it is not wrapped in
/// that pair. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::unquote("\"abc\"", '"') == "abc");
/// assert(sourcemeta::core::unquote("abc", '"') == "abc");
/// assert(sourcemeta::core::unquote("\"", '"') == "\"");
/// ```
constexpr auto unquote(const std::string_view input, const char quote) noexcept
    -> std::string_view {
  if (input.size() < 2 || input.front() != quote || input.back() != quote) {
    return input;
  }

  auto inner{input};
  inner.remove_prefix(1);
  inner.remove_suffix(1);
  return inner;
}

/// @ingroup text
///
/// Return `input` left-padded with `character` to at least `width` bytes, or
/// a copy of `input` when it is already that long. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::pad_left("42", 5, '0') == "00042");
/// assert(sourcemeta::core::pad_left("hello", 3, '0') == "hello");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto pad_left(const std::string_view input, const std::size_t width,
              const char character) -> std::string;

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
/// Split `input` at the last occurrence of `delimiter`, returning the
/// parts before and after it. Return `std::nullopt` when the delimiter is
/// absent. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// const auto parts{sourcemeta::core::rsplit_once("a.b.c", '.')};
/// assert(parts.has_value());
/// assert(parts->first == "a.b");
/// assert(parts->second == "c");
/// assert(!sourcemeta::core::rsplit_once("no separator", '.').has_value());
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto rsplit_once(const std::string_view input, const char delimiter) noexcept
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
/// Return the parts of `input` separated by `delimiter` as a vector, preserving
/// empty parts. The parts are views into `input`, which must outlive them. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// const auto parts{sourcemeta::core::split("alpha;beta;gamma", ';')};
/// assert(parts.size() == 3);
/// assert(parts.at(0) == "alpha");
/// assert(parts.at(2) == "gamma");
/// ```
inline auto split(const std::string_view input, const char delimiter)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> parts;
  sourcemeta::core::split(
      input, delimiter,
      [&parts](const std::string_view part) -> void { parts.push_back(part); });
  return parts;
}

/// @ingroup text
///
/// Return the items of `items` as a string, separated by `separator`. The items
/// must be string-like, as nothing is formatted along the way. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <array>
/// #include <cassert>
/// #include <string_view>
///
/// constexpr std::array<std::string_view, 3> values{{"a", "b", "c"}};
/// assert(sourcemeta::core::join(values, ", ") == "a, b, c");
/// ```
template <typename Range>
auto join(const Range &items, const std::string_view separator) -> std::string {
  std::string result;
  bool first{true};
  for (const auto &item : items) {
    if (!first) {
      result.append(separator);
    }

    result.append(item);
    first = false;
  }

  return result;
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
/// constexpr std::array<int, 3> values{{1, 2, 3}};
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
/// The integer types that can be spelled out in decimal. Booleans are excluded
/// because they have a textual form of their own that callers rarely want
/// rendered as a digit.
template <typename Type>
concept text_spellable_integer =
    std::integral<Type> && !std::same_as<std::remove_cv_t<Type>, bool> &&
    sizeof(Type) <= sizeof(std::uint64_t);

/// @ingroup text
///
/// The exact number of characters that the decimal spelling of an integer type
/// can require. The largest magnitude needs one character more than the type
/// represents without loss, and a signed type needs one more again for a sign.
template <typename Integer>
  requires text_spellable_integer<Integer>
inline constexpr std::size_t DIGITS_CAPACITY{
    static_cast<std::size_t>(std::numeric_limits<Integer>::digits10) + 1 +
    (std::numeric_limits<Integer>::is_signed ? 1U : 0U)};

/// @ingroup text
///
/// A buffer wide enough for the decimal spelling of any integer of up to 64
/// bits, including a leading sign. That bound is twenty characters, reached by
/// both the largest unsigned value and the smallest signed one.
using DigitsBuffer = std::array<char, std::max(DIGITS_CAPACITY<std::uint64_t>,
                                               DIGITS_CAPACITY<std::int64_t>)>;

/// @ingroup text
///
/// Write the decimal spelling of an integer into a caller-provided buffer,
/// returning a view of the characters written. That view stays valid for as
/// long as the buffer does. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// sourcemeta::core::DigitsBuffer buffer;
/// assert(sourcemeta::core::digits_view(1234, buffer) == "1234");
/// ```
///
/// Use this when the result feeds a lookup or another view-taking interface.
/// It never consults a locale, so a digit grouping separator can never appear
/// in the result.
///
/// Any sufficiently large buffer is accepted, so a caller that only ever
/// formats a narrow type can size one down to its capacity. Requiring the
/// buffer to fit the widest possible value is what makes the conversion unable
/// to fail, which is why it reports no error.
template <typename Integer, std::size_t Capacity>
  requires text_spellable_integer<Integer> &&
           (Capacity >= DIGITS_CAPACITY<Integer>)
inline auto digits_view(const Integer value,
                        std::array<char, Capacity> &buffer) noexcept
    -> std::string_view {
  const auto result{
      std::to_chars(buffer.data(), buffer.data() + buffer.size(), value)};
  return {buffer.data(), static_cast<std::size_t>(result.ptr - buffer.data())};
}

/// @ingroup text
///
/// Append the decimal spelling of an integer to a string like output sink. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// std::string result{"port="};
/// sourcemeta::core::digits_append(result, 8080);
/// assert(result == "port=8080");
/// ```
template <typename Output, typename Integer>
  requires text_spellable_integer<Integer>
inline auto digits_append(Output &output, const Integer value) -> void {
  std::array<char, DIGITS_CAPACITY<Integer>> buffer;
  const auto digits{digits_view(value, buffer)};
  output.append(digits.data(), digits.size());
}

/// @ingroup text
///
/// Write the decimal spelling of an integer to a stream. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <iostream>
///
/// sourcemeta::core::digits_write(std::cout, 1234);
/// // prints: 1234
/// ```
///
/// Prefer this to the stream insertion operator wherever the output has a
/// machine-readable grammar. Insertion formats through the stream's imbued
/// locale, so a locale carrying a digit grouping separator would otherwise
/// break the result apart.
template <typename CharT, typename Traits, typename Integer>
  requires text_spellable_integer<Integer>
inline auto digits_write(std::basic_ostream<CharT, Traits> &stream,
                         const Integer value) -> void {
  std::array<char, DIGITS_CAPACITY<Integer>> buffer;
  const auto digits{digits_view(value, buffer)};
  stream.write(digits.data(), static_cast<std::streamsize>(digits.size()));
}

/// @ingroup text
///
/// Decode a single hexadecimal digit into its numeric value, returning a
/// negative value when the character is not a hexadecimal digit. Both letter
/// cases are accepted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::hex_digit_value('a') == 10);
/// assert(sourcemeta::core::hex_digit_value('z') < 0);
/// ```
inline auto hex_digit_value(const char character) noexcept -> std::int8_t {
  // Indexed by byte value: ASCII '0'-'9', 'A'-'F', and 'a'-'f' map to their
  // hexadecimal value, everything else to -1
  static constexpr std::array<std::int8_t, 256> TABLE{
      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,
       6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
       -1, -1, -1, -1}};
  return TABLE[static_cast<unsigned char>(character)];
}

/// @ingroup text
///
/// Check whether the given character is an ASCII hexadecimal digit
/// (`'0'`-`'9'`, `'a'`-`'f'`, `'A'`-`'F'`). For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::is_hex_digit('a'));
/// assert(!sourcemeta::core::is_hex_digit('z'));
/// ```
inline auto is_hex_digit(const char character) noexcept -> bool {
  return hex_digit_value(character) >= 0;
}

/// @ingroup text
///
/// Decode a hexadecimal string into its raw bytes, returning no value when
/// the input contains a character outside the hexadecimal alphabet, or has an
/// odd length unless `allow_odd_length` is set, in which case a leading zero
/// nibble is assumed. Both letter cases are accepted. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// const auto bytes{sourcemeta::core::hex_to_bytes("666f6f")};
/// assert(bytes.has_value());
/// assert(bytes.value() == "foo");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto hex_to_bytes(const std::string_view input,
                  const bool allow_odd_length = false)
    -> std::optional<std::string>;

/// @ingroup text
///
/// Encode a byte sequence as a lowercase hexadecimal string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::bytes_to_hex("foo") == "666f6f");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto bytes_to_hex(const std::string_view input) -> std::string;

/// @ingroup text
///
/// Return whether two strings are equal under ASCII case-insensitive
/// comparison. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::equals_ignore_case("Hello", "hELLO"));
/// assert(!sourcemeta::core::equals_ignore_case("foo", "bar"));
/// ```
///
/// This comparison is allocation-free, as it compares the lowercase form of
/// each character without materialising lowercased copies of its arguments.
inline auto equals_ignore_case(const std::string_view left,
                               const std::string_view right) noexcept -> bool {
  if (left.size() != right.size()) {
    return false;
  }

  for (std::size_t index{0}; index < left.size(); ++index) {
    if (to_lowercase(left[index]) != to_lowercase(right[index])) {
      return false;
    }
  }

  return true;
}

/// @ingroup text
///
/// Return whether `value` begins with `prefix` under ASCII case-insensitive
/// comparison. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::starts_with_ignore_case("__Host-id", "__host-"));
/// assert(!sourcemeta::core::starts_with_ignore_case("id", "__host-"));
/// ```
///
/// Like the other case-insensitive comparisons, this is allocation-free.
inline auto starts_with_ignore_case(const std::string_view value,
                                    const std::string_view prefix) noexcept
    -> bool {
  if (value.size() < prefix.size()) {
    return false;
  }

  for (std::size_t index{0}; index < prefix.size(); ++index) {
    if (to_lowercase(value[index]) != to_lowercase(prefix[index])) {
      return false;
    }
  }

  return true;
}

/// @ingroup text
///
/// Return whether one string orders before another under ASCII case-insensitive
/// lexicographic comparison, which is useful as a sort comparator. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::less_ignore_case("apple", "Banana"));
/// assert(!sourcemeta::core::less_ignore_case("Banana", "apple"));
/// ```
///
/// Like the equality comparison, this is allocation-free.
inline auto less_ignore_case(const std::string_view left,
                             const std::string_view right) noexcept -> bool {
  const auto length{left.size() < right.size() ? left.size() : right.size()};
  for (std::size_t index{0}; index < length; ++index) {
    const auto left_lower{to_lowercase(left[index])};
    const auto right_lower{to_lowercase(right[index])};
    if (left_lower != right_lower) {
      return left_lower < right_lower;
    }
  }

  return left.size() < right.size();
}

/// @ingroup text
///
/// Collapse consecutive runs of a character into a single occurrence. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
///
/// assert(sourcemeta::core::squeeze("a//b///c", '/') == "a/b/c");
/// ```
SOURCEMETA_CORE_TEXT_EXPORT
auto squeeze(const std::string_view input, const char character) -> std::string;

/// @ingroup text
///
/// Collapse consecutive runs of a character into a single occurrence, appending
/// the result to a string like output sink rather than allocating a new
/// string. The output must not alias the input. For example:
///
/// ```cpp
/// #include <sourcemeta/core/text.h>
/// #include <cassert>
/// #include <string>
///
/// std::string output{"path="};
/// sourcemeta::core::squeeze("a//b", '/', output);
/// assert(output == "path=a/b");
/// ```
template <typename Output>
auto squeeze(const std::string_view input, const char character, Output &output)
    -> void {
  bool in_run{false};
  for (const auto value : input) {
    if (value == character) {
      if (!in_run) {
        output.push_back(value);
      }

      in_run = true;
    } else {
      output.push_back(value);
      in_run = false;
    }
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
