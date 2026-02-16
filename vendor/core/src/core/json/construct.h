#ifndef SOURCEMETA_CORE_JSON_CONSTRUCT_H_
#define SOURCEMETA_CORE_JSON_CONSTRUCT_H_

#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/json_value.h>

#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/unicode.h>

#include "parser.h"

#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <cstdint>    // std::uint64_t, std::uint32_t
#include <cstring>    // std::memchr
#include <functional> // std::reference_wrapper
#include <optional>   // std::optional
#include <stdexcept>  // std::invalid_argument
#include <utility>    // std::move
#include <vector>     // std::vector

namespace sourcemeta::core {

namespace internal {

inline auto unescape_string(const char *data, const std::uint32_t length) ->
    typename JSON::String {
  typename JSON::String result;
  const char *cursor{data};
  const char *string_end{data + length};

  if (!std::memchr(data, '\\', length)) {
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
        auto parse_hex4 = [](const char *&position) -> unsigned long {
          unsigned long value{0};
          for (std::size_t index = 0; index < 4; index++) {
            const char hex_char{*position++};
            unsigned long digit;
            if (hex_char >= '0' && hex_char <= '9') {
              digit = static_cast<unsigned long>(hex_char - '0');
            } else if (hex_char >= 'a' && hex_char <= 'f') {
              digit = static_cast<unsigned long>(hex_char - 'a') + 10;
            } else if (hex_char >= 'A' && hex_char <= 'F') {
              digit = static_cast<unsigned long>(hex_char - 'A') + 10;
            } else {
              digit = 0;
            }
            value = (value << 4) | digit;
          }
          return value;
        };

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
      default:
        break;
    }
  }

  return result;
}

inline auto construct_number(const char *data, const std::uint32_t length)
    -> JSON {
  const bool has_dot{std::memchr(data, '.', length) != nullptr};
  const bool has_exponent{std::memchr(data, 'e', length) != nullptr ||
                          std::memchr(data, 'E', length) != nullptr};

  if (has_exponent) {
    try {
      return JSON{Decimal{std::string_view{data, length}}};
    } catch (const DecimalParseError &) {
      throw JSONParseError(1, 1);
    } catch (const std::invalid_argument &) {
      throw JSONParseError(1, 1);
    }
  }

  if (has_dot) {
    std::size_t first_nonzero_position{JSON::String::npos};
    const auto decimal_position{static_cast<std::size_t>(
        static_cast<const char *>(std::memchr(data, '.', length)) - data)};
    for (std::size_t index = 0; index < length; index++) {
      if (index != decimal_position && data[index] != '0' &&
          data[index] != '-') {
        first_nonzero_position = index;
        break;
      }
    }

    if (first_nonzero_position == JSON::String::npos) {
      first_nonzero_position = 0;
    }

    const auto decimal_after_first_nonzero{decimal_position >
                                           first_nonzero_position};
    const auto significant_digits{length - first_nonzero_position -
                                  (decimal_after_first_nonzero ? 1 : 0)};
    constexpr std::size_t MAX_SAFE_SIGNIFICANT_DIGITS{15};
    if (significant_digits > MAX_SAFE_SIGNIFICANT_DIGITS) {
      try {
        return JSON{Decimal{std::string_view{data, length}}};
      } catch (const DecimalParseError &) {
        throw JSONParseError(1, 1);
      } catch (const std::invalid_argument &) {
        throw JSONParseError(1, 1);
      }
    }

    const typename JSON::String string_value{data, length};
    const auto double_result{sourcemeta::core::to_double(string_value)};
    if (double_result.has_value()) {
      return JSON{double_result.value()};
    }
    try {
      return JSON{Decimal{string_value}};
    } catch (const DecimalParseError &) {
      throw JSONParseError(1, 1);
    } catch (const std::invalid_argument &) {
      throw JSONParseError(1, 1);
    }
  }

  const typename JSON::String string_value{data, length};
  const auto int_result{sourcemeta::core::to_int64_t(string_value)};
  if (int_result.has_value()) {
    return JSON{int_result.value()};
  }
  try {
    return JSON{Decimal{string_value}};
  } catch (const DecimalParseError &) {
    throw JSONParseError(1, 1);
  } catch (const std::invalid_argument &) {
    throw JSONParseError(1, 1);
  }
}

inline auto post_column_for(const TapeEntry &entry) -> std::uint64_t {
  switch (entry.type) {
    case TapeType::True:
      return entry.column + 3;
    case TapeType::False:
      return entry.column + 4;
    case TapeType::Null:
      return entry.column + 3;
    case TapeType::String:
    case TapeType::Key:
      return entry.column + entry.length + 1;
    case TapeType::Number:
      return entry.column + entry.length - 1;
    default:
      return entry.column;
  }
}

} // namespace internal

// NOLINTBEGIN(cppcoreguidelines-avoid-goto,bugprone-use-after-move)

#define CALLBACK_PRE(value_type, entry_ref, context, index, property)          \
  if (callback) {                                                              \
    callback(JSON::ParsePhase::Pre, JSON::Type::value_type, (entry_ref).line,  \
             (entry_ref).column, context, index, property);                    \
  }

#define CALLBACK_POST(value_type, post_line, post_column)                      \
  if (callback) {                                                              \
    callback(JSON::ParsePhase::Post, JSON::Type::value_type, post_line,        \
             post_column, JSON::ParseContext::Root, 0, JSON::StringView{});    \
  }

inline auto construct_json(const char *buffer,
                           const std::vector<TapeEntry> &tape,
                           const JSON::ParseCallback &callback) -> JSON {
  using Result = JSON;
  enum class Container : std::uint8_t { Array, Object };
  std::vector<Container> levels;
  std::vector<std::reference_wrapper<Result>> frames;
  levels.reserve(32);
  frames.reserve(32);
  std::optional<Result> result;
  typename Result::String key;
  typename Result::Object::hash_type key_hash;
  std::uint64_t key_line{0};
  std::uint64_t key_column{0};
  std::size_t tape_index{0};

  if (tape.empty()) {
    throw JSONParseError(1, 1);
  }

  const auto &entry{tape[tape_index]};
  switch (entry.type) {
    case TapeType::True:
      CALLBACK_PRE(Boolean, entry, JSON::ParseContext::Root, 0,
                   JSON::StringView{});
      CALLBACK_POST(Boolean, entry.line, internal::post_column_for(entry));
      return JSON{true};
    case TapeType::False:
      CALLBACK_PRE(Boolean, entry, JSON::ParseContext::Root, 0,
                   JSON::StringView{});
      CALLBACK_POST(Boolean, entry.line, internal::post_column_for(entry));
      return JSON{false};
    case TapeType::Null:
      CALLBACK_PRE(Null, entry, JSON::ParseContext::Root, 0,
                   JSON::StringView{});
      CALLBACK_POST(Null, entry.line, internal::post_column_for(entry));
      return JSON{nullptr};
    case TapeType::String: {
      CALLBACK_PRE(String, entry, JSON::ParseContext::Root, 0,
                   JSON::StringView{});
      auto value{Result{
          internal::unescape_string(buffer + entry.offset, entry.length)}};
      CALLBACK_POST(String, entry.line, internal::post_column_for(entry));
      return value;
    }
    case TapeType::Number: {
      auto value{
          internal::construct_number(buffer + entry.offset, entry.length)};
      if (value.is_integer()) {
        CALLBACK_PRE(Integer, entry, JSON::ParseContext::Root, 0,
                     JSON::StringView{});
        CALLBACK_POST(Integer, entry.line, internal::post_column_for(entry));
      } else if (value.is_decimal()) {
        CALLBACK_PRE(Decimal, entry, JSON::ParseContext::Root, 0,
                     JSON::StringView{});
        CALLBACK_POST(Decimal, entry.line, internal::post_column_for(entry));
      } else {
        CALLBACK_PRE(Real, entry, JSON::ParseContext::Root, 0,
                     JSON::StringView{});
        CALLBACK_POST(Real, entry.line, internal::post_column_for(entry));
      }
      return value;
    }
    case TapeType::ArrayStart:
      CALLBACK_PRE(Array, entry, JSON::ParseContext::Root, 0,
                   JSON::StringView{});
      goto do_construct_array;
    case TapeType::ObjectStart:
      CALLBACK_PRE(Object, entry, JSON::ParseContext::Root, 0,
                   JSON::StringView{});
      goto do_construct_object;
    default:
      throw JSONParseError(1, 1);
  }

  /*
   * Construct an array
   */

do_construct_array: {
  const auto &array_entry{tape[tape_index]};
  assert(array_entry.type == TapeType::ArrayStart);
  const auto child_count{array_entry.count};
  tape_index++;

  if (levels.empty()) {
    assert(!result.has_value());
    levels.push_back(Container::Array);
    result = std::make_optional<Result>(Result::make_array());
    frames.emplace_back(result.value());
  } else if (levels.back() == Container::Array) {
    levels.push_back(Container::Array);
    frames.back().get().push_back(Result::make_array());
    frames.emplace_back(frames.back().get().back());
  } else if (levels.back() == Container::Object) {
    levels.push_back(Container::Array);
    frames.back().get().assign(key, Result::make_array());
    frames.emplace_back(frames.back().get().at(key));
  }

  frames.back().get().as_array().reserve(child_count);

  if (child_count == 0) {
    assert(tape[tape_index].type == TapeType::ArrayEnd);
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(Array, end_entry.line, end_entry.column);
    goto do_construct_container_end;
  }

  goto do_construct_array_item;
}

do_construct_array_item: {
  assert(!levels.empty());
  assert(levels.back() == Container::Array);
  const auto &item_entry{tape[tape_index]};

  switch (item_entry.type) {
    case TapeType::ArrayStart:
      CALLBACK_PRE(Array, item_entry, JSON::ParseContext::Index,
                   frames.back().get().size(), JSON::StringView{});
      goto do_construct_array;
    case TapeType::ObjectStart:
      CALLBACK_PRE(Object, item_entry, JSON::ParseContext::Index,
                   frames.back().get().size(), JSON::StringView{});
      goto do_construct_object;
    case TapeType::True:
      CALLBACK_PRE(Boolean, item_entry, JSON::ParseContext::Index,
                   frames.back().get().size(), JSON::StringView{});
      frames.back().get().push_back(JSON{true});
      tape_index++;
      CALLBACK_POST(Boolean, item_entry.line,
                    internal::post_column_for(item_entry));
      goto do_construct_array_item_separator;
    case TapeType::False:
      CALLBACK_PRE(Boolean, item_entry, JSON::ParseContext::Index,
                   frames.back().get().size(), JSON::StringView{});
      frames.back().get().push_back(JSON{false});
      tape_index++;
      CALLBACK_POST(Boolean, item_entry.line,
                    internal::post_column_for(item_entry));
      goto do_construct_array_item_separator;
    case TapeType::Null:
      CALLBACK_PRE(Null, item_entry, JSON::ParseContext::Index,
                   frames.back().get().size(), JSON::StringView{});
      frames.back().get().push_back(JSON{nullptr});
      tape_index++;
      CALLBACK_POST(Null, item_entry.line,
                    internal::post_column_for(item_entry));
      goto do_construct_array_item_separator;
    case TapeType::String:
      CALLBACK_PRE(String, item_entry, JSON::ParseContext::Index,
                   frames.back().get().size(), JSON::StringView{});
      frames.back().get().push_back(Result{internal::unescape_string(
          buffer + item_entry.offset, item_entry.length)});
      tape_index++;
      CALLBACK_POST(String, item_entry.line,
                    internal::post_column_for(item_entry));
      goto do_construct_array_item_separator;
    case TapeType::Number: {
      const auto current_index{frames.back().get().size()};
      auto value{internal::construct_number(buffer + item_entry.offset,
                                            item_entry.length)};
      if (value.is_integer()) {
        CALLBACK_PRE(Integer, item_entry, JSON::ParseContext::Index,
                     current_index, JSON::StringView{});
      } else if (value.is_decimal()) {
        CALLBACK_PRE(Decimal, item_entry, JSON::ParseContext::Index,
                     current_index, JSON::StringView{});
      } else {
        CALLBACK_PRE(Real, item_entry, JSON::ParseContext::Index, current_index,
                     JSON::StringView{});
      }
      const auto value_type{value.type()};
      frames.back().get().push_back(std::move(value));
      tape_index++;
      if (value_type == JSON::Type::Integer) {
        CALLBACK_POST(Integer, item_entry.line,
                      internal::post_column_for(item_entry));
      } else if (value_type == JSON::Type::Decimal) {
        CALLBACK_POST(Decimal, item_entry.line,
                      internal::post_column_for(item_entry));
      } else {
        CALLBACK_POST(Real, item_entry.line,
                      internal::post_column_for(item_entry));
      }
      goto do_construct_array_item_separator;
    }
    default:
      throw JSONParseError(1, 1);
  }
}

do_construct_array_item_separator:
  if (tape[tape_index].type == TapeType::ArrayEnd) {
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(Array, end_entry.line, end_entry.column);
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
    assert(!result.has_value());
    levels.push_back(Container::Object);
    result = std::make_optional<Result>(Result::make_object());
    frames.emplace_back(result.value());
  } else if (levels.back() == Container::Array) {
    levels.push_back(Container::Object);
    frames.back().get().push_back(Result::make_object());
    frames.emplace_back(frames.back().get().back());
  } else if (levels.back() == Container::Object) {
    levels.push_back(Container::Object);
    frames.back().get().assign(key, Result::make_object());
    frames.emplace_back(frames.back().get().at(key));
  }

  frames.back().get().as_object().reserve(property_count);

  if (property_count == 0) {
    assert(tape[tape_index].type == TapeType::ObjectEnd);
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(Object, end_entry.line, end_entry.column);
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
  if (std::memchr(key_data, '\\', key_length)) {
    key = internal::unescape_string(key_data, key_length);
    key_hash = frames.back().get().as_object().hash(key);
  } else {
    key.assign(key_data, key_length);
    key_hash = frames.back().get().as_object().hash(key_data, key_length);
  }
  key_line = key_entry.line;
  key_column = key_entry.column;
  tape_index++;
  goto do_construct_object_value;
}

do_construct_object_value: {
  const auto &value_entry{tape[tape_index]};

  switch (value_entry.type) {
    case TapeType::ArrayStart:
      if (callback) {
        callback(JSON::ParsePhase::Pre, JSON::Type::Array, key_line, key_column,
                 JSON::ParseContext::Property, 0, key);
      }
      goto do_construct_array;
    case TapeType::ObjectStart:
      if (callback) {
        callback(JSON::ParsePhase::Pre, JSON::Type::Object, key_line,
                 key_column, JSON::ParseContext::Property, 0, key);
      }
      goto do_construct_object;
    case TapeType::True:
      if (callback) {
        callback(JSON::ParsePhase::Pre, JSON::Type::Boolean, key_line,
                 key_column, JSON::ParseContext::Property, 0, key);
      }
      frames.back().get().assign_assume_new(std::move(key), JSON{true},
                                            key_hash);
      tape_index++;
      CALLBACK_POST(Boolean, value_entry.line,
                    internal::post_column_for(value_entry));
      goto do_construct_object_property_end;
    case TapeType::False:
      if (callback) {
        callback(JSON::ParsePhase::Pre, JSON::Type::Boolean, key_line,
                 key_column, JSON::ParseContext::Property, 0, key);
      }
      frames.back().get().assign_assume_new(std::move(key), JSON{false},
                                            key_hash);
      tape_index++;
      CALLBACK_POST(Boolean, value_entry.line,
                    internal::post_column_for(value_entry));
      goto do_construct_object_property_end;
    case TapeType::Null:
      if (callback) {
        callback(JSON::ParsePhase::Pre, JSON::Type::Null, key_line, key_column,
                 JSON::ParseContext::Property, 0, key);
      }
      frames.back().get().assign_assume_new(std::move(key), JSON{nullptr},
                                            key_hash);
      tape_index++;
      CALLBACK_POST(Null, value_entry.line,
                    internal::post_column_for(value_entry));
      goto do_construct_object_property_end;
    case TapeType::String:
      if (callback) {
        callback(JSON::ParsePhase::Pre, JSON::Type::String, key_line,
                 key_column, JSON::ParseContext::Property, 0, key);
      }
      frames.back().get().assign_assume_new(
          std::move(key),
          Result{internal::unescape_string(buffer + value_entry.offset,
                                           value_entry.length)},
          key_hash);
      tape_index++;
      CALLBACK_POST(String, value_entry.line,
                    internal::post_column_for(value_entry));
      goto do_construct_object_property_end;
    case TapeType::Number: {
      auto value{internal::construct_number(buffer + value_entry.offset,
                                            value_entry.length)};
      const auto value_type{value.type()};
      if (value_type == JSON::Type::Integer) {
        if (callback) {
          callback(JSON::ParsePhase::Pre, JSON::Type::Integer, key_line,
                   key_column, JSON::ParseContext::Property, 0, key);
        }
      } else if (value_type == JSON::Type::Decimal) {
        if (callback) {
          callback(JSON::ParsePhase::Pre, JSON::Type::Decimal, key_line,
                   key_column, JSON::ParseContext::Property, 0, key);
        }
      } else {
        if (callback) {
          callback(JSON::ParsePhase::Pre, JSON::Type::Real, key_line,
                   key_column, JSON::ParseContext::Property, 0, key);
        }
      }
      frames.back().get().assign_assume_new(std::move(key), std::move(value),
                                            key_hash);
      tape_index++;
      if (value_type == JSON::Type::Integer) {
        CALLBACK_POST(Integer, value_entry.line,
                      internal::post_column_for(value_entry));
      } else if (value_type == JSON::Type::Decimal) {
        CALLBACK_POST(Decimal, value_entry.line,
                      internal::post_column_for(value_entry));
      } else {
        CALLBACK_POST(Real, value_entry.line,
                      internal::post_column_for(value_entry));
      }
      goto do_construct_object_property_end;
    }
    default:
      throw JSONParseError(1, 1);
  }
}

do_construct_object_property_end:
  if (tape[tape_index].type == TapeType::ObjectEnd) {
    const auto &end_entry{tape[tape_index]};
    tape_index++;
    CALLBACK_POST(Object, end_entry.line, end_entry.column);
    goto do_construct_container_end;
  }

  goto do_construct_object_key;

  /*
   * Finish constructing a container
   */

do_construct_container_end:
  assert(!levels.empty());
  if (levels.size() == 1) {
    return result.value();
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
