#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_is_baggage(const std::string_view value) noexcept -> bool {
  std::size_t members{0};
  auto rest{value};
  while (true) {
    // The list production is one member followed by at most 179 more, so 180
    // is the cap. The separate figures of 64 members and 8192 bytes are the
    // point past which a platform "MAY drop list-members" rather than a bound
    // on what is well formed, so they are left alone
    // https://www.w3.org/TR/baggage/#definition
    // https://www.w3.org/TR/baggage/#limits
    if (members >= 180) {
      return false;
    }

    ++members;
    const auto comma{rest.find(',')};
    auto member{trim(rest.substr(0, comma), http_is_ows)};

    // A member carries its own pair and then any number of properties, and
    // only the pair must hold an equals sign, since "Additional metadata MAY
    // be appended to values in the form of property set" whose entries are
    // "keys and/or key-value pairs". The list production offers no alternative
    // for an empty member, so one is malformed, where tracestate allows one:
    // "Empty and whitespace-only list members are allowed"
    // https://www.w3.org/TR/baggage/#property
    // https://www.w3.org/TR/baggage/#definition
    // https://www.w3.org/TR/trace-context/#tracestate-header-field-values
    bool pair{true};
    while (true) {
      const auto semicolon{member.find(';')};
      const auto entry{trim(member.substr(0, semicolon), http_is_ows)};
      const auto equal{entry.find('=')};
      if (pair && equal == std::string_view::npos) {
        return false;
      }

      // "Leading and trailing whitespaces (OWS) are allowed and are not
      // considered to be a part of the key", which is itself the token this
      // module already knows. The list production brackets whitespace around
      // each key and value but not around the whole field value, so stripping
      // at the two outer ends accepts a little more than the grammar writes
      // down. It costs nothing, since "A field value does not include leading
      // or trailing whitespace" in the first place
      // https://www.w3.org/TR/baggage/#key
      // https://www.rfc-editor.org/rfc/rfc9110#section-5.5
      if (!http_is_token(trim(entry.substr(0, equal), http_is_ows))) {
        return false;
      }

      // Only the first equals sign separates, since a value may carry more of
      // them: "Parsers MUST NOT assume that the equal sign is only used to
      // separate key and value". An absent one is a property given as a bare
      // key, which the pair itself is not allowed to be
      // https://www.w3.org/TR/baggage/#value
      if (equal != std::string_view::npos &&
          !http_is_baggage_value(trim(entry.substr(equal + 1), http_is_ows))) {
        return false;
      }

      pair = false;
      if (semicolon == std::string_view::npos) {
        break;
      }

      member.remove_prefix(semicolon + 1);
    }

    if (comma == std::string_view::npos) {
      return true;
    }

    rest.remove_prefix(comma + 1);
  }
}

} // namespace sourcemeta::core
