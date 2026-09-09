#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

// RFC 9110 §5.6.6: "parameters = *( OWS ";" OWS [ parameter ] )", where
// "parameter = parameter-name "=" parameter-value", the name is a token, and
// the value is "( token / quoted-string )". That section also notes that
// parameters "do not allow whitespace (not even "bad" whitespace) around the
// "=" character"
auto http_is_parameters(const std::string_view parameters) -> bool {
  // A field value carries no leading or trailing whitespace of its own per
  // RFC 9110 §5.5, so the tail is trimmed before the grammar applies
  const auto span{http_trim_trailing_ows(parameters)};
  std::string storage;
  std::string_view value;
  std::size_t position{0};
  while (position < span.size()) {
    while (position < span.size() && http_is_ows(span[position])) {
      position += 1;
    }

    if (position >= span.size() || span[position] != ';') {
      return false;
    }

    position += 1;
    while (position < span.size() && http_is_ows(span[position])) {
      position += 1;
    }

    // The parameter of each repetition is optional, so a separator that stands
    // alone is well-formed
    if (position >= span.size() || span[position] == ';') {
      continue;
    }

    const auto name{position};
    while (position < span.size() && http_is_tchar(span[position])) {
      position += 1;
    }

    if (position == name || position >= span.size() || span[position] != '=') {
      return false;
    }

    position += 1;
    if (position < span.size() && span[position] == '"') {
      const auto end{http_scan_quoted_string(span, position, storage, value)};
      if (!end.has_value()) {
        return false;
      }

      position = end.value();
      continue;
    }

    const auto start{position};
    while (position < span.size() && http_is_tchar(span[position])) {
      position += 1;
    }

    if (position == start) {
      return false;
    }
  }

  return true;
}

} // namespace

auto http_parse_media_type(const std::string_view media_type)
    -> std::optional<HTTPMediaType> {
  const auto [value, parameters] = http_split_entry(media_type);
  const auto bare{http_trim_leading_ows(value)};
  if (!http_is_media_type(bare) || !http_is_parameters(parameters)) {
    return std::nullopt;
  }

  const auto slash{bare.find('/')};
  const auto subtype{http_subview(bare, slash + 1, bare.size() - slash - 1)};

  // RFC 6838 §4.2: "Characters after last plus always specify a structured
  // syntax suffix", so a trailing plus with nothing after it names none
  const auto plus{subtype.rfind('+')};
  const auto suffix{
      (plus == std::string_view::npos || plus + 1 == subtype.size())
          ? std::string_view{}
          : http_subview(subtype, plus, subtype.size() - plus)};

  return HTTPMediaType{.type = http_subview(bare, 0, slash),
                       .subtype = subtype,
                       .suffix = suffix,
                       .parameters = parameters};
}

} // namespace sourcemeta::core
