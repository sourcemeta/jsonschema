#ifndef SOURCEMETA_CORE_JSON_PARSER_H_
#define SOURCEMETA_CORE_JSON_PARSER_H_

#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>

#include "grammar.h"

#include <cassert> // assert
#include <cstdint> // std::uint64_t, std::uint32_t
#include <vector>  // std::vector

namespace sourcemeta::core {

enum class TapeType : std::uint8_t {
  ObjectStart,
  ObjectEnd,
  ArrayStart,
  ArrayEnd,
  Key,
  String,
  Number,
  Null,
  True,
  False
};

struct TapeEntry {
  TapeType type;
  std::uint32_t offset;
  std::uint32_t length;
  std::uint32_t count;
  std::uint64_t line;
  std::uint64_t column;
};

namespace internal {

template <bool TrackPositions>
inline auto skip_whitespace(const char *&cursor, const char *end,
                            std::uint64_t &line, std::uint64_t &column)
    -> void {
  while (cursor < end) {
    switch (*cursor) {
      case internal::token_whitespace_space<typename JSON::Char>:
      case internal::token_whitespace_tabulation<typename JSON::Char>:
      case internal::token_whitespace_carriage_return<typename JSON::Char>:
        if constexpr (TrackPositions) {
          column += 1;
        }
        cursor++;
        continue;
      case internal::token_whitespace_line_feed<typename JSON::Char>:
        if constexpr (TrackPositions) {
          line += 1;
          column = 0;
        }
        cursor++;
        continue;
      default:
        return;
    }
  }
}

template <bool TrackPositions>
inline auto scan_null(const std::uint64_t line, std::uint64_t &column,
                      const char *&cursor, const char *end) -> void {
  for (
      const auto character :
      internal::constant_null<typename JSON::Char, typename JSON::CharTraits>.substr(
          1)) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    if (cursor >= end) {
      throw JSONParseError(line, column);
    }
    if (*cursor != character) {
      throw JSONParseError(line, column);
    }
    cursor++;
  }
}

template <bool TrackPositions>
inline auto scan_true(const std::uint64_t line, std::uint64_t &column,
                      const char *&cursor, const char *end) -> void {
  for (
      const auto character :
      internal::constant_true<typename JSON::Char, typename JSON::CharTraits>.substr(
          1)) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    if (cursor >= end) {
      throw JSONParseError(line, column);
    }
    if (*cursor != character) {
      throw JSONParseError(line, column);
    }
    cursor++;
  }
}

template <bool TrackPositions>
inline auto scan_false(const std::uint64_t line, std::uint64_t &column,
                       const char *&cursor, const char *end) -> void {
  for (
      const auto character :
      internal::constant_false<typename JSON::Char, typename JSON::CharTraits>.substr(
          1)) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    if (cursor >= end) {
      throw JSONParseError(line, column);
    }
    if (*cursor != character) {
      throw JSONParseError(line, column);
    }
    cursor++;
  }
}

template <bool TrackPositions>
inline auto scan_string_unicode_code_point(const std::uint64_t line,
                                           std::uint64_t &column,
                                           const char *&cursor, const char *end)
    -> unsigned long {
  unsigned long result{0};
  for (std::size_t index = 0; index < 4; index++) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    if (cursor >= end) {
      throw JSONParseError(line, column);
    }
    const char hex_char{*cursor++};
    unsigned long digit;
    if (hex_char >= '0' && hex_char <= '9') {
      digit = static_cast<unsigned long>(hex_char - '0');
    } else if (hex_char >= 'a' && hex_char <= 'f') {
      digit = static_cast<unsigned long>(hex_char - 'a') + 10;
    } else if (hex_char >= 'A' && hex_char <= 'F') {
      digit = static_cast<unsigned long>(hex_char - 'A') + 10;
    } else {
      throw JSONParseError(line, column);
    }
    result = (result << 4) | digit;
  }

  assert(result <= 0xFFFF);
  return result;
}

template <bool TrackPositions>
inline auto scan_string_unicode(const std::uint64_t line, std::uint64_t &column,
                                const char *&cursor, const char *end) -> void {
  auto code_point{scan_string_unicode_code_point<TrackPositions>(line, column,
                                                                 cursor, end)};
  using CharT = typename JSON::Char;

  if (code_point >= 0xDC00 && code_point <= 0xDFFF) {
    throw JSONParseError(line, column);
  }

  if (code_point >= 0xD800 && code_point <= 0xDBFF) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    if (cursor >= end) {
      throw JSONParseError(line, column);
    }
    if (*cursor != internal::token_string_escape<CharT>) {
      throw JSONParseError(line, column);
    }
    cursor++;

    if constexpr (TrackPositions) {
      column += 1;
    }
    if (cursor >= end) {
      throw JSONParseError(line, column);
    }
    if (*cursor != internal::token_string_escape_unicode<CharT>) {
      throw JSONParseError(line, column);
    }
    cursor++;

    const auto low_code_point{scan_string_unicode_code_point<TrackPositions>(
        line, column, cursor, end)};

    // See
    // https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF
    if (low_code_point < 0xDC00 || low_code_point > 0xDFFF) {
      throw JSONParseError(line, column);
    }
  }
}

template <bool TrackPositions>
inline auto scan_string_escape(const std::uint64_t line, std::uint64_t &column,
                               const char *&cursor, const char *end) -> void {
  if constexpr (TrackPositions) {
    column += 1;
  }
  if (cursor >= end) {
    throw JSONParseError(line, column);
  }
  switch (*cursor++) {
    case internal::token_string_quote<typename JSON::Char>:
    case internal::token_string_escape<typename JSON::Char>:
    case internal::token_string_solidus<typename JSON::Char>:
    case internal::token_string_escape_backspace<typename JSON::Char>:
    case internal::token_string_escape_form_feed<typename JSON::Char>:
    case internal::token_string_escape_line_feed<typename JSON::Char>:
    case internal::token_string_escape_carriage_return<typename JSON::Char>:
    case internal::token_string_escape_tabulation<typename JSON::Char>:
      return;
    case internal::token_string_escape_unicode<typename JSON::Char>:
      scan_string_unicode<TrackPositions>(line, column, cursor, end);
      return;
    default:
      throw JSONParseError(line, column);
  }
}

template <bool TrackPositions>
inline auto scan_string(const std::uint64_t line, std::uint64_t &column,
                        const char *&cursor, const char *end) -> void {
  using CharT = typename JSON::Char;
  while (cursor < end) {
    const char *scan{cursor};
    while (scan < end && *scan != internal::token_string_quote<CharT> &&
           *scan != internal::token_string_escape<CharT> &&
           static_cast<unsigned char>(*scan) >= 0x20) {
      scan++;
    }

    if (scan > cursor) {
      if constexpr (TrackPositions) {
        column += static_cast<std::uint64_t>(scan - cursor);
      }
      cursor = scan;
    }

    if (cursor >= end) {
      if constexpr (TrackPositions) {
        column += 1;
      }
      throw JSONParseError(line, column);
    }

    if constexpr (TrackPositions) {
      column += 1;
    }
    const char character{*cursor++};

    switch (character) {
      case internal::token_string_quote<typename JSON::Char>:
        return;
      case internal::token_string_escape<typename JSON::Char>:
        scan_string_escape<TrackPositions>(line, column, cursor, end);
        break;
      default:
        throw JSONParseError(line, column);
    }
  }

  if constexpr (TrackPositions) {
    column += 1;
  }
  throw JSONParseError(line, column);
}

template <bool TrackPositions>
inline auto scan_digits(const std::uint64_t line, std::uint64_t &column,
                        const char *&cursor, const char *end,
                        const bool at_least_one) -> void {
  using CharT = typename JSON::Char;
  bool found{false};
  while (cursor < end && *cursor >= internal::token_number_zero<CharT> &&
         *cursor <= internal::token_number_nine<CharT>) {
    found = true;
    if constexpr (TrackPositions) {
      column += 1;
    }
    cursor++;
  }
  if (at_least_one && !found) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
}

template <bool TrackPositions>
inline auto scan_number(const std::uint64_t line, std::uint64_t &column,
                        const char *&cursor, const char *end, const char first)
    -> void {
  using CharT = typename JSON::Char;
  if (first == internal::token_number_minus<CharT>) {
    if (cursor >= end || *cursor < internal::token_number_zero<CharT> ||
        *cursor > internal::token_number_nine<CharT>) {
      if constexpr (TrackPositions) {
        column += 1;
      }
      throw JSONParseError(line, column);
    }
  }

  const char int_start{first == internal::token_number_minus<CharT> ? *cursor
                                                                    : first};
  if (first == internal::token_number_minus<CharT>) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    cursor++;
  }

  if (int_start == internal::token_number_zero<CharT>) {
    if (cursor < end && *cursor >= internal::token_number_zero<CharT> &&
        *cursor <= internal::token_number_nine<CharT>) {
      if constexpr (TrackPositions) {
        column += 1;
      }
      throw JSONParseError(line, column);
    }
  } else {
    scan_digits<TrackPositions>(line, column, cursor, end, false);
  }

  if (cursor < end && *cursor == internal::token_number_decimal_point<CharT>) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    cursor++;
    scan_digits<TrackPositions>(line, column, cursor, end, true);
  }

  if (cursor < end &&
      (*cursor == internal::token_number_exponent_lowercase<CharT> ||
       *cursor == internal::token_number_exponent_uppercase<CharT>)) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    cursor++;
    if (cursor < end && (*cursor == internal::token_number_plus<CharT> ||
                         *cursor == internal::token_number_minus<CharT>)) {
      if constexpr (TrackPositions) {
        column += 1;
      }
      cursor++;
    }
    scan_digits<TrackPositions>(line, column, cursor, end, true);
  }
}

} // namespace internal

// NOLINTBEGIN(cppcoreguidelines-avoid-goto)

template <bool TrackPositions>
inline auto scan_json(const char *&cursor, const char *end,
                      const char *buffer_start, std::uint64_t &line,
                      std::uint64_t &column, std::vector<TapeEntry> &tape)
    -> void {
  struct ContainerFrame {
    std::size_t tape_index;
    std::uint32_t child_count;
  };

  using CharT = typename JSON::Char;
  char character = 0;
  std::vector<ContainerFrame> container_stack;
  container_stack.reserve(32);

  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;

  {
    const auto value_line{line};
    const auto value_column{column};
    switch (character) {
      case internal::token_true<CharT>:
        internal::scan_true<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::True, 0, 0, 0, value_line, value_column});
        return;
      case internal::token_false<CharT>:
        internal::scan_false<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::False, 0, 0, 0, value_line, value_column});
        return;
      case internal::token_null<CharT>:
        internal::scan_null<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::Null, 0, 0, 0, value_line, value_column});
        return;
      case internal::token_string_quote<CharT>: {
        const auto string_start{
            static_cast<std::uint32_t>(cursor - buffer_start)};
        internal::scan_string<TrackPositions>(line, column, cursor, end);
        const auto string_length{static_cast<std::uint32_t>(
            cursor - buffer_start - string_start - 1)};
        tape.push_back({TapeType::String, string_start, string_length, 0,
                        value_line, value_column});
        return;
      }
      case internal::token_array_begin<CharT>:
        goto do_scan_array;
      case internal::token_object_begin<CharT>:
        goto do_scan_object;
      case internal::token_number_minus<CharT>:
      case internal::token_number_zero<CharT>:
      case internal::token_number_one<CharT>:
      case internal::token_number_two<CharT>:
      case internal::token_number_three<CharT>:
      case internal::token_number_four<CharT>:
      case internal::token_number_five<CharT>:
      case internal::token_number_six<CharT>:
      case internal::token_number_seven<CharT>:
      case internal::token_number_eight<CharT>:
      case internal::token_number_nine<CharT>: {
        const auto number_start{
            static_cast<std::uint32_t>(cursor - buffer_start - 1)};
        internal::scan_number<TrackPositions>(line, column, cursor, end,
                                              character);
        const auto number_length{
            static_cast<std::uint32_t>(cursor - buffer_start - number_start)};
        tape.push_back({TapeType::Number, number_start, number_length, 0,
                        value_line, value_column});
        return;
      }
      default:
        throw JSONParseError(line, column);
    }
  }

  /*
   * Scan an array
   */

do_scan_array: {
  const auto start_index{tape.size()};
  tape.push_back({TapeType::ArrayStart, 0, 0, 0, line, column});
  container_stack.push_back({start_index, 0});

  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }

  if (*cursor == internal::token_array_end<CharT>) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    cursor++;
    tape[start_index].count = 0;
    tape.push_back({TapeType::ArrayEnd, 0, 0, 0, line, column});
    container_stack.pop_back();
    goto do_scan_container_end;
  }

  goto do_scan_array_item;
}

do_scan_array_item:
  assert(!container_stack.empty());
  container_stack.back().child_count++;

  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;

  {
    const auto value_line{line};
    const auto value_column{column};
    switch (character) {
      case internal::token_array_begin<CharT>:
        goto do_scan_array;
      case internal::token_object_begin<CharT>:
        goto do_scan_object;
      case internal::token_true<CharT>:
        internal::scan_true<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::True, 0, 0, 0, value_line, value_column});
        goto do_scan_array_item_separator;
      case internal::token_false<CharT>:
        internal::scan_false<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::False, 0, 0, 0, value_line, value_column});
        goto do_scan_array_item_separator;
      case internal::token_null<CharT>:
        internal::scan_null<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::Null, 0, 0, 0, value_line, value_column});
        goto do_scan_array_item_separator;
      case internal::token_string_quote<CharT>: {
        const auto string_start{
            static_cast<std::uint32_t>(cursor - buffer_start)};
        internal::scan_string<TrackPositions>(line, column, cursor, end);
        const auto string_length{static_cast<std::uint32_t>(
            cursor - buffer_start - string_start - 1)};
        tape.push_back({TapeType::String, string_start, string_length, 0,
                        value_line, value_column});
        goto do_scan_array_item_separator;
      }
      case internal::token_number_minus<CharT>:
      case internal::token_number_zero<CharT>:
      case internal::token_number_one<CharT>:
      case internal::token_number_two<CharT>:
      case internal::token_number_three<CharT>:
      case internal::token_number_four<CharT>:
      case internal::token_number_five<CharT>:
      case internal::token_number_six<CharT>:
      case internal::token_number_seven<CharT>:
      case internal::token_number_eight<CharT>:
      case internal::token_number_nine<CharT>: {
        const auto number_start{
            static_cast<std::uint32_t>(cursor - buffer_start - 1)};
        internal::scan_number<TrackPositions>(line, column, cursor, end,
                                              character);
        const auto number_length{
            static_cast<std::uint32_t>(cursor - buffer_start - number_start)};
        tape.push_back({TapeType::Number, number_start, number_length, 0,
                        value_line, value_column});
        goto do_scan_array_item_separator;
      }
      default:
        throw JSONParseError(line, column);
    }
  }

do_scan_array_item_separator:
  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;
  switch (character) {
    case internal::token_array_delimiter<CharT>:
      goto do_scan_array_item;
    case internal::token_array_end<CharT>: {
      assert(!container_stack.empty());
      auto &frame{container_stack.back()};
      tape[frame.tape_index].count = frame.child_count;
      tape.push_back({TapeType::ArrayEnd, 0, 0, 0, line, column});
      container_stack.pop_back();
      goto do_scan_container_end;
    }
    default:
      throw JSONParseError(line, column);
  }

  /*
   * Scan an object
   */

do_scan_object: {
  const auto start_index{tape.size()};
  tape.push_back({TapeType::ObjectStart, 0, 0, 0, line, column});
  container_stack.push_back({start_index, 0});

  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }

  if (*cursor == internal::token_object_end<CharT>) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    cursor++;
    tape[start_index].count = 0;
    tape.push_back({TapeType::ObjectEnd, 0, 0, 0, line, column});
    container_stack.pop_back();
    goto do_scan_container_end;
  }

  goto do_scan_object_key;
}

do_scan_object_key:
  assert(!container_stack.empty());
  container_stack.back().child_count++;

  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;
  switch (character) {
    case internal::token_string_quote<CharT>: {
      const auto key_start{static_cast<std::uint32_t>(cursor - buffer_start)};
      const auto key_line{line};
      const auto key_column{column};
      internal::scan_string<TrackPositions>(line, column, cursor, end);
      const auto key_length{
          static_cast<std::uint32_t>(cursor - buffer_start - key_start - 1)};
      tape.push_back(
          {TapeType::Key, key_start, key_length, 0, key_line, key_column});
      goto do_scan_object_separator;
    }
    default:
      throw JSONParseError(line, column);
  }

do_scan_object_separator:
  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;
  switch (character) {
    case internal::token_object_key_delimiter<CharT>:
      goto do_scan_object_value;
    default:
      throw JSONParseError(line, column);
  }

do_scan_object_value:
  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;

  {
    const auto value_line{line};
    const auto value_column{column};
    switch (character) {
      case internal::token_array_begin<CharT>:
        goto do_scan_array;
      case internal::token_object_begin<CharT>:
        goto do_scan_object;
      case internal::token_true<CharT>:
        internal::scan_true<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::True, 0, 0, 0, value_line, value_column});
        goto do_scan_object_property_end;
      case internal::token_false<CharT>:
        internal::scan_false<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::False, 0, 0, 0, value_line, value_column});
        goto do_scan_object_property_end;
      case internal::token_null<CharT>:
        internal::scan_null<TrackPositions>(line, column, cursor, end);
        tape.push_back({TapeType::Null, 0, 0, 0, value_line, value_column});
        goto do_scan_object_property_end;
      case internal::token_string_quote<CharT>: {
        const auto string_start{
            static_cast<std::uint32_t>(cursor - buffer_start)};
        internal::scan_string<TrackPositions>(line, column, cursor, end);
        const auto string_length{static_cast<std::uint32_t>(
            cursor - buffer_start - string_start - 1)};
        tape.push_back({TapeType::String, string_start, string_length, 0,
                        value_line, value_column});
        goto do_scan_object_property_end;
      }
      case internal::token_number_minus<CharT>:
      case internal::token_number_zero<CharT>:
      case internal::token_number_one<CharT>:
      case internal::token_number_two<CharT>:
      case internal::token_number_three<CharT>:
      case internal::token_number_four<CharT>:
      case internal::token_number_five<CharT>:
      case internal::token_number_six<CharT>:
      case internal::token_number_seven<CharT>:
      case internal::token_number_eight<CharT>:
      case internal::token_number_nine<CharT>: {
        const auto number_start{
            static_cast<std::uint32_t>(cursor - buffer_start - 1)};
        internal::scan_number<TrackPositions>(line, column, cursor, end,
                                              character);
        const auto number_length{
            static_cast<std::uint32_t>(cursor - buffer_start - number_start)};
        tape.push_back({TapeType::Number, number_start, number_length, 0,
                        value_line, value_column});
        goto do_scan_object_property_end;
      }
      default:
        throw JSONParseError(line, column);
    }
  }

do_scan_object_property_end:
  internal::skip_whitespace<TrackPositions>(cursor, end, line, column);
  if (cursor >= end) {
    if constexpr (TrackPositions) {
      column += 1;
    }
    throw JSONParseError(line, column);
  }
  if constexpr (TrackPositions) {
    column += 1;
  }
  character = *cursor++;
  switch (character) {
    case internal::token_object_delimiter<CharT>:
      goto do_scan_object_key;
    case internal::token_object_end<CharT>: {
      assert(!container_stack.empty());
      auto &frame{container_stack.back()};
      tape[frame.tape_index].count = frame.child_count;
      tape.push_back({TapeType::ObjectEnd, 0, 0, 0, line, column});
      container_stack.pop_back();
      goto do_scan_container_end;
    }
    default:
      throw JSONParseError(line, column);
  }

do_scan_container_end:
  if (container_stack.empty()) {
    return;
  }

  if (tape[container_stack.back().tape_index].type == TapeType::ArrayStart) {
    goto do_scan_array_item_separator;
  } else {
    goto do_scan_object_property_end;
  }
}

// NOLINTEND(cppcoreguidelines-avoid-goto)

} // namespace sourcemeta::core

#endif
