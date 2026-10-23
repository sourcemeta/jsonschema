#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::all_of, std::find
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace {

// A key is either a plain identifier or a tenant and a system joined by an at
// sign, and the three differ in how long they may be and in what may start
// them: only a tenant admits a leading digit, where the other two require a
// lowercase letter
// https://www.w3.org/TR/trace-context/#key
auto key_part(const std::string_view identifier, const bool tenant,
              const std::size_t maximum) noexcept -> bool {
  if (identifier.empty() || identifier.size() > maximum ||
      !((identifier.front() >= 'a' && identifier.front() <= 'z') ||
        (tenant && sourcemeta::core::is_digit(identifier.front())))) {
    return false;
  }
  return std::all_of(
      identifier.begin() + 1, identifier.end(), [](const char character) {
        return (character >= 'a' && character <= 'z') ||
               sourcemeta::core::is_digit(character) || character == '_' ||
               character == '-' || character == '*' || character == '/';
      });
}

// "There can be a maximum of 32 list-members in a list", which bounds both the
// count and the keys kept to compare against
// https://www.w3.org/TR/trace-context/#tracestate-header-field-values
constexpr std::size_t MAXIMUM_MEMBERS{32};

} // namespace

namespace sourcemeta::core {

auto http_is_tracestate(const std::string_view value) noexcept -> bool {
  // A list holds at most 32 members, so the keys seen so far fit in a fixed
  // array that a linear scan searches, which keeps this free of allocation
  std::array<std::string_view, MAXIMUM_MEMBERS> keys;
  std::size_t seen{0};
  std::size_t members{0};
  auto rest{value};
  while (true) {
    // The limit counts the empty members too, since each is a list-member of
    // its own
    if (members >= MAXIMUM_MEMBERS) {
      return false;
    }
    const auto comma{rest.find(',')};

    // "Spaces and horizontal tabs surrounding list-members are ignored", so
    // they are stripped from every member wherever it sits, and what is left
    // of one made of nothing else is the empty member the next rule allows.
    // The list production brackets only its inner boundaries, so stripping at
    // the two outer ends accepts a little more than the grammar writes down.
    // That is deliberate and costs nothing, since "A field value does not
    // include leading or trailing whitespace" in the first place, and
    // stripping can never turn one well-formed member into another, because a
    // key cannot begin with a space or a tab and a value cannot end with one
    // https://www.w3.org/TR/trace-context/#tracestate-header-field-values
    // https://www.w3.org/TR/trace-context/#list
    // https://www.rfc-editor.org/rfc/rfc9110#section-5.5
    const auto entry{trim(rest.substr(0, comma), http_is_ows)};

    // "Empty and whitespace-only list members are allowed. Vendors MUST accept
    // empty tracestate headers but SHOULD avoid sending them", which is there
    // because "some vendors automatically inject whitespace after a comma
    // separator, even in the case of an empty header". Baggage has no such
    // alternative in its own member production, so a reader coming from that
    // parser should not expect this
    // https://www.w3.org/TR/trace-context/#tracestate-header-field-values
    // https://www.w3.org/TR/baggage/#definition
    if (!entry.empty()) {
      const auto equal{entry.find('=')};
      if (equal == std::string_view::npos) {
        return false;
      }
      const auto key{entry.substr(0, equal)};
      const auto content{entry.substr(equal + 1)};
      const auto at_position{key.find('@')};

      // A simple key is one character plus 255, a tenant 1 plus 240 and a
      // system 1 plus 13, which is where the three bounds come from
      // https://www.w3.org/TR/trace-context/#key
      const bool valid_key{
          at_position == std::string_view::npos
              ? key_part(key, false, 256)
              : key_part(key.substr(0, at_position), true, 241) &&
                    key_part(key.substr(at_position + 1), false, 14)};
      if (!valid_key) {
        return false;
      }

      // "Only one entry per key is allowed because the entry represents that
      // last position in the trace", which the grammar itself cannot say. The
      // obligation that follows it is addressed to a producer and is not in
      // capitals, so it is not a requirement in the sense this document
      // reserves for those: "vendors must overwrite their entry upon reentry
      // to their tracing system". A reader is offered the alternative of
      // dropping the repeated entry instead of the whole value, which this
      // declines so that a caller is told rather than quietly corrected, and
      // which is only offered in a section marked non-normative. Baggage is
      // the opposite, where "Uniqueness of keys between multiple list-members
      // in a baggage-string is not guaranteed"
      // https://www.w3.org/TR/trace-context/#combined-header-value
      // https://www.w3.org/TR/trace-context/#a-traceparent-is-received
      // https://www.w3.org/TR/baggage/#baggage-string
      if (std::find(keys.begin(), keys.begin() + seen, key) !=
          keys.begin() + seen) {
        return false;
      }
      keys[seen] = key;
      ++seen;

      // "The value is an opaque string containing up to 256 printable ASCII
      // [RFC0020] characters (i.e., the range 0x20 to 0x7E) except comma (,)
      // and (=)", and it ends in a mandatory non-blank character, which
      // stripping the surrounding whitespace has already settled. Baggage
      // instead counts zero or more of its own octets, so it permits an empty
      // value
      // https://www.w3.org/TR/trace-context/#value
      // https://www.w3.org/TR/baggage/#definition
      if (content.empty() || content.size() > 256 ||
          !std::all_of(content.begin(), content.end(),
                       [](const char character) {
                         const auto byte{static_cast<unsigned char>(character)};
                         return byte >= 0x20 && byte <= 0x7E &&
                                character != ',' && character != '=';
                       })) {
        return false;
      }
    }

    ++members;
    if (comma == std::string_view::npos) {
      return true;
    }
    rest.remove_prefix(comma + 1);
  }
}

} // namespace sourcemeta::core
