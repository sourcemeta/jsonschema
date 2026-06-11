#ifndef SOURCEMETA_CORE_JSONPOINTER_PARSER_H_
#define SOURCEMETA_CORE_JSONPOINTER_PARSER_H_

#include "grammar.h"

#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonpointer_error.h>

#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint64_t
#include <istream>      // std::basic_istream
#include <sstream>      // std::basic_stringstream
#include <string>       // std::string
#include <system_error> // std::errc
#include <type_traits>  // std::conditional_t

namespace sourcemeta::core::internal {
template <typename CharT, typename Traits,
          template <typename T> typename Allocator>
inline auto
reset(std::basic_stringstream<CharT, Traits, Allocator<CharT>> &stream)
    -> void {
  stream.str("");
  stream.clear();
}

template <typename CharT, typename Traits,
          template <typename T> typename Allocator>
inline auto
parse_index(std::basic_stringstream<CharT, Traits, Allocator<CharT>> &stream,
            const std::uint64_t column) -> std::size_t {
  const auto input = stream.str();
  std::size_t index_value{};
  const auto result =
      std::from_chars(input.data(), input.data() + input.size(), index_value);
  if (result.ec != std::errc{}) [[unlikely]] {
    throw PointerParseError(column);
  }

  return index_value;
}

} // namespace sourcemeta::core::internal

// We use "goto" for performance reasons
// NOLINTBEGIN(cppcoreguidelines-avoid-goto)

namespace sourcemeta::core {
template <bool CheckOnly>
auto parse_pointer(std::basic_istream<JSON::Char, JSON::CharTraits> &stream)
    -> std::conditional_t<CheckOnly, void, Pointer> {
  [[maybe_unused]] Pointer result;
  JSON::Char character = 0;
  JSON::CharTraits::int_type code = 0;
  [[maybe_unused]] std::basic_stringstream<JSON::Char> string;
  std::uint64_t column{0};

parse_token_begin:
  code = stream.get();
  column += 1;
  if (JSON::CharTraits::eq_int_type(code, JSON::CharTraits::eof())) {
    goto done;
  }
  character = JSON::CharTraits::to_char_type(code);
  // A JSON Pointer is a Unicode string
  // containing a sequence of zero or more reference tokens, each prefixed
  // by a '/' (%x2F) character.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-3
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      goto parse_token_content;
    default:
      throw PointerParseError(column);
  }

parse_token_content:
  code = stream.peek();
  if (JSON::CharTraits::eq_int_type(code, JSON::CharTraits::eof())) {
    column += 1;
    stream.ignore();
    if constexpr (!CheckOnly) {
      result.emplace_back("");
    }
    goto done;
  }
  character = JSON::CharTraits::to_char_type(code);
  switch (character) {
      // Note that leading zeros are not allowed
      // See https://www.rfc-editor.org/rfc/rfc6901#section-4
    case internal::token_pointer_number_zero<JSON::Char>:
      column += 1;
      stream.ignore();
      goto parse_token_index_end;
    case internal::token_pointer_number_one<JSON::Char>:
    case internal::token_pointer_number_two<JSON::Char>:
    case internal::token_pointer_number_three<JSON::Char>:
    case internal::token_pointer_number_four<JSON::Char>:
    case internal::token_pointer_number_five<JSON::Char>:
    case internal::token_pointer_number_six<JSON::Char>:
    case internal::token_pointer_number_seven<JSON::Char>:
    case internal::token_pointer_number_eight<JSON::Char>:
    case internal::token_pointer_number_nine<JSON::Char>:
      column += 1;
      stream.ignore();
      if constexpr (!CheckOnly) {
        string.put(character);
      }
      goto parse_token_index_rest_any;
    case internal::token_pointer_slash<JSON::Char>:
      if constexpr (!CheckOnly) {
        result.emplace_back("");
      }
      goto parse_token_begin;
    case internal::token_pointer_tilde<JSON::Char>:
      column += 1;
      stream.ignore();
      goto parse_token_escape_tilde;
    default:
      column += 1;
      stream.ignore();
      if constexpr (!CheckOnly) {
        string.put(character);
      }
      goto parse_token_property_rest_any;
  }

  /*
   * Indexes
   */

parse_token_index_end:
  if constexpr (!CheckOnly) {
    string.put(character);
  }
  code = stream.peek();
  if (JSON::CharTraits::eq_int_type(code, JSON::CharTraits::eof())) {
    column += 1;
    stream.ignore();
    if constexpr (!CheckOnly) {
      result.emplace_back(internal::parse_index(string, column));
      internal::reset(string);
    }
    goto done;
  }
  character = JSON::CharTraits::to_char_type(code);
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      column += 1;
      stream.ignore();
      if constexpr (!CheckOnly) {
        result.emplace_back(internal::parse_index(string, column));
        internal::reset(string);
      }
      goto parse_token_content;
    default:
      goto parse_token_property_rest_any;
  }

parse_token_index_rest_any:
  code = stream.peek();
  if (JSON::CharTraits::eq_int_type(code, JSON::CharTraits::eof())) {
    column += 1;
    stream.ignore();
    if constexpr (!CheckOnly) {
      result.emplace_back(internal::parse_index(string, column));
      internal::reset(string);
    }
    goto done;
  }
  character = JSON::CharTraits::to_char_type(code);
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      column += 1;
      stream.ignore();
      if constexpr (!CheckOnly) {
        result.emplace_back(internal::parse_index(string, column));
        internal::reset(string);
      }
      goto parse_token_content;
    case internal::token_pointer_number_zero<JSON::Char>:
    case internal::token_pointer_number_one<JSON::Char>:
    case internal::token_pointer_number_two<JSON::Char>:
    case internal::token_pointer_number_three<JSON::Char>:
    case internal::token_pointer_number_four<JSON::Char>:
    case internal::token_pointer_number_five<JSON::Char>:
    case internal::token_pointer_number_six<JSON::Char>:
    case internal::token_pointer_number_seven<JSON::Char>:
    case internal::token_pointer_number_eight<JSON::Char>:
    case internal::token_pointer_number_nine<JSON::Char>:
      column += 1;
      stream.ignore();
      if constexpr (!CheckOnly) {
        string.put(character);
      }
      goto parse_token_index_rest_any;

    default:
      goto parse_token_property_rest_any;
  }

  /*
   * Properties
   */

parse_token_property_rest_any:
  code = stream.get();
  column += 1;
  if (JSON::CharTraits::eq_int_type(code, JSON::CharTraits::eof())) {
    if constexpr (!CheckOnly) {
      result.emplace_back(string.str());
      internal::reset(string);
    }
    goto done;
  }
  character = JSON::CharTraits::to_char_type(code);
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      if constexpr (!CheckOnly) {
        result.emplace_back(string.str());
        internal::reset(string);
      }
      goto parse_token_content;
    case internal::token_pointer_tilde<JSON::Char>:
      goto parse_token_escape_tilde;
    default:
      if constexpr (!CheckOnly) {
        string.put(character);
      }
      goto parse_token_property_rest_any;
  }

parse_token_escape_tilde:
  code = stream.get();
  column += 1;
  if (JSON::CharTraits::eq_int_type(code, JSON::CharTraits::eof())) {
    throw PointerParseError(column);
  }
  character = JSON::CharTraits::to_char_type(code);
  // Because the characters '~' (%x7E) and '/' (%x2F) have special
  // meanings in JSON Pointer, '~' needs to be encoded as '~0' and '/'
  // needs to be encoded as '~1' when these characters appear in a
  // reference token.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-3
  switch (character) {
    case internal::token_pointer_number_zero<JSON::Char>:
      if constexpr (!CheckOnly) {
        string.put(internal::token_pointer_tilde<JSON::Char>);
      }
      goto parse_token_property_rest_any;
    case internal::token_pointer_number_one<JSON::Char>:
      if constexpr (!CheckOnly) {
        string.put(internal::token_pointer_slash<JSON::Char>);
      }
      goto parse_token_property_rest_any;
    default:
      throw PointerParseError(column);
  }

done:
  if constexpr (!CheckOnly) {
    return result;
  }
}

// NOLINTEND(cppcoreguidelines-avoid-goto)

} // namespace sourcemeta::core

#endif
