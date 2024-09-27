#ifndef SOURCEMETA_JSONTOOLKIT_JSONPOINTER_PARSER_H_
#define SOURCEMETA_JSONTOOLKIT_JSONPOINTER_PARSER_H_

#include "grammar.h"

#include <sourcemeta/jsontoolkit/jsonpointer_error.h>
#include <sourcemeta/jsontoolkit/jsonpointer_pointer.h>

#include <istream>   // std::basic_istream
#include <sstream>   // std::basic_stringstream
#include <stdexcept> // std::out_of_range
#include <string>    // std::stoi

namespace sourcemeta::jsontoolkit::internal {
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
            const std::uint64_t column) -> decltype(auto) {
  try {
    return std::stoul(stream.str());
  } catch (const std::out_of_range &) {
    throw PointerParseError(column);
  }
}

} // namespace sourcemeta::jsontoolkit::internal

// We use "goto" for performance reasons
// NOLINTBEGIN(cppcoreguidelines-avoid-goto)

namespace sourcemeta::jsontoolkit {
auto parse_pointer(std::basic_istream<JSON::Char, JSON::CharTraits> &stream)
    -> Pointer {
  Pointer result;
  JSON::Char character;
  std::basic_stringstream<JSON::Char> string;
  std::uint64_t column{0};

parse_token_begin:
  character = static_cast<JSON::Char>(stream.get());
  column += 1;
  // A JSON Pointer is a Unicode string
  // containing a sequence of zero or more reference tokens, each prefixed
  // by a '/' (%x2F) character.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-3
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      goto parse_token_content;
    case static_cast<JSON::Char>(JSON::CharTraits::eof()):
      goto done;
    default:
      throw PointerParseError(column);
  }

parse_token_content:
  character = static_cast<JSON::Char>(stream.peek());
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
      string.put(character);
      goto parse_token_index_rest_any;
    case static_cast<JSON::Char>(JSON::CharTraits::eof()):
      column += 1;
      stream.ignore();
      result.emplace_back("");
      goto done;
    case internal::token_pointer_slash<JSON::Char>:
      result.emplace_back("");
      goto parse_token_begin;
    case internal::token_pointer_tilde<JSON::Char>:
      column += 1;
      stream.ignore();
      goto parse_token_escape_tilde;
    default:
      column += 1;
      stream.ignore();
      string.put(character);
      goto parse_token_property_rest_any;
  }

  /*
   * Indexes
   */

parse_token_index_end:
  string.put(character);
  character = static_cast<JSON::Char>(stream.peek());
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      column += 1;
      stream.ignore();
      result.emplace_back(internal::parse_index(string, column));
      internal::reset(string);
      goto parse_token_content;
    case static_cast<JSON::Char>(JSON::CharTraits::eof()):
      column += 1;
      stream.ignore();
      result.emplace_back(internal::parse_index(string, column));
      internal::reset(string);
      goto done;
    default:
      goto parse_token_property_rest_any;
  }

parse_token_index_rest_any:
  character = static_cast<JSON::Char>(stream.peek());
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      column += 1;
      stream.ignore();
      result.emplace_back(internal::parse_index(string, column));
      internal::reset(string);
      goto parse_token_content;
    case static_cast<JSON::Char>(JSON::CharTraits::eof()):
      column += 1;
      stream.ignore();
      result.emplace_back(internal::parse_index(string, column));
      internal::reset(string);
      goto done;
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
      string.put(character);
      goto parse_token_index_rest_any;

    default:
      goto parse_token_property_rest_any;
  }

  /*
   * Properties
   */

parse_token_property_rest_any:
  character = static_cast<JSON::Char>(stream.get());
  column += 1;
  switch (character) {
    case internal::token_pointer_slash<JSON::Char>:
      result.emplace_back(string.str());
      internal::reset(string);
      goto parse_token_content;
    case internal::token_pointer_tilde<JSON::Char>:
      goto parse_token_escape_tilde;
    case static_cast<JSON::Char>(JSON::CharTraits::eof()):
      result.emplace_back(string.str());
      internal::reset(string);
      goto done;
    default:
      string.put(character);
      goto parse_token_property_rest_any;
  }

parse_token_escape_tilde:
  character = static_cast<JSON::Char>(stream.get());
  column += 1;
  // Because the characters '~' (%x7E) and '/' (%x2F) have special
  // meanings in JSON Pointer, '~' needs to be encoded as '~0' and '/'
  // needs to be encoded as '~1' when these characters appear in a
  // reference token.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-3
  switch (character) {
    case internal::token_pointer_number_zero<JSON::Char>:
      string.put(internal::token_pointer_tilde<JSON::Char>);
      goto parse_token_property_rest_any;
    case internal::token_pointer_number_one<JSON::Char>:
      string.put(internal::token_pointer_slash<JSON::Char>);
      goto parse_token_property_rest_any;
    default:
      throw PointerParseError(column);
  }

done:
  return result;
}

// NOLINTEND(cppcoreguidelines-avoid-goto)

} // namespace sourcemeta::jsontoolkit

#endif
