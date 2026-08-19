#ifndef SOURCEMETA_CORE_HTTP_HELPERS_H_
#define SOURCEMETA_CORE_HTTP_HELPERS_H_

#include <sourcemeta/core/http_syntax.h>
#include <sourcemeta/core/text.h>

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <string_view> // std::string_view
#include <utility>     // std::pair

// Bounds are validated by surrounding logic.
// NOLINTBEGIN(bugprone-suspicious-stringview-data-usage)
namespace sourcemeta::core {

inline auto http_subview(const std::string_view value, const std::size_t offset,
                         const std::size_t length) noexcept
    -> std::string_view {
  return std::string_view{value.data() + offset, length};
}

inline auto http_media_specificity(const std::string_view range,
                                   const std::string_view candidate) noexcept
    -> std::uint8_t {
  if (equals_ignore_case(range, candidate)) {
    return 3;
  }
  if (range == "*/*") {
    return 1;
  }
  const auto range_slash{range.find('/')};
  if (range_slash == std::string_view::npos) {
    return 0;
  }
  // Every caller validates its candidate media types upfront
  const auto candidate_slash{candidate.find('/')};
  assert(candidate_slash != std::string_view::npos);
  if (range.size() - range_slash != 2 || range[range_slash + 1] != '*') {
    return 0;
  }
  if (range_slash != candidate_slash) {
    return 0;
  }
  if (!equals_ignore_case(http_subview(range, 0, range_slash),
                          http_subview(candidate, 0, range_slash))) {
    return 0;
  }
  return 2;
}

template <typename Visitor>
inline auto http_for_each_list_entry(const std::string_view header,
                                     Visitor visit) -> void {
  std::size_t position{0};
  while (position < header.size()) {
    while (position < header.size() && http_is_ows(header[position])) {
      ++position;
    }
    std::size_t end_position{position};
    bool in_quotes{false};
    while (end_position < header.size()) {
      const char current{header[end_position]};
      if (in_quotes) {
        if (current == '\\' && end_position + 1 < header.size()) {
          ++end_position;
        } else if (current == '"') {
          in_quotes = false;
        }
      } else if (current == '"') {
        in_quotes = true;
      } else if (current == ',') {
        break;
      }
      ++end_position;
    }
    std::size_t entry_end{end_position};
    while (entry_end > position && http_is_ows(header[entry_end - 1])) {
      --entry_end;
    }
    if (position < entry_end) {
      visit(http_subview(header, position, entry_end - position));
    }
    position = (end_position < header.size()) ? end_position + 1 : end_position;
  }
}

inline auto http_split_entry(const std::string_view entry) noexcept
    -> std::pair<std::string_view, std::string_view> {
  std::size_t semicolon{0};
  bool in_quotes{false};
  // RFC 9110 §8.8.3: "etagc = %x21 / %x23-7E / obs-text", so a semicolon
  // inside a double-quoted value is content and must not split the entry from
  // its parameters
  while (semicolon < entry.size()) {
    const char current{entry[semicolon]};
    if (in_quotes) {
      if (current == '\\' && semicolon + 1 < entry.size()) {
        ++semicolon;
      } else if (current == '"') {
        in_quotes = false;
      }
    } else if (current == '"') {
      in_quotes = true;
    } else if (current == ';') {
      break;
    }
    ++semicolon;
  }
  return {http_trim_trailing_ows(http_subview(entry, 0, semicolon)),
          http_subview(entry, semicolon, entry.size() - semicolon)};
}

template <typename Visitor>
inline auto http_for_each_parameter(const std::string_view parameters,
                                    Visitor visit) -> void {
  std::size_t position{0};
  while (position < parameters.size()) {
    if (parameters[position] == ';') {
      ++position;
    }
    while (position < parameters.size() && http_is_ows(parameters[position])) {
      ++position;
    }
    std::size_t end_position{position};
    bool in_quotes{false};
    while (end_position < parameters.size()) {
      const char current{parameters[end_position]};
      if (in_quotes) {
        if (current == '\\' && end_position + 1 < parameters.size()) {
          ++end_position;
        } else if (current == '"') {
          in_quotes = false;
        }
      } else if (current == '"') {
        in_quotes = true;
      } else if (current == ';') {
        break;
      }
      ++end_position;
    }
    const auto raw{http_subview(parameters, position, end_position - position)};
    position = end_position;
    if (raw.empty()) {
      continue;
    }
    std::size_t equals{0};
    while (equals < raw.size() && raw[equals] != '=') {
      ++equals;
    }
    if (equals == raw.size()) {
      visit(http_trim_trailing_ows(raw), std::string_view{});
    } else {
      visit(http_trim_trailing_ows(http_subview(raw, 0, equals)),
            http_trim_trailing_ows(
                http_subview(raw, equals + 1, raw.size() - equals - 1)));
    }
  }
}

// RFC 9110 §5.6.4: a parameter value is either a token or a quoted-string, and
// §5.6.6 states "The quoted and unquoted values are equivalent". So the decoded
// content is compared rather than the raw syntax, treating charset="utf-8" and
// charset=utf-8 as the same value. Inside a quoted-string a quoted-pair carries
// only its second octet. RFC 2978 §2.3: charset names "are case-insensitive",
// so that one parameter folds case, while all others compare octet for octet
// because their case sensitivity depends on the parameter semantics.
inline auto http_parameter_value_equal(const std::string_view name,
                                       std::string_view left,
                                       std::string_view right) noexcept
    -> bool {
  const bool fold_case{equals_ignore_case(name, "charset")};
  const bool left_quoted{left.size() >= 2 && left.front() == '"' &&
                         left.back() == '"'};
  const bool right_quoted{right.size() >= 2 && right.front() == '"' &&
                          right.back() == '"'};
  if (left_quoted) {
    left = http_subview(left, 1, left.size() - 2);
  }
  if (right_quoted) {
    right = http_subview(right, 1, right.size() - 2);
  }
  std::size_t left_index{0};
  std::size_t right_index{0};
  while (left_index < left.size() && right_index < right.size()) {
    char left_character{left[left_index]};
    if (left_quoted && left_character == '\\' && left_index + 1 < left.size()) {
      left_character = left[++left_index];
    }
    char right_character{right[right_index]};
    if (right_quoted && right_character == '\\' &&
        right_index + 1 < right.size()) {
      right_character = right[++right_index];
    }
    if (fold_case) {
      left_character = to_lowercase(left_character);
      right_character = to_lowercase(right_character);
    }
    if (left_character != right_character) {
      return false;
    }
    ++left_index;
    ++right_index;
  }
  return left_index == left.size() && right_index == right.size();
}

// RFC 9110 §12.5.1 lets a media range carry media-type parameters, and "a
// parameter value that matches the [media-range] parameter" is required for
// the range to apply. A named parameter of the range is satisfied only when the
// candidate media type carries the same parameter. Parameter names are
// case-insensitive per RFC 9110 §5.6.6, while values are matched by their
// decoded content.
inline auto
http_candidate_has_parameter(const std::string_view candidate_parameters,
                             const std::string_view name,
                             const std::string_view value) noexcept -> bool {
  bool found{false};
  http_for_each_parameter(
      candidate_parameters,
      [&](const std::string_view candidate_name,
          const std::string_view candidate_value) noexcept -> void {
        if (equals_ignore_case(candidate_name, name) &&
            http_parameter_value_equal(name, candidate_value, value)) {
          found = true;
        }
      });
  return found;
}

// RFC 9110 §12.5.1: "Media ranges can be overridden by more specific media
// ranges or specific media types. If more than one media range applies to a
// given type, the most specific reference has precedence." The base tier ranks
// */* below type/* below type/subtype, and each media-type parameter the range
// pins that the candidate also carries makes the range strictly more specific.
// A range that pins a parameter the candidate lacks does not match at all.
inline auto http_media_range_specificity(
    const std::string_view range, const std::string_view range_parameters,
    const std::string_view candidate,
    const std::string_view candidate_parameters) noexcept -> std::uint8_t {
  const std::uint8_t base{http_media_specificity(range, candidate)};
  if (base == 0) {
    return 0;
  }
  std::uint8_t matched{0};
  bool all_present{true};
  http_for_each_parameter(
      range_parameters,
      [&](const std::string_view name,
          const std::string_view value) noexcept -> void {
        if (http_candidate_has_parameter(candidate_parameters, name, value)) {
          ++matched;
        } else {
          all_present = false;
        }
      });
  if (!all_present) {
    return 0;
  }
  return static_cast<std::uint8_t>(base + matched);
}

// RFC 9110 §12.4.2 q-value. A malformed weight is a fail-safe refusal, so it
// is treated as 0 rather than maximal preference. An absent weight is not
// routed here and keeps its 1.0 default at the call site.
inline auto http_parse_qvalue(const std::string_view value) noexcept -> float {
  if (value.empty()) {
    return 0.0F;
  }
  if (value[0] != '0' && value[0] != '1') {
    return 0.0F;
  }
  const float integer_part{static_cast<float>(value[0] - '0')};
  if (value.size() == 1) {
    return integer_part;
  }
  if (value[1] != '.' || value.size() > 5) {
    return 0.0F;
  }
  std::uint16_t numerator{0};
  std::uint16_t denominator{1};
  for (std::size_t index{2}; index < value.size(); ++index) {
    const char character{value[index]};
    if (character < '0' || character > '9') {
      return 0.0F;
    }
    numerator =
        static_cast<std::uint16_t>((numerator * 10) + (character - '0'));
    denominator = static_cast<std::uint16_t>(denominator * 10);
  }
  const float fraction{static_cast<float>(numerator) /
                       static_cast<float>(denominator)};
  const float result{integer_part + fraction};
  if (result > 1.0F) {
    return 0.0F;
  }
  return result;
}

inline auto http_extract_quality(const std::string_view parameters) noexcept
    -> float {
  float quality{1.0F};
  http_for_each_parameter(
      parameters,
      [&quality](const std::string_view name,
                 const std::string_view value) noexcept -> void {
        if (name.size() == 1 && (name[0] == 'q' || name[0] == 'Q')) {
          quality = http_parse_qvalue(value);
        }
      });
  return quality;
}

// RFC 9110 §12.5.1: "media-range = ( "*/*" / ( type "/" "*" ) / ( type "/"
// subtype ) ) parameters" followed by an optional "weight". The "q" parameter
// both carries the weight and closes the media-type parameter list, so every
// parameter before "q" is a media-type parameter that participates in matching,
// while "q" and any accept-ext after it do not. This returns the media-type
// parameter span (the prefix up to but excluding the "q" parameter) together
// with the parsed weight, which defaults to 1.0 when no "q" is present.
inline auto http_split_media_range(const std::string_view parameters) noexcept
    -> std::pair<std::string_view, float> {
  std::size_t position{0};
  std::size_t media_parameters_end{parameters.size()};
  float quality{1.0F};
  while (position < parameters.size()) {
    const std::size_t separator{position};
    if (parameters[position] == ';') {
      ++position;
    }
    while (position < parameters.size() && http_is_ows(parameters[position])) {
      ++position;
    }
    std::size_t end_position{position};
    bool in_quotes{false};
    while (end_position < parameters.size()) {
      const char current{parameters[end_position]};
      if (in_quotes) {
        if (current == '\\' && end_position + 1 < parameters.size()) {
          ++end_position;
        } else if (current == '"') {
          in_quotes = false;
        }
      } else if (current == '"') {
        in_quotes = true;
      } else if (current == ';') {
        break;
      }
      ++end_position;
    }
    const auto raw{http_subview(parameters, position, end_position - position)};
    position = end_position;
    if (raw.empty()) {
      continue;
    }
    std::size_t equals{0};
    while (equals < raw.size() && raw[equals] != '=') {
      ++equals;
    }
    const auto name{http_trim_trailing_ows(http_subview(raw, 0, equals))};
    if (name.size() == 1 && (name[0] == 'q' || name[0] == 'Q')) {
      media_parameters_end = separator;
      const auto value{(equals == raw.size())
                           ? std::string_view{}
                           : http_trim_trailing_ows(http_subview(
                                 raw, equals + 1, raw.size() - equals - 1))};
      quality = http_parse_qvalue(value);
      break;
    }
  }
  return {http_subview(parameters, 0, media_parameters_end), quality};
}

template <typename Visitor>
inline auto http_for_each_accept_entry(const std::string_view header,
                                       Visitor visit) -> void {
  http_for_each_list_entry(
      header, [&visit](const std::string_view entry) -> auto {
        const auto [value, parameters] = http_split_entry(entry);
        if (!value.empty()) {
          visit(value, http_extract_quality(parameters));
        }
      });
}

// RFC 9110 §12.5.1 media ranges, exposing the media type, its media-type
// parameters, and the weight separately so a parameterized range is matched
// against a candidate's own parameters rather than being collapsed to its type.
template <typename Visitor>
inline auto http_for_each_media_range(const std::string_view header,
                                      Visitor visit) -> void {
  http_for_each_list_entry(
      header, [&visit](const std::string_view entry) -> auto {
        const auto [value, parameters] = http_split_entry(entry);
        if (value.empty()) {
          return;
        }
        const auto [media_parameters, quality] =
            http_split_media_range(parameters);
        visit(value, media_parameters, quality);
      });
}

template <typename Visitor>
inline auto http_for_each_field_value(const std::string_view header,
                                      Visitor visit) -> void {
  http_for_each_list_entry(
      header, [&visit](const std::string_view entry) -> auto {
        const auto [value, parameters] = http_split_entry(entry);
        (void)parameters;
        if (!value.empty()) {
          visit(value);
        }
      });
}

} // namespace sourcemeta::core
// NOLINTEND(bugprone-suspicious-stringview-data-usage)

#endif
