#ifndef SOURCEMETA_CORE_JSON_CONSTRUCT_H_
#define SOURCEMETA_CORE_JSON_CONSTRUCT_H_

#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>

#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>

#include "parser.h"

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint64_t, std::uint32_t, std::uint8_t
#include <functional>  // std::reference_wrapper
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable
#include <vector>      // std::vector

namespace sourcemeta::core {

namespace internal {

// The four hexadecimal digits of a `\u` escape, read from the position given
// and leaving it on whatever follows them
inline auto parse_hex4(const char *&position) -> unsigned long {
  unsigned long value{0};
  for (std::size_t index = 0; index < 4; index++) {
    const auto digit{hex_digit_value(*position++)};
    value = (value << 4) | static_cast<unsigned long>(digit < 0 ? 0 : digit);
  }

  return value;
}

inline auto unescape_string(const char *data, const std::uint32_t length,
                            const bool has_escape) -> JSON::String {
  JSON::String result;
  const char *cursor{data};
  const char *string_end{data + length};

  if (!has_escape) {
    result.append(data, length);
    return result;
  }

  result.reserve(length);
  while (cursor < string_end) {
    const char *scan{cursor};
    while (scan < string_end && *scan != '\\') {
      scan++;
    }

    if (scan > cursor) {
      result.append(cursor, static_cast<std::size_t>(scan - cursor));
      cursor = scan;
    }

    if (cursor >= string_end) {
      break;
    }

    assert(*cursor == '\\');
    cursor++;
    assert(cursor < string_end);

    switch (*cursor++) {
      case '"':
        result.push_back('"');
        break;
      case '\\':
        result.push_back('\\');
        break;
      case '/':
        result.push_back('/');
        break;
      case 'b':
        result.push_back('\b');
        break;
      case 'f':
        result.push_back('\f');
        break;
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      case 'u': {
        auto code_point{parse_hex4(cursor)};
        if (code_point >= 0xD800 && code_point <= 0xDBFF) {
          assert(cursor + 6 <= string_end);
          cursor += 2;
          const auto low{parse_hex4(cursor)};
          code_point = 0x10000 + ((code_point - 0xD800) << 10) + (low - 0xDC00);
        }

        sourcemeta::core::codepoint_to_utf8(static_cast<char32_t>(code_point),
                                            result);
        break;
      }
      // The scan accepted the escape, so what stands after the reverse solidus
      // is one of the characters the grammar names
      default:
        std::unreachable();
    }
  }

  return result;
}

// A number literal that the scanner accepts can still name a value outside the
// range this module represents, and where it does the failure belongs at the
// literal rather than at the start of the document
inline auto construct_decimal(const std::string_view value,
                              const std::uint64_t line,
                              const std::uint64_t column) -> JSON {
  try {
    return JSON{Decimal{value}};
  } catch (const DecimalParseError &) {
    throw JSONParseError(line, column);
  }
}

inline auto construct_number(const char *data, const std::uint32_t length,
                             const std::uint8_t flags,
                             const std::uint32_t significant_digits,
                             const std::uint64_t line,
                             const std::uint64_t column) -> JSON {
  if ((flags & TAPE_FLAG_NUMBER_EXPONENT) != 0) {
    return construct_decimal(std::string_view{data, length}, line, column);
  }

  if ((flags & TAPE_FLAG_NUMBER_DOT) != 0) {
    constexpr std::uint32_t MAX_SAFE_SIGNIFICANT_DIGITS{15};
    if (significant_digits > MAX_SAFE_SIGNIFICANT_DIGITS) {
      return construct_decimal(std::string_view{data, length}, line, column);
    }

    const std::string_view value{data, length};
    const auto double_result{sourcemeta::core::to_double_exact(value)};
    if (double_result.has_value()) {
      return JSON{double_result.value()};
    }

    return construct_decimal(value, line, column);
  }

  auto digit_length = length;
  if (digit_length > 0 && data[0] == '-') {
    digit_length--;
  }

  // A literal of digits alone stands for an exponent of zero, which is inside
  // the range the storage holds, so reading one back cannot fail the way a
  // literal that carries an exponent of its own can
  if (digit_length <= 19) {
    const std::string_view value{data, length};
    const auto int_result{sourcemeta::core::to_int64_t(value)};
    if (int_result.has_value()) {
      return JSON{int_result.value()};
    }

    return JSON{Decimal{value}};
  }

  return JSON{Decimal{std::string_view{data, length}}};
}

// Every value that is not a container is read the same way wherever it stands,
// and the kind a number literal names is settled by reading it rather than by
// what the scan recorded
inline auto construct_leaf(const char *buffer, const TapeEntry &entry) -> JSON {
  switch (entry.type) {
    case TapeType::True:
      return JSON{true};
    case TapeType::False:
      return JSON{false};
    case TapeType::Null:
      return JSON{nullptr};
    case TapeType::String:
      return JSON{
          unescape_string(buffer + entry.offset, entry.length,
                          (entry.flags & TAPE_FLAG_STRING_ESCAPE) != 0)};
    // A container is read by the walk itself rather than here, so what is left
    // for this to answer for is a number
    default:
      assert(entry.type == TapeType::Number);
      return construct_number(buffer + entry.offset, entry.length, entry.flags,
                              entry.count, entry.line, entry.column);
  }
}

inline auto post_column_for(const TapeEntry &entry) -> std::uint64_t {
  // A string carries the length of what stands between its quotation marks, so
  // the mark closing it stands one past that, while every other value carries
  // the length of the whole literal and so ends on its last character
  if (entry.type == TapeType::String || entry.type == TapeType::Key) {
    return entry.column + entry.length + 1;
  }

  return entry.column + entry.length - 1;
}

} // namespace internal

// NOLINTBEGIN(cppcoreguidelines-avoid-goto,bugprone-use-after-move)

#define CALLBACK_PRE(value_type, entry_ref, context, index, property)          \
  if (callback) {                                                              \
    callback(JSON::ParsePhase::Pre, value_type, (entry_ref).line,              \
             (entry_ref).column, context, index, property);                    \
  }

#define CALLBACK_POST(value_type, post_line, post_column)                      \
  if (callback) {                                                              \
    callback(JSON::ParsePhase::Post, value_type, post_line, post_column,       \
             JSON::ParseContext::Root, 0, empty_property);                     \
  }

inline auto construct_json(const char *buffer,
                           const std::vector<TapeEntry> &tape,
                           const JSON::ParseCallback &callback, JSON &output)
    -> void {
  using Result = JSON;
  enum class Container : std::uint8_t { Array, Object };
  std::vector<Container> levels;
  std::vector<std::reference_wrapper<Result>> frames;
  levels.reserve(32);
  frames.reserve(32);
  Result::String key;
  Result::Object::hash_type key_hash;
  std::uint64_t key_line{0};
  std::uint64_t key_column{0};
  std::size_t tape_index{0};
  static const JSON::String empty_property;

  assert(!tape.empty());

  const auto &entry{tape[tape_index]};
  if (entry.type == TapeType::ArrayStart) {
    CALLBACK_PRE(JSON::Type::Array, entry, JSON::ParseContext::Root, 0,
                 empty_property);
    goto do_construct_array;
  }

  if (entry.type == TapeType::ObjectStart) {
    CALLBACK_PRE(JSON::Type::Object, entry, JSON::ParseContext::Root, 0,
                 empty_property);
    goto do_construct_object;
  }

  output = internal::construct_leaf(buffer, entry);
  if (callback) {
    const auto value_type{output.type()};
    callback(JSON::ParsePhase::Pre, value_type, entry.line, entry.column,
             JSON::ParseContext::Root, 0, empty_property);
    callback(JSON::ParsePhase::Post, value_type, entry.line,
             internal::post_column_for(entry), JSON::ParseContext::Root, 0,
             empty_property);
  }

  return;

  /*
   * Construct an array
   */

do_construct_array: {
  const auto &array_entry{tape[tape_index]};
  assert(array_entry.type == TapeType::ArrayStart);
  const auto child_count{array_entry.count};
  tape_index++;

  if (levels.empty()) {
    levels.push_back(Container::Array);
    output = Result::make_array();
    frames.emplace_back(output);
  } else if (levels.back() == Container::Array) {
    levels.push_back(Container::Array);
    frames.back().get().push_back(Result::make_array());
    frames.emplace_back(frames.back().get().back());
  } else if (levels.back() == Container::Object) {
    levels.push_back(Container::Array);
    auto &inserted{frames.back().get().assign_assume_new(
        std::move(key), Result::make_array(), key_hash)};
    if (callback) {
      callback(JSON::ParsePhase::Pre, JSON::Type::Array, key_line, key_column,
               JSON::ParseContext::Property, 0,
               frames.back().get().as_object().back_key());
    }
    frames.emplace_back(inserted);
  }

  frames.back().get().as_array().reserve(child_count);

  if (child_count == 0) {
    assert(tape[tape_index].type == TapeType::ArrayEnd);
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(JSON::Type::Array, end_entry.line, end_entry.column);
    goto do_construct_container_end;
  }

  goto do_construct_array_item;
}

do_construct_array_item: {
  assert(!levels.empty());
  assert(levels.back() == Container::Array);
  const auto &item_entry{tape[tape_index]};

  if (item_entry.type == TapeType::ArrayStart) {
    CALLBACK_PRE(JSON::Type::Array, item_entry, JSON::ParseContext::Index,
                 frames.back().get().size(), empty_property);
    goto do_construct_array;
  }

  if (item_entry.type == TapeType::ObjectStart) {
    CALLBACK_PRE(JSON::Type::Object, item_entry, JSON::ParseContext::Index,
                 frames.back().get().size(), empty_property);
    goto do_construct_object;
  }

  const auto item_index{frames.back().get().size()};
  auto item_value{internal::construct_leaf(buffer, item_entry)};
  const auto item_type{item_value.type()};
  CALLBACK_PRE(item_type, item_entry, JSON::ParseContext::Index, item_index,
               empty_property);
  frames.back().get().push_back(std::move(item_value));
  tape_index++;
  CALLBACK_POST(item_type, item_entry.line,
                internal::post_column_for(item_entry));
  goto do_construct_array_item_separator;
}

do_construct_array_item_separator:
  if (tape[tape_index].type == TapeType::ArrayEnd) {
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(JSON::Type::Array, end_entry.line, end_entry.column);
    goto do_construct_container_end;
  }

  goto do_construct_array_item;

  /*
   * Construct an object
   */

do_construct_object: {
  const auto &object_entry{tape[tape_index]};
  assert(object_entry.type == TapeType::ObjectStart);
  const auto property_count{object_entry.count};
  tape_index++;

  if (levels.empty()) {
    levels.push_back(Container::Object);
    output = Result::make_object();
    frames.emplace_back(output);
  } else if (levels.back() == Container::Array) {
    levels.push_back(Container::Object);
    frames.back().get().push_back(Result::make_object());
    frames.emplace_back(frames.back().get().back());
  } else if (levels.back() == Container::Object) {
    levels.push_back(Container::Object);
    auto &inserted{frames.back().get().assign_assume_new(
        std::move(key), Result::make_object(), key_hash)};
    if (callback) {
      callback(JSON::ParsePhase::Pre, JSON::Type::Object, key_line, key_column,
               JSON::ParseContext::Property, 0,
               frames.back().get().as_object().back_key());
    }
    frames.emplace_back(inserted);
  }

  frames.back().get().as_object().reserve(property_count);

  if (property_count == 0) {
    assert(tape[tape_index].type == TapeType::ObjectEnd);
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(JSON::Type::Object, end_entry.line, end_entry.column);
    goto do_construct_container_end;
  }

  goto do_construct_object_key;
}

do_construct_object_key: {
  assert(!levels.empty());
  assert(levels.back() == Container::Object);
  const auto &key_entry{tape[tape_index]};
  assert(key_entry.type == TapeType::Key);
  const char *key_data{buffer + key_entry.offset};
  const auto key_length{key_entry.length};
  if ((key_entry.flags & TAPE_FLAG_STRING_ESCAPE) != 0) {
    key = internal::unescape_string(key_data, key_length, true);
    key_hash = Result::Object::hash(key);
  } else {
    key.assign(key_data, key_length);
    key_hash = Result::Object::hash(key_data, key_length);
  }
  key_line = key_entry.line;
  key_column = key_entry.column;
  tape_index++;
  goto do_construct_object_value;
}

do_construct_object_value: {
  const auto &value_entry{tape[tape_index]};

  if (value_entry.type == TapeType::ArrayStart) {
    goto do_construct_array;
  }

  if (value_entry.type == TapeType::ObjectStart) {
    goto do_construct_object;
  }

  auto value{internal::construct_leaf(buffer, value_entry)};
  const auto value_type{value.type()};
  frames.back().get().assign_assume_new(std::move(key), std::move(value),
                                        key_hash);
  if (callback) {
    callback(JSON::ParsePhase::Pre, value_type, key_line, key_column,
             JSON::ParseContext::Property, 0,
             frames.back().get().as_object().back_key());
  }
  tape_index++;
  CALLBACK_POST(value_type, value_entry.line,
                internal::post_column_for(value_entry));
  goto do_construct_object_property_end;
}

do_construct_object_property_end:
  if (tape[tape_index].type == TapeType::ObjectEnd) {
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(JSON::Type::Object, end_entry.line, end_entry.column);
    goto do_construct_container_end;
  }

  goto do_construct_object_key;

  /*
   * Finish constructing a container
   */

do_construct_container_end:
  assert(!levels.empty());
  if (levels.size() == 1) {
    return;
  }

  frames.pop_back();
  levels.pop_back();
  if (levels.back() == Container::Array) {
    goto do_construct_array_item_separator;
  } else {
    goto do_construct_object_property_end;
  }
}

// NOLINTEND(cppcoreguidelines-avoid-goto,bugprone-use-after-move)

#undef CALLBACK_PRE
#undef CALLBACK_POST

} // namespace sourcemeta::core

#endif
