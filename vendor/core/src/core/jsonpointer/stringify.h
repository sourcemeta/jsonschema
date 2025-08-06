#ifndef SOURCEMETA_CORE_JSONPOINTER_STRINGIFY_H_
#define SOURCEMETA_CORE_JSONPOINTER_STRINGIFY_H_

#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/uri.h>

#include "grammar.h"

#include <cassert> // assert
#include <ios>     // std::basic_ostream
#include <ostream> // std::basic_ostream
#include <sstream> // std::basic_istringstream
#include <string>  // std::to_string, std::basic_string
#include <variant> // std::holds_alternative

namespace sourcemeta::core::internal {
inline auto
write_character(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::Char character) -> void {
  stream.put(character);
}
} // namespace sourcemeta::core::internal

namespace sourcemeta::core {

template <typename CharT, typename Traits,
          template <typename T> typename Allocator, typename TokenT>
auto stringify_token(const TokenT &token,
                     std::basic_ostream<CharT, Traits> &stream) -> void {
  // A JSON Pointer is a Unicode string (see [RFC4627], Section 3)
  // containing a sequence of zero or more reference tokens, each prefixed
  // by a '/' (%x2F) character.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-3
  stream.put(internal::token_pointer_slash<CharT>);
  if (token.is_property()) {
    for (const auto &character : token.to_property()) {
      switch (character) {
        // Because the characters '~' (%x7E) and '/' (%x2F) have special
        // meanings in JSON Pointer, '~' needs to be encoded as '~0' and '/'
        // needs to be encoded as '~1' when these characters appear in a
        // reference token.
        // See https://www.rfc-editor.org/rfc/rfc6901#section-3
        case internal::token_pointer_slash<CharT>:
          stream.put(internal::token_pointer_tilde<CharT>);
          stream.put(internal::token_pointer_one<CharT>);
          break;
        case internal::token_pointer_tilde<CharT>:
          stream.put(internal::token_pointer_tilde<CharT>);
          stream.put(internal::token_pointer_zero<CharT>);
          break;

        // All instances of quotation mark '"' (%x22), reverse solidus '\'
        // (%x5C), and control (%x00-1F) characters MUST be escaped. See
        // https://www.rfc-editor.org/rfc/rfc6901#section-5
        case internal::token_pointer_quote<CharT>:
          internal::write_character(stream,
                                    internal::token_pointer_quote<CharT>);
          break;
        case internal::token_pointer_reverse_solidus<CharT>:
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          break;

        // See https://www.asciitable.com
        // See https://www.rfc-editor.org/rfc/rfc4627#section-2.5

        // Null
        case '\u0000':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('0');
          break;
        // Start of heading
        case '\u0001':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('1');
          break;
        // Start of text
        case '\u0002':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('2');
          break;
        // End of text
        case '\u0003':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('3');
          break;
        // End of transmission
        case '\u0004':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('4');
          break;
        // Enquiry
        case '\u0005':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('5');
          break;
        // Acknowledge
        case '\u0006':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('6');
          break;
        // Bell
        case '\u0007':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('7');
          break;
        // Backspace
        case '\u0008':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_backspace<CharT>);
          break;
        // Horizontal tab
        case '\u0009':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_tab<CharT>);
          break;
        // Line feed
        case '\u000A':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_line_feed<CharT>);
          break;
        // Vertical tab
        case '\u000B':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('B');
          break;
        // Form feed
        case '\u000C':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_form_feed<CharT>);
          break;
        // Carriage return
        case '\u000D':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_carriage_return<CharT>);
          break;
        // Shift out
        case '\u000E':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('E');
          break;
        // Shift in
        case '\u000F':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('0');
          stream.put('F');
          break;
        // Data link escape
        case '\u0010':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('0');
          break;
        // Device control 1
        case '\u0011':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('1');
          break;
        // Device control 2
        case '\u0012':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('2');
          break;
        // Device control 3
        case '\u0013':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('3');
          break;
        // Device control 4
        case '\u0014':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('4');
          break;
        // Negative acknowledge
        case '\u0015':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('5');
          break;
        // Synchronous idle
        case '\u0016':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('6');
          break;
        // End of transmission block
        case '\u0017':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('7');
          break;
        // Cancel
        case '\u0018':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('8');
          break;
        // End of medium
        case '\u0019':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('9');
          break;
        // Substitute
        case '\u001A':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('A');
          break;
        // Escape
        case '\u001B':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('B');
          break;
        // File separator
        case '\u001C':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('C');
          break;
        // Group separator
        case '\u001D':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('D');
          break;
        // Record separator
        case '\u001E':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('E');
          break;
        // Unit separator
        case '\u001F':
          internal::write_character(
              stream, internal::token_pointer_reverse_solidus<CharT>);
          stream.put(internal::token_pointer_escape_unicode<CharT>);
          stream.put('0');
          stream.put('0');
          stream.put('1');
          stream.put('F');
          break;
        default:
          internal::write_character(stream, character);
      }
    }
  } else {
    const auto index{std::to_string(token.to_index())};
    stream.write(
        index.c_str(),
        static_cast<typename std::basic_ostream<CharT, Traits>::int_type>(
            index.size()));
  }
}

template <typename CharT, typename Traits,
          template <typename T> typename Allocator, typename PointerT>
auto stringify(const PointerT &pointer,
               std::basic_ostream<CharT, Traits> &stream) -> void {
  if constexpr (requires { typename PointerT::Wildcard; }) {
    for (const auto &token : pointer) {
      if (std::holds_alternative<typename PointerT::Wildcard>(token)) {
        // Represent wildcards using an impossible JSON Pointer token
        stream.put(internal::token_pointer_slash<CharT>);
        stream.put(internal::token_pointer_tilde<CharT>);

        switch (std::get<typename PointerT::Wildcard>(token)) {
          case PointerT::Wildcard::Property:
            stream.put('P');
            break;
          case PointerT::Wildcard::Item:
            stream.put('I');
            break;
          case PointerT::Wildcard::Key:
            stream.put('K');
            break;
          default:
            assert(false);
            break;
        }

        stream.put(internal::token_pointer_tilde<CharT>);
      } else if (std::holds_alternative<typename PointerT::Regex>(token)) {
        // Represent wildcards using an impossible JSON Pointer token
        stream.put(internal::token_pointer_slash<CharT>);
        stream.put(internal::token_pointer_tilde<CharT>);
        stream.put('R');
        const auto &value{std::get<typename PointerT::Regex>(token)};
        stream.write(value.c_str(), static_cast<std::streamsize>(value.size()));
        stream.put(internal::token_pointer_tilde<CharT>);
      } else if (std::holds_alternative<typename PointerT::Condition>(token)) {
        stream.put(internal::token_pointer_slash<CharT>);
        stream.put(internal::token_pointer_tilde<CharT>);
        stream.put('?');
        const auto &value{std::get<typename PointerT::Condition>(token)};
        if (value.suffix.has_value()) {
          stream.write(value.suffix->c_str(),
                       static_cast<std::streamsize>(value.suffix->size()));
        }

        stream.put(internal::token_pointer_tilde<CharT>);
      } else if (std::holds_alternative<typename PointerT::Negation>(token)) {
        stream.put(internal::token_pointer_slash<CharT>);
        stream.put(internal::token_pointer_tilde<CharT>);
        stream.put('!');
        stream.put(internal::token_pointer_tilde<CharT>);
      } else {
        stringify_token<CharT, Traits, Allocator, typename PointerT::Token>(
            std::get<typename PointerT::Token>(token), stream);
      }
    }
  } else {
    for (const auto &token : pointer) {
      stringify_token<CharT, Traits, Allocator, typename PointerT::Token>(
          token, stream);
    }
  }
}

} // namespace sourcemeta::core

#endif
