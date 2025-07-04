#ifndef SOURCEMETA_CORE_JSON_STRINGIFY_H_
#define SOURCEMETA_CORE_JSON_STRINGIFY_H_

#include <sourcemeta/core/json_value.h>

#include "grammar.h"

#include <algorithm> // std::transform, std::sort
#include <cstddef>   // std::size_t
#include <cstdint>   // std::int64_t
#include <iomanip>   // std::setprecision
#include <ios>       // std::noshowpoint, std::fixed
#include <iterator>  // std::next, std::cbegin, std::cend, std::back_inserter
#include <ostream>   // std::basic_ostream
#include <sstream>   // std::ostringstream
#include <string>    // std::to_string
#include <vector>    // std::vector

namespace sourcemeta::core::internal {
constexpr auto LINE_WIDTH{80};
constexpr auto INDENTATION_MULTIPLIER{2};
template <typename CharT, typename Traits>
auto indent(std::basic_ostream<CharT, Traits> &stream,
            const std::size_t indentation) -> void {
  for (std::size_t index{0}; index < indentation * INDENTATION_MULTIPLIER;
       index++) {
    stream.put(internal::token_whitespace_space<CharT>);
  }
}
} // namespace sourcemeta::core::internal

namespace sourcemeta::core {

template <template <typename T> typename Allocator>
auto stringify(
    const std::nullptr_t,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream)
    -> void {
  stream.write(
      internal::constant_null<typename JSON::Char, typename JSON::CharTraits>.data(),
      internal::constant_null<typename JSON::Char, typename JSON::CharTraits>.size());
}

template <template <typename T> typename Allocator>
auto stringify(
    const bool value,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream)
    -> void {
  if (value) {
    stream.write(
        internal::constant_true<typename JSON::Char, typename JSON::CharTraits>.data(),
        internal::constant_true<typename JSON::Char, typename JSON::CharTraits>.size());
  } else {
    stream.write(
        internal::constant_false<typename JSON::Char, typename JSON::CharTraits>.data(),
        internal::constant_false<typename JSON::Char, typename JSON::CharTraits>.size());
  }
}

template <template <typename T> typename Allocator>
auto stringify(
    const std::int64_t value,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream)
    -> void {
  const auto string{std::to_string(value)};
  stream.write(string.c_str(),
               static_cast<typename std::basic_ostream<
                   typename JSON::Char, typename JSON::CharTraits>::int_type>(
                   string.size()));
}

template <template <typename T> typename Allocator>
auto stringify(
    const double value, const bool is_integer_real,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream)
    -> void {
  if (value == static_cast<double>(0.0)) {
    stream.write("0.0", 3);
  } else if (is_integer_real) {
    const auto flags{stream.flags()};
    const auto precision{stream.precision()};
    stream << std::fixed << std::setprecision(1) << value;
    stream.flags(flags);
    stream.precision(precision);
  } else {
    const auto flags{stream.flags()};
    const auto precision{stream.precision()};
    stream << std::noshowpoint << value;
    stream.flags(flags);
    stream.precision(precision);
  }
}

template <template <typename T> typename Allocator>
auto stringify(
    const JSON &document,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare) -> void;

template <template <typename T> typename Allocator>
auto stringify(
    const typename JSON::String &document,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream)
    -> void {
  stream.put(internal::token_string_quote<typename JSON::Char>);
  for (const auto character : document) {
    switch (character) {
      case internal::token_string_escape<typename JSON::Char>:
      case internal::token_string_quote<typename JSON::Char>:
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(character);
        break;

      // See https://www.asciitable.com
      // See https://www.rfc-editor.org/rfc/rfc4627#section-2.5

      // Null
      case '\u0000':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('0');
        break;
      // Start of heading
      case '\u0001':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('1');
        break;
      // Start of text
      case '\u0002':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('2');
        break;
      // End of text
      case '\u0003':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('3');
        break;
      // End of transmission
      case '\u0004':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('4');
        break;
      // Enquiry
      case '\u0005':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('5');
        break;
      // Acknowledge
      case '\u0006':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('6');
        break;
      // Bell
      case '\u0007':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('7');
        break;
      // Backspace
      case '\b':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(
            internal::token_string_escape_backspace<typename JSON::Char>);
        break;
      // Horizontal tab
      case '\t':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(
            internal::token_string_escape_tabulation<typename JSON::Char>);
        break;
      // Line feed
      case '\n':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(
            internal::token_string_escape_line_feed<typename JSON::Char>);
        break;
      // Vertical tab
      case '\u000B':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('B');
        break;
      // Form feed
      case '\f':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(
            internal::token_string_escape_form_feed<typename JSON::Char>);
        break;
      // Carriage return
      case '\r':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(
            internal::token_string_escape_carriage_return<typename JSON::Char>);
        break;
      // Shift out
      case '\u000E':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('E');
        break;
      // Shift in
      case '\u000F':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('0');
        stream.put('F');
        break;
      // Data link escape
      case '\u0010':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('0');
        break;
      // Device control 1
      case '\u0011':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('1');
        break;
      // Device control 2
      case '\u0012':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('2');
        break;
      // Device control 3
      case '\u0013':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('3');
        break;
      // Device control 4
      case '\u0014':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('4');
        break;
      // Negative acknowledge
      case '\u0015':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('5');
        break;
      // Synchronous idle
      case '\u0016':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('6');
        break;
      // End of transmission block
      case '\u0017':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('7');
        break;
      // Cancel
      case '\u0018':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('8');
        break;
      // End of medium
      case '\u0019':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('9');
        break;
      // Substitute
      case '\u001A':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('A');
        break;
      // Escape
      case '\u001B':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('B');
        break;
      // File separator
      case '\u001C':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('C');
        break;
      // Group separator
      case '\u001D':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('D');
        break;
      // Record separator
      case '\u001E':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('E');
        break;
      // Unit separator
      case '\u001F':
        stream.put(internal::token_string_escape<typename JSON::Char>);
        stream.put(internal::token_string_escape_unicode<typename JSON::Char>);
        stream.put('0');
        stream.put('0');
        stream.put('1');
        stream.put('F');
        break;
      default:
        stream.put(character);
    }
  }

  stream.put(internal::token_string_quote<typename JSON::Char>);
}

template <template <typename T> typename Allocator>
auto stringify(
    const typename JSON::Array &document,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare) -> void {
  stream.put(internal::token_array_begin<typename JSON::Char>);
  const auto end{std::cend(document)};
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    stringify<Allocator>(*iterator, stream, compare);
    if (std::next(iterator) != end) {
      stream.put(internal::token_array_delimiter<typename JSON::Char>);
    }
  }

  stream.put(internal::token_array_end<typename JSON::Char>);
}

template <template <typename T> typename Allocator>
auto stringify(
    const typename JSON::Object &document, const JSON &container,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare) -> void {
  stream.put(internal::token_object_begin<typename JSON::Char>);

  if (compare) {
    std::vector<JSON::String, JSON::Allocator<JSON::String>> keys;
    keys.reserve(container.size());
    std::transform(std::cbegin(document), std::cend(document),
                   std::back_inserter(keys),
                   [](const auto &property) { return property.first; });
    std::ranges::sort(keys, compare);
    const auto end{std::cend(keys)};
    for (auto iterator = std::cbegin(keys); iterator != end; ++iterator) {
      stringify<Allocator>(*iterator, stream);
      stream.put(internal::token_object_key_delimiter<typename JSON::Char>);
      stringify<Allocator>(container.at(*iterator), stream, compare);
      if (std::next(iterator) != end) {
        stream.put(internal::token_object_delimiter<typename JSON::Char>);
      }
    }
  } else {
    const auto end{std::cend(document)};
    for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
      stringify<Allocator>(iterator->first, stream);
      stream.put(internal::token_object_key_delimiter<typename JSON::Char>);
      stringify<Allocator>(iterator->second, stream, compare);
      if (std::next(iterator) != end) {
        stream.put(internal::token_object_delimiter<typename JSON::Char>);
      }
    }
  }

  stream.put(internal::token_object_end<typename JSON::Char>);
}

template <template <typename T> typename Allocator>
auto prettify(
    const typename JSON::Object &document, const JSON &,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare, const std::size_t) -> void;

template <template <typename T> typename Allocator>
auto prettify(
    const typename JSON::Array &document,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare, const std::size_t indentation,
    const std::size_t property_size) -> void {
  const auto end{std::cend(document)};
  const auto effective_indentation{
      (indentation * internal::INDENTATION_MULTIPLIER) + property_size};

  // Attempt to print arrays in a single line if possible

  bool prettify_in_place{effective_indentation < internal::LINE_WIDTH};
  std::ostringstream inplace;
  inplace.put(internal::token_array_begin<typename JSON::Char>);
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    if (iterator->is_object() || iterator->is_array()) {
      prettify_in_place = false;
      break;
    }

    inplace.put(internal::token_whitespace_space<typename JSON::Char>);
    prettify<Allocator>(*iterator, inplace, compare, indentation);
    if (std::next(iterator) == end) {
      inplace.put(internal::token_whitespace_space<typename JSON::Char>);
    } else {
      inplace.put(internal::token_array_delimiter<typename JSON::Char>);
    }

    if (inplace.str().size() + effective_indentation >= internal::LINE_WIDTH) {
      prettify_in_place = false;
      break;
    }
  }

  if (prettify_in_place) {
    stream << inplace.str();
    stream.put(internal::token_array_end<typename JSON::Char>);
    return;
  }

  stream.put(internal::token_array_begin<typename JSON::Char>);
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    stream.put(internal::token_whitespace_line_feed<typename JSON::Char>);
    internal::indent(stream, indentation + 1);
    prettify<Allocator>(*iterator, stream, compare, indentation + 1);
    if (std::next(iterator) == end) {
      stream.put(internal::token_whitespace_line_feed<typename JSON::Char>);
    } else {
      stream.put(internal::token_array_delimiter<typename JSON::Char>);
    }
  }

  if (std::cbegin(document) != end) {
    internal::indent(stream, indentation);
  }

  stream.put(internal::token_array_end<typename JSON::Char>);
}

template <template <typename T> typename Allocator>
auto prettify(
    const typename JSON::Object &document, const JSON &container,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare, const std::size_t indentation) -> void {
  stream.put(internal::token_object_begin<typename JSON::Char>);

  if (compare) {
    std::vector<JSON::String, JSON::Allocator<JSON::String>> keys;
    keys.reserve(container.size());
    std::transform(std::cbegin(document), std::cend(document),
                   std::back_inserter(keys),
                   [](const auto &property) { return property.first; });
    std::ranges::sort(keys, compare);
    const auto end{std::cend(keys)};
    for (auto iterator = std::cbegin(keys); iterator != end; ++iterator) {
      stream.put(internal::token_whitespace_line_feed<typename JSON::Char>);
      internal::indent(stream, indentation + 1);
      const auto current_position{stream.tellp()};
      stringify<Allocator>(*iterator, stream);
      stream.put(internal::token_object_key_delimiter<typename JSON::Char>);
      stream.put(internal::token_whitespace_space<typename JSON::Char>);
      prettify<Allocator>(
          container.at(*iterator), stream, compare, indentation + 1,
          // Pass the length of the property name as encoded in JSON
          // to help determine the actual current column
          static_cast<std::size_t>(stream.tellp() - current_position));
      if (std::next(iterator) == end) {
        stream.put(internal::token_whitespace_line_feed<typename JSON::Char>);
      } else {
        stream.put(internal::token_object_delimiter<typename JSON::Char>);
      }
    }
  } else {
    const auto end{std::cend(document)};
    for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
      stream.put(internal::token_whitespace_line_feed<typename JSON::Char>);
      internal::indent(stream, indentation + 1);
      const auto current_position{stream.tellp()};
      stringify<Allocator>(iterator->first, stream);
      stream.put(internal::token_object_key_delimiter<typename JSON::Char>);
      stream.put(internal::token_whitespace_space<typename JSON::Char>);
      prettify<Allocator>(
          iterator->second, stream, compare, indentation + 1,
          // Pass the length of the property name as encoded in JSON
          // to help determine the actual current column
          static_cast<std::size_t>(stream.tellp() - current_position));
      if (std::next(iterator) == end) {
        stream.put(internal::token_whitespace_line_feed<typename JSON::Char>);
      } else {
        stream.put(internal::token_object_delimiter<typename JSON::Char>);
      }
    }
  }

  if (std::cbegin(document) != std::cend(document)) {
    internal::indent(stream, indentation);
  }

  stream.put(internal::token_object_end<typename JSON::Char>);
}

template <template <typename T> typename Allocator>
auto stringify(
    const JSON &document,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare) -> void {
  switch (document.type()) {
    case JSON::Type::Null:
      stringify<Allocator>(nullptr, stream);
      break;
    case JSON::Type::Boolean:
      stringify<Allocator>(document.to_boolean(), stream);
      break;
    case JSON::Type::Integer:
      stringify<Allocator>(document.to_integer(), stream);
      break;
    case JSON::Type::Real:
      stringify<Allocator>(document.to_real(), document.is_integer_real(),
                           stream);
      break;
    case JSON::Type::String:
      stringify<Allocator>(document.to_string(), stream);
      break;
    case JSON::Type::Array:
      stringify<Allocator>(document.as_array(), stream, compare);
      break;
    case JSON::Type::Object:
      stringify<Allocator>(document.as_object(), document, stream, compare);
      break;
  }
}

// TODO: Get rid of unused Allocator templates in this file

template <template <typename T> typename Allocator>
auto prettify(
    const JSON &document,
    std::basic_ostream<typename JSON::Char, typename JSON::CharTraits> &stream,
    const JSON::KeyComparison &compare, const std::size_t indentation = 0,
    const std::size_t property_size = 0) -> void {
  switch (document.type()) {
    case JSON::Type::Null:
      stringify<Allocator>(nullptr, stream);
      break;
    case JSON::Type::Boolean:
      stringify<Allocator>(document.to_boolean(), stream);
      break;
    case JSON::Type::Integer:
      stringify<Allocator>(document.to_integer(), stream);
      break;
    case JSON::Type::Real:
      stringify<Allocator>(document.to_real(), document.is_integer_real(),
                           stream);
      break;
    case JSON::Type::String:
      stringify<Allocator>(document.to_string(), stream);
      break;
    case JSON::Type::Array:
      prettify<Allocator>(document.as_array(), stream, compare, indentation,
                          property_size);
      break;
    case JSON::Type::Object:
      prettify<Allocator>(document.as_object(), document, stream, compare,
                          indentation);
      break;
  }
}

} // namespace sourcemeta::core

#endif
