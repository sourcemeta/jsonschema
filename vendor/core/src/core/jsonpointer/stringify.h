#ifndef SOURCEMETA_CORE_JSONPOINTER_STRINGIFY_H_
#define SOURCEMETA_CORE_JSONPOINTER_STRINGIFY_H_

#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/uri.h>

#include "grammar.h"

#include <array>    // std::array
#include <cassert>  // assert
#include <charconv> // std::to_chars
#include <ios>      // std::basic_ostream
#include <ostream>  // std::basic_ostream
#include <sstream>  // std::basic_istringstream
#include <string>   // std::basic_string
#include <variant>  // std::holds_alternative

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
      // Because the characters '~' (%x7E) and '/' (%x2F) have special
      // meanings in JSON Pointer, '~' needs to be encoded as '~0' and '/'
      // needs to be encoded as '~1' when these characters appear in a
      // reference token. Every other character is written verbatim.
      // See https://www.rfc-editor.org/rfc/rfc6901#section-3
      switch (character) {
        case internal::token_pointer_slash<CharT>:
          stream.put(internal::token_pointer_tilde<CharT>);
          stream.put(internal::token_pointer_one<CharT>);
          break;
        case internal::token_pointer_tilde<CharT>:
          stream.put(internal::token_pointer_tilde<CharT>);
          stream.put(internal::token_pointer_zero<CharT>);
          break;
        default:
          internal::write_character(stream, character);
      }
    }
  } else {
    std::array<char, 20> buffer{};
    const auto [end_pointer, error_code] = std::to_chars(
        buffer.data(), buffer.data() + buffer.size(), token.to_index());
    stream.write(
        buffer.data(),
        static_cast<typename std::basic_ostream<CharT, Traits>::int_type>(
            end_pointer - buffer.data()));
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
