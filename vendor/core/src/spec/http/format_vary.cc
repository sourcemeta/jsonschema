#include <sourcemeta/core/http.h>

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

constexpr std::string_view VARY_ENTRY_SEPARATOR{", "};

} // namespace

namespace sourcemeta::core {

auto http_format_vary(std::span<const std::string_view> field_names,
                      std::string &out) -> bool {
  // RFC 9110 §5.6.1.1: "a sender MUST NOT generate empty list elements", which
  // leaves nothing to send for an empty list, and §5.1 makes every member a
  // token, the wildcard among them
  if (field_names.empty()) {
    return false;
  }

  std::size_t total{VARY_ENTRY_SEPARATOR.size() * (field_names.size() - 1)};
  for (const auto field_name : field_names) {
    if (!http_is_token(field_name)) {
      return false;
    }

    total += field_name.size();
  }

  out.reserve(out.size() + total);
  out.append(field_names.front());
  for (const auto field_name : field_names.subspan(1)) {
    out.append(VARY_ENTRY_SEPARATOR);
    out.append(field_name);
  }

  return true;
}

auto http_format_vary(std::span<const std::string_view> field_names)
    -> std::optional<std::string> {
  std::string out;
  if (!http_format_vary(field_names, out)) {
    return std::nullopt;
  }

  return out;
}

} // namespace sourcemeta::core
