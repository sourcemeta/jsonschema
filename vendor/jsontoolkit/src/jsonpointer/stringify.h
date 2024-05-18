#ifndef SOURCEMETA_JSONTOOLKIT_JSONPOINTER_STRINGIFY_H_
#define SOURCEMETA_JSONTOOLKIT_JSONPOINTER_STRINGIFY_H_

#include "grammar.h"

#include <sourcemeta/jsontoolkit/jsonpointer_pointer.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <ostream> // std::basic_ostream
#include <sstream> // std::basic_istringstream
#include <string>  // std::to_string, std::basic_string

namespace {
template <typename CharT, typename Traits,
          template <typename T> typename Allocator>
static auto write_character(std::basic_ostream<CharT, Traits> &stream,
                            const CharT character,
                            const bool perform_uri_escaping) -> void {
  // The dollar sign does not need to be encoded in URI fragments
  // See `fragment` in https://www.rfc-editor.org/rfc/rfc3986#appendix-A
  if (perform_uri_escaping && character != '$') {
    // TODO: Implement a URI function capable of efficiently escaping a single
    // character to avoid this expensive ugliness
    std::basic_istringstream<CharT, Traits, Allocator<CharT>> input{
        std::basic_string<CharT, Traits, Allocator<CharT>>{character}};
    sourcemeta::jsontoolkit::URI::escape(input, stream);
  } else {
    stream.put(character);
  }
}
} // namespace

namespace sourcemeta::jsontoolkit {

template <typename CharT, typename Traits,
          template <typename T> typename Allocator>
auto stringify(const GenericPointer<CharT, Traits, Allocator> &pointer,
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
              write_character<CharT, Traits, Allocator>(
                  stream, internal::token_pointer_reverse_solidus<CharT>,
                  perform_uri_escaping);
            }

            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_quote<CharT>,
                perform_uri_escaping);
            break;
          case internal::token_pointer_reverse_solidus<CharT>:
            if (!perform_uri_escaping) {
              write_character<CharT, Traits, Allocator>(
                  stream, internal::token_pointer_reverse_solidus<CharT>,
                  perform_uri_escaping);
            }

            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            break;

          // See https://www.asciitable.com
          // See https://www.rfc-editor.org/rfc/rfc4627#section-2.5

          // Null
          case '\u0000':
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_backspace<CharT>);
            break;
          // Horizontal tab
          case '\u0009':
            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_tab<CharT>);
            break;
          // Line feed
          case '\u000A':
            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_line_feed<CharT>);
            break;
          // Vertical tab
          case '\u000B':
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_form_feed<CharT>);
            break;
          // Carriage return
          case '\u000D':
            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_carriage_return<CharT>);
            break;
          // Shift out
          case '\u000E':
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
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
            write_character<CharT, Traits, Allocator>(
                stream, internal::token_pointer_reverse_solidus<CharT>,
                perform_uri_escaping);
            stream.put(internal::token_pointer_escape_unicode<CharT>);
            stream.put('0');
            stream.put('0');
            stream.put('1');
            stream.put('F');
            break;
          default:
            write_character<CharT, Traits, Allocator>(stream, character,
                                                      perform_uri_escaping);
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

} // namespace sourcemeta::jsontoolkit

#endif
