#ifndef SOURCEMETA_CORE_JSON_STRINGIFY_H_
#define SOURCEMETA_CORE_JSON_STRINGIFY_H_

#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/text.h>

#include "grammar.h"

#include <array>    // std::array
#include <cassert>  // assert
#include <charconv> // std::to_chars
#include <cmath>    // std::signbit
#include <cstddef>  // std::size_t
#include <cstdint>  // std::int64_t
#include <ios>      // std::streamsize
#include <iterator> // std::next, std::cbegin, std::cend, std::back_inserter
#include <ostream>  // std::basic_ostream
#include <sstream>  // std::ostringstream
#include <string>   // std::basic_string
#include <vector>   // std::vector

namespace sourcemeta::core::internal {
constexpr auto LINE_WIDTH{80};

constexpr auto &STRING_CONTROL_ESCAPES{
    CONSTANT_STRING_CONTROL_ESCAPES<JSON::Char, JSON::CharTraits>};

inline auto stringify_string_size(const JSON::String &document) -> std::size_t {
  // Account for the surrounding quotes
  std::size_t result{2};
  for (const auto character : document) {
    const auto code_point{static_cast<unsigned char>(character)};
    if (code_point < STRING_CONTROL_ESCAPES.size()) {
      result += STRING_CONTROL_ESCAPES[code_point].size();
    } else if (character == internal::TOKEN_STRING_ESCAPE<JSON::Char> ||
               character == internal::TOKEN_STRING_QUOTE<JSON::Char>) {
      result += 2;
    } else {
      result += 1;
    }
  }

  return result;
}

template <typename CharT, typename Traits>
auto indent(std::basic_ostream<CharT, Traits> &stream,
            const std::size_t indentation, const std::size_t indent_by)
    -> void {
  for (std::size_t index{0}; index < indentation * indent_by; index++) {
    stream.put(internal::TOKEN_WHITESPACE_SPACE<CharT>);
  }
}
} // namespace sourcemeta::core::internal

namespace sourcemeta::core {

template <template <typename T> typename Allocator>
auto stringify(const std::nullptr_t,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stream.write(internal::CONSTANT_NULL<JSON::Char, JSON::CharTraits>.data(),
               internal::CONSTANT_NULL<JSON::Char, JSON::CharTraits>.size());
}

template <template <typename T> typename Allocator>
auto stringify(const bool value,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  if (value) {
    stream.write(internal::CONSTANT_TRUE<JSON::Char, JSON::CharTraits>.data(),
                 internal::CONSTANT_TRUE<JSON::Char, JSON::CharTraits>.size());
  } else {
    stream.write(internal::CONSTANT_FALSE<JSON::Char, JSON::CharTraits>.data(),
                 internal::CONSTANT_FALSE<JSON::Char, JSON::CharTraits>.size());
  }
}

template <template <typename T> typename Allocator>
auto stringify(const std::int64_t value,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  digits_write(stream, value);
}

template <template <typename T> typename Allocator>
auto stringify(const double value, const bool is_integral,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  // RFC 8259 Section 6 permits the -0.0 number syntax and parsing preserves
  // the sign of a zero, so serialisation keeps the sign as well and the
  // round trip is lossless
  if (value == 0.0) {
    if (std::signbit(value)) {
      stream.write("-0.0", 4);
    } else {
      stream.write("0.0", 3);
    }
  } else if (is_integral) {
    // Write the integer digits followed by an explicit ".0" to preserve the
    // real type. Using to_chars rather than a formatted stream keeps the
    // decimal separator independent of the global locale, which otherwise
    // corrupts the output under a comma-decimal locale
    std::array<char, 344> buffer{};
    const auto result{std::to_chars(buffer.data(),
                                    buffer.data() + buffer.size(), value,
                                    std::chars_format::fixed)};
    assert(result.ec == std::errc{});
    stream.write(buffer.data(), result.ptr - buffer.data());
    stream.write(".0", 2);
  } else {
    std::array<char, 64> buffer{};
    const auto result{
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value)};
    // This can't realistically happen on production given the buffer size
    assert(result.ec == std::errc{});
    stream.write(buffer.data(), result.ptr - buffer.data());
  }
}

template <template <typename T> typename Allocator>
auto stringify(const typename JSON::String &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stream.put(internal::TOKEN_STRING_QUOTE<JSON::Char>);

  // Emit the characters that need no escaping in runs, as writing a block at
  // a time is considerably cheaper than writing a character at a time
  std::size_t run{0};
  for (std::size_t index{0}; index < document.size(); index++) {
    const auto character{document[index]};
    const auto code_point{static_cast<unsigned char>(character)};
    const auto is_control{code_point < internal::STRING_CONTROL_ESCAPES.size()};
    if (!is_control && character != internal::TOKEN_STRING_ESCAPE<JSON::Char> &&
        character != internal::TOKEN_STRING_QUOTE<JSON::Char>) {
      continue;
    }

    if (index == run + 1) {
      stream.put(document[run]);
    } else if (index > run) {
      stream.write(document.data() + run,
                   static_cast<std::streamsize>(index - run));
    }

    if (is_control) {
      const auto escape{internal::STRING_CONTROL_ESCAPES[code_point]};
      stream.write(escape.data(), static_cast<std::streamsize>(escape.size()));
    } else {
      stream.put(internal::TOKEN_STRING_ESCAPE<JSON::Char>);
      stream.put(character);
    }

    run = index + 1;
  }

  if (run < document.size()) {
    stream.write(document.data() + run,
                 static_cast<std::streamsize>(document.size() - run));
  }

  stream.put(internal::TOKEN_STRING_QUOTE<JSON::Char>);
}

template <template <typename T> typename Allocator>
auto stringify(const typename JSON::Array &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stream.put(internal::TOKEN_ARRAY_BEGIN<JSON::Char>);
  const auto end{std::cend(document)};
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    stringify<Allocator>(*iterator, stream);
    if (std::next(iterator) != end) {
      stream.put(internal::TOKEN_ARRAY_DELIMITER<JSON::Char>);
    }
  }

  stream.put(internal::TOKEN_ARRAY_END<JSON::Char>);
}

template <template <typename T> typename Allocator>
auto stringify(const typename JSON::Object &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stream.put(internal::TOKEN_OBJECT_BEGIN<JSON::Char>);

  const auto end{std::cend(document)};
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    stringify<Allocator>(iterator->first, stream);
    stream.put(internal::TOKEN_OBJECT_KEY_DELIMITER<JSON::Char>);
    stringify<Allocator>(iterator->second, stream);
    if (std::next(iterator) != end) {
      stream.put(internal::TOKEN_OBJECT_DELIMITER<JSON::Char>);
    }
  }

  stream.put(internal::TOKEN_OBJECT_END<JSON::Char>);
}

template <template <typename T> typename Allocator>
auto prettify(const typename JSON::Object &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const std::size_t indentation, const std::size_t indent_by)
    -> void;

template <template <typename T> typename Allocator>
auto prettify(const typename JSON::Array &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const std::size_t indentation, const std::size_t indent_by,
              const std::size_t property_size) -> void {
  const auto end{std::cend(document)};
  const auto effective_indentation{(indentation * indent_by) + property_size};

  // Attempt to print arrays in a single line if possible

  bool prettify_in_place{effective_indentation < internal::LINE_WIDTH};
  std::ostringstream inplace;
  inplace.put(internal::TOKEN_ARRAY_BEGIN<JSON::Char>);
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    if (iterator->is_object() || iterator->is_array()) {
      prettify_in_place = false;
      break;
    }

    inplace.put(internal::TOKEN_WHITESPACE_SPACE<JSON::Char>);
    prettify<Allocator>(*iterator, inplace, indentation, indent_by);
    if (std::next(iterator) == end) {
      inplace.put(internal::TOKEN_WHITESPACE_SPACE<JSON::Char>);
    } else {
      inplace.put(internal::TOKEN_ARRAY_DELIMITER<JSON::Char>);
    }

    if (inplace.str().size() + effective_indentation >= internal::LINE_WIDTH) {
      prettify_in_place = false;
      break;
    }
  }

  if (prettify_in_place) {
    stream << inplace.str();
    stream.put(internal::TOKEN_ARRAY_END<JSON::Char>);
    return;
  }

  stream.put(internal::TOKEN_ARRAY_BEGIN<JSON::Char>);
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    stream.put(internal::TOKEN_WHITESPACE_LINE_FEED<JSON::Char>);
    internal::indent(stream, indentation + 1, indent_by);
    prettify<Allocator>(*iterator, stream, indentation + 1, indent_by);
    if (std::next(iterator) == end) {
      stream.put(internal::TOKEN_WHITESPACE_LINE_FEED<JSON::Char>);
    } else {
      stream.put(internal::TOKEN_ARRAY_DELIMITER<JSON::Char>);
    }
  }

  if (std::cbegin(document) != end) {
    internal::indent(stream, indentation, indent_by);
  }

  stream.put(internal::TOKEN_ARRAY_END<JSON::Char>);
}

template <template <typename T> typename Allocator>
auto prettify(const typename JSON::Object &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const std::size_t indentation, const std::size_t indent_by)
    -> void {
  stream.put(internal::TOKEN_OBJECT_BEGIN<JSON::Char>);

  const auto end{std::cend(document)};
  for (auto iterator = std::cbegin(document); iterator != end; ++iterator) {
    stream.put(internal::TOKEN_WHITESPACE_LINE_FEED<JSON::Char>);
    internal::indent(stream, indentation + 1, indent_by);
    // The length of the property name as encoded in JSON, plus its
    // delimiter and the space that follows it, to help determine the
    // actual current column
    const auto property_size{internal::stringify_string_size(iterator->first) +
                             2};
    stringify<Allocator>(iterator->first, stream);
    stream.put(internal::TOKEN_OBJECT_KEY_DELIMITER<JSON::Char>);
    stream.put(internal::TOKEN_WHITESPACE_SPACE<JSON::Char>);
    prettify<Allocator>(iterator->second, stream, indentation + 1, indent_by,
                        property_size);
    if (std::next(iterator) == end) {
      stream.put(internal::TOKEN_WHITESPACE_LINE_FEED<JSON::Char>);
    } else {
      stream.put(internal::TOKEN_OBJECT_DELIMITER<JSON::Char>);
    }
  }

  if (std::cbegin(document) != std::cend(document)) {
    internal::indent(stream, indentation, indent_by);
  }

  stream.put(internal::TOKEN_OBJECT_END<JSON::Char>);
}

template <template <typename T> typename Allocator>
auto stringify(const JSON &document,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
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
      stringify<Allocator>(document.to_real(), document.is_integral(), stream);
      break;
    case JSON::Type::String:
      stringify<Allocator>(document.to_string(), stream);
      break;
    case JSON::Type::Array:
      stringify<Allocator>(document.as_array(), stream);
      break;
    case JSON::Type::Object:
      stringify<Allocator>(document.as_object(), stream);
      break;
    case JSON::Type::Decimal:
      // We ALWAYS parse numbers with exponents as decimal, so if we don't
      // preserve the exponent, we might end up incorrectly treating the number
      // when parsing it again
      stream << document.to_decimal().to_scientific_string();
      break;
  }
}

// TODO: Get rid of unused Allocator templates in this file

template <template <typename T> typename Allocator>
auto prettify(const JSON &document,
              std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
              const std::size_t indentation = 0,
              const std::size_t indent_by = 2,
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
      stringify<Allocator>(document.to_real(), document.is_integral(), stream);
      break;
    case JSON::Type::String:
      stringify<Allocator>(document.to_string(), stream);
      break;
    case JSON::Type::Array:
      prettify<Allocator>(document.as_array(), stream, indentation, indent_by,
                          property_size);
      break;
    case JSON::Type::Object:
      prettify<Allocator>(document.as_object(), stream, indentation, indent_by);
      break;
    case JSON::Type::Decimal:
      // We ALWAYS parse numbers with exponents as decimal, so if we don't
      // preserve the exponent, we might end up incorrectly treating the number
      // when parsing it again
      stream << document.to_decimal().to_scientific_string();
      break;
  }
}

} // namespace sourcemeta::core

#endif
