#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::all_of
#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

auto http_is_traceparent(const std::string_view value) noexcept -> bool {
  // The size test comes first so that the indexing below stays in range, as a
  // view offers no character past its end, and the dashes sit where the four
  // fields meet: "If the size of the header is shorter than 55 characters, the
  // vendor should not parse the header and should restart the trace". A value
  // carrying fields of a later version must put a dash where the flags end:
  // "Vendors MUST check that the 2 characters are either the end of the string
  // or a dash". The lowest version is the whole value, so it admits nothing
  // beyond the flags
  // https://www.w3.org/TR/trace-context/#versioning-of-traceparent
  //
  // What a later version appends past that dash is left uninterpreted, since
  // the rules for reading one enumerate three checks and no more, and "Vendors
  // MUST NOT parse or assume anything about unknown fields for this version".
  // Reading nothing into those bytes is not the same as taking any byte at
  // all, though: whatever they turn out to mean, they travel in a field value,
  // and the octets one may hold are settled below
  if (value.size() < 55 || value[2] != '-' || value[35] != '-' ||
      value[52] != '-' || (value.starts_with("00") && value.size() != 55) ||
      (value.size() > 55 && value[55] != '-')) {
    return false;
  }

  // "Version ff is invalid"
  // https://www.w3.org/TR/trace-context/#version
  if (value.starts_with("ff")) {
    return false;
  }

  // Every field is lowercase hexadecimal, including the version prefix, whose
  // failure to parse is what tells a reader to start a new trace:
  // "it's not 2 hex characters followed by a dash". The flags are read as a
  // bit field rather than compared, so no value of them is rejected here
  // https://www.w3.org/TR/trace-context/#traceparent-header-field-values
  for (std::size_t index{0}; index < 55; ++index) {
    if (index != 2 && index != 35 && index != 52 &&
        (!is_hex_digit(value[index]) || !is_lowercase(value[index]))) {
      return false;
    }
  }

  // Holding the appended bytes to the octets of a field value says nothing
  // about what they mean, so it leaves them as opaque as the rule above
  // demands. "Field values containing CR, LF, or NUL characters are invalid
  // and dangerous", and a recipient of one "MUST either reject the message or
  // replace each of those characters with SP before further processing", of
  // which refusing the value is the first
  // https://www.rfc-editor.org/rfc/rfc9110#section-5.5
  const auto appended{value.substr(55)};
  if (!std::all_of(appended.begin(), appended.end(), http_is_field_octet)) {
    return false;
  }

  // "If the trace-id value is invalid (for example if it contains non-allowed
  // characters or all zeros), vendors MUST ignore the traceparent", and the
  // same holds of the eight byte identifier
  // https://www.w3.org/TR/trace-context/#trace-id
  // https://www.w3.org/TR/trace-context/#parent-id
  return value.substr(3, 32).find_first_not_of('0') != std::string_view::npos &&
         value.substr(36, 16).find_first_not_of('0') != std::string_view::npos;
}

} // namespace sourcemeta::core
