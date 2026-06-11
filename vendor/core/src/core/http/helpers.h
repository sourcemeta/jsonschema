#ifndef SOURCEMETA_CORE_HTTP_HELPERS_H_
#define SOURCEMETA_CORE_HTTP_HELPERS_H_

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint16_t
#include <string_view> // std::string_view
#include <utility>     // std::pair

// Bounds are validated by surrounding logic.
// NOLINTBEGIN(bugprone-suspicious-stringview-data-usage)
namespace sourcemeta::core {

inline auto http_is_ows(const char character) noexcept -> bool {
  return character == ' ' || character == '\t';
}

inline auto http_ascii_lower(const char character) noexcept -> char {
  return (character >= 'A' && character <= 'Z')
             ? static_cast<char>(character + ('a' - 'A'))
             : character;
}

inline auto http_iequals_ascii(const std::string_view left,
                               const std::string_view right) noexcept -> bool {
  if (left.size() != right.size()) {
    return false;
  }
  for (std::size_t index{0}; index < left.size(); ++index) {
    if (http_ascii_lower(left[index]) != http_ascii_lower(right[index])) {
      return false;
    }
  }
  return true;
}

inline auto http_subview(const std::string_view value, const std::size_t offset,
                         const std::size_t length) noexcept
    -> std::string_view {
  return std::string_view{value.data() + offset, length};
}

inline auto http_media_specificity(const std::string_view range,
                                   const std::string_view candidate) noexcept
    -> std::uint8_t {
  if (http_iequals_ascii(range, candidate)) {
    return 3;
  }
  if (range == "*/*") {
    return 1;
  }
  const auto range_slash{range.find('/')};
  const auto candidate_slash{candidate.find('/')};
  if (range_slash == std::string_view::npos ||
      candidate_slash == std::string_view::npos) {
    return 0;
  }
  if (range.size() - range_slash != 2 || range[range_slash + 1] != '*') {
    return 0;
  }
  if (range_slash != candidate_slash) {
    return 0;
  }
  for (std::size_t index{0}; index < range_slash; ++index) {
    if (http_ascii_lower(range[index]) != http_ascii_lower(candidate[index])) {
      return 0;
    }
  }
  return 2;
}

inline auto http_trim_trailing_ows(const std::string_view value) noexcept
    -> std::string_view {
  std::size_t size{value.size()};
  while (size > 0 && http_is_ows(value[size - 1])) {
    --size;
  }
  return http_subview(value, 0, size);
}

inline auto http_trim_leading_ows(const std::string_view value) noexcept
    -> std::string_view {
  std::size_t position{0};
  while (position < value.size() && http_is_ows(value[position])) {
    ++position;
  }
  return http_subview(value, position, value.size() - position);
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
  while (semicolon < entry.size() && entry[semicolon] != ';') {
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

// RFC 9110 §12.4.2 q-value. A malformed weight is a fail-safe refusal, so it
// is treated as 0 rather than maximal preference. An absent weight is not
// routed here and keeps its 1.0 default at the call site.
inline auto http_parse_qvalue(const std::string_view value) noexcept -> float {
  if (value.empty()) {
    return 0.0f;
  }
  if (value[0] != '0' && value[0] != '1') {
    return 0.0f;
  }
  const float integer_part{static_cast<float>(value[0] - '0')};
  if (value.size() == 1) {
    return integer_part;
  }
  if (value[1] != '.' || value.size() > 5) {
    return 0.0f;
  }
  std::uint16_t numerator{0};
  std::uint16_t denominator{1};
  for (std::size_t index{2}; index < value.size(); ++index) {
    const char character{value[index]};
    if (character < '0' || character > '9') {
      return 0.0f;
    }
    numerator = static_cast<std::uint16_t>(numerator * 10 + (character - '0'));
    denominator = static_cast<std::uint16_t>(denominator * 10);
  }
  const float fraction{static_cast<float>(numerator) /
                       static_cast<float>(denominator)};
  const float result{integer_part + fraction};
  if (result > 1.0f) {
    return 0.0f;
  }
  return result;
}

inline auto http_extract_quality(const std::string_view parameters) noexcept
    -> float {
  float quality{1.0f};
  http_for_each_parameter(
      parameters, [&quality](const std::string_view name,
                             const std::string_view value) noexcept {
        if (name.size() == 1 && (name[0] == 'q' || name[0] == 'Q')) {
          quality = http_parse_qvalue(value);
        }
      });
  return quality;
}

template <typename Visitor>
inline auto http_for_each_accept_entry(const std::string_view header,
                                       Visitor visit) -> void {
  http_for_each_list_entry(header, [&visit](const std::string_view entry) {
    const auto [value, parameters] = http_split_entry(entry);
    if (!value.empty()) {
      visit(value, http_extract_quality(parameters));
    }
  });
}

template <typename Visitor>
inline auto http_for_each_field_value(const std::string_view header,
                                      Visitor visit) -> void {
  http_for_each_list_entry(header, [&visit](const std::string_view entry) {
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
