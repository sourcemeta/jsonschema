#ifndef SOURCEMETA_CORE_JSONPOINTER_STRINGIFY_H_
#define SOURCEMETA_CORE_JSONPOINTER_STRINGIFY_H_

#include "grammar.h"

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer_pointer.h>
#include <sourcemeta/core/uri.h>

#include <ostream> // std::basic_ostream
#include <sstream> // std::basic_istringstream
#include <string>  // std::to_string, std::basic_string

namespace sourcemeta::core::internal {
inline auto
write_character(std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::Char character, const bool perform_uri_escaping)
    -> void {
  // The dollar sign does not need to be encoded in URI fragments
  // See `fragment` in https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  if (perform_uri_escaping && character != '$') {
    // TODO: Implement a URI function capable of efficiently escaping a single
    // character to avoid this expensive ugliness
    std::basic_istringstream<JSON::Char> input{
        std::basic_string<JSON::Char>{character}};
    sourcemeta::core::URI::escape(input, stream);
  } else {
    stream.put(character);
  }
}
} // namespace sourcemeta::core::internal

namespace sourcemeta::core {

template <typename CharT, typename Traits,
          template <typename T> typename Allocator, typename PointerT>
auto stringify(const PointerT &pointer,
               std::basic_ostream<CharT, Traits> &stream,
               const bool perform_uri_escaping) -> void {
  for (const auto &token : pointer) {
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
            if (!perform_uri_escaping) {
              internal::write_character(
                  stream, internal::token_pointer_reverse_solidus<CharT>,
                  perform_uri_escaping);
            }

            internal::write_character(stream,
                                      internal::token_pointer_quote<CharT>,
                                      perform_uri_escaping);
            break;
          case internal::token_pointer_reverse_solidus<CharT>:
            if (!perform_uri_escaping) {
              internal::write_character(
                  stream, internal::token_pointer_reverse_solidus<CharT>,
                  perform_uri_escaping);
            }

            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            break;

          // See https://www.asciitable.com
          // See https://www.rfc-editor.org/rfc/rfc4627#section-2.5

          // Null
          case '\u0000':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('0');
            break;
          // Start of heading
          case '\u0001':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('1');
            break;
          // Start of text
          case '\u0002':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('2');
            break;
          // End of text
          case '\u0003':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('3');
            break;
          // End of transmission
          case '\u0004':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('4');
            break;
          // Enquiry
          case '\u0005':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('5');
            break;
          // Acknowledge
          case '\u0006':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('6');
            break;
          // Bell
          case '\u0007':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('7');
            break;
          // Backspace
          case '\u0008':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_backspace<CharT>);
            break;
          // Horizontal tab
          case '\u0009':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_tab<CharT>);
            break;
          // Line feed
          case '\u000A':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_line_feed<CharT>);
            break;
          // Vertical tab
          case '\u000B':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('B');
            break;
          // Form feed
          case '\u000C':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_form_feed<CharT>);
            break;
          // Carriage return
          case '\u000D':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_carriage_return<CharT>);
            break;
          // Shift out
          case '\u000E':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('E');
            break;
          // Shift in
          case '\u000F':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('0');
            stream.put('F');
            break;
          // Data link escape
          case '\u0010':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('0');
            break;
          // Device control 1
          case '\u0011':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('1');
            break;
          // Device control 2
          case '\u0012':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('2');
            break;
          // Device control 3
          case '\u0013':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('3');
            break;
          // Device control 4
          case '\u0014':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('4');
            break;
          // Negative acknowledge
          case '\u0015':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('5');
            break;
          // Synchronous idle
          case '\u0016':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('6');
            break;
          // End of transmission block
          case '\u0017':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('7');
            break;
          // Cancel
          case '\u0018':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('8');
            break;
          // End of medium
          case '\u0019':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('9');
            break;
          // Substitute
          case '\u001A':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('A');
            break;
          // Escape
          case '\u001B':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('B');
            break;
          // File separator
          case '\u001C':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('C');
            break;
          // Group separator
          case '\u001D':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('D');
            break;
          // Record separator
          case '\u001E':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('E');
            break;
          // Unit separator
          case '\u001F':
            internal::write_character(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('F');
            break;
          default:
            internal::write_character(stream, character, perform_uri_escaping);
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
}

} // namespace sourcemeta::core

#endif
