#include <sourcemeta/core/http.h>

#include <cstddef>     // std::size_t
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

constexpr std::string_view LINK_TARGET_OPEN{"<"};
constexpr std::string_view LINK_REL_OPEN{">; rel=\""};
constexpr std::string_view LINK_QUOTE_CLOSE{"\""};
constexpr std::string_view LINK_PARAMETER_OPEN{"; "};
constexpr std::string_view LINK_PARAMETER_EQUALS{"=\""};
constexpr std::string_view LINK_ENTRY_SEPARATOR{", "};

constexpr std::size_t LINK_FIXED_BYTES{
    LINK_TARGET_OPEN.size() + LINK_REL_OPEN.size() + LINK_QUOTE_CLOSE.size()};

constexpr std::size_t LINK_PARAMETER_FIXED_BYTES{LINK_PARAMETER_OPEN.size() +
                                                 LINK_PARAMETER_EQUALS.size() +
                                                 LINK_QUOTE_CLOSE.size()};

auto required_size(const sourcemeta::core::HTTPLink &link) noexcept
    -> std::size_t {
  std::size_t size{LINK_FIXED_BYTES + link.target.size() + link.rel.size()};
  for (const auto &[name, value] : link.parameters) {
    size += LINK_PARAMETER_FIXED_BYTES + name.size() + value.size();
  }
  return size;
}

} // namespace

namespace sourcemeta::core {

auto http_format_link(const HTTPLink &link, std::string &out) -> void {
  out.reserve(out.size() + required_size(link));
  out.append(LINK_TARGET_OPEN);
  out.append(link.target);
  out.append(LINK_REL_OPEN);
  out.append(link.rel);
  out.append(LINK_QUOTE_CLOSE);
  for (const auto &[name, value] : link.parameters) {
    out.append(LINK_PARAMETER_OPEN);
    out.append(name);
    out.append(LINK_PARAMETER_EQUALS);
    out.append(value);
    out.append(LINK_QUOTE_CLOSE);
  }
}

auto http_format_link(const HTTPLink &link) -> std::string {
  std::string out;
  http_format_link(link, out);
  return out;
}

auto http_format_links(std::span<const HTTPLink> links, std::string &out)
    -> void {
  if (links.empty()) {
    return;
  }
  std::size_t total{0};
  for (const auto &link : links) {
    total += required_size(link);
  }
  total += LINK_ENTRY_SEPARATOR.size() * (links.size() - 1);
  out.reserve(out.size() + total);
  http_format_link(links.front(), out);
  for (auto iterator{links.begin() + 1}; iterator != links.end(); ++iterator) {
    out.append(LINK_ENTRY_SEPARATOR);
    http_format_link(*iterator, out);
  }
}

auto http_format_links(std::span<const HTTPLink> links) -> std::string {
  std::string out;
  http_format_links(links, out);
  return out;
}

} // namespace sourcemeta::core
