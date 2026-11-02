#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace {

auto byte_at(const std::string_view value,
             const std::string_view::size_type position) noexcept
    -> std::uint8_t {
  return static_cast<std::uint8_t>(
      (sourcemeta::core::hex_digit_value(value[position]) << 4) |
      sourcemeta::core::hex_digit_value(value[position + 1]));
}

} // namespace

namespace sourcemeta::core {

auto http_parse_traceparent(const std::string_view value) noexcept
    -> std::optional<HTTPTraceParent> {
  if (!http_is_traceparent(value)) {
    return std::nullopt;
  }

  // The fields sit at fixed offsets, since a later version keeps the first
  // four where version 00 puts them and appends beyond the flags, which a
  // reader is told to ignore: "Vendors MUST NOT parse or assume anything about
  // unknown fields for this version"
  // https://www.w3.org/TR/trace-context/#versioning-of-traceparent
  return HTTPTraceParent{.version = byte_at(value, 0),
                         .trace_id = value.substr(3, 32),
                         .parent_id = value.substr(36, 16),
                         .flags = byte_at(value, 53)};
}

} // namespace sourcemeta::core
