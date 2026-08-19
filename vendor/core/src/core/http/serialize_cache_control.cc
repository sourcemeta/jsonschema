#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::ranges::all_of
#include <chrono>      // std::chrono::seconds
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

constexpr std::string_view DIRECTIVE_SEPARATOR{", "};

auto append_directive(std::string &out, const std::string_view name,
                      bool &first) -> void {
  if (!first) {
    out.append(DIRECTIVE_SEPARATOR);
  }

  out.append(name);
  first = false;
}

// RFC 9111 §5.2.2.1: "This directive uses the token form of the argument
// syntax: e.g., 'max-age=5' not 'max-age="5"'. A sender MUST NOT generate the
// quoted-string form"
auto append_seconds(std::string &out, const std::string_view name,
                    const std::chrono::seconds value, bool &first) -> void {
  append_directive(out, name, first);
  out.push_back('=');
  sourcemeta::core::DigitsBuffer buffer;
  out.append(sourcemeta::core::digits_view(value.count(), buffer));
}

// RFC 9111 §5.2.2.4 and §5.2.2.7: a qualified directive "uses the quoted-string
// form of the argument syntax. A sender SHOULD NOT generate the token form
// (even if quoting appears not to be needed for single-entry lists)"
auto append_fields(std::string &out, const std::string_view name,
                   const std::span<const std::string_view> fields, bool &first)
    -> void {
  append_directive(out, name, first);
  out.append("=\"");
  bool leading{true};
  for (const auto field : fields) {
    if (!leading) {
      out.append(DIRECTIVE_SEPARATOR);
    }

    out.append(field);
    leading = false;
  }

  out.push_back('"');
}

// RFC 9111 §5.2.2.7 and §5.2.2.9 name the two, which the shared entry point
// has already established the value to be one of
auto visibility_directive(
    const sourcemeta::core::HTTPCacheVisibility value) noexcept
    -> std::string_view {
  switch (value) {
    case sourcemeta::core::HTTPCacheVisibility::Public:
      return "public";
    case sourcemeta::core::HTTPCacheVisibility::Private:
      return "private";
  }

  std::unreachable();
}

// RFC 9110 §5.1: field-name = token, so a name outside that set could not be
// carried even by the quoting
auto fields_valid(const std::span<const std::string_view> fields) -> bool {
  return std::ranges::all_of(fields, [](const std::string_view field) -> bool {
    return sourcemeta::core::http_is_token(field);
  });
}

} // namespace

namespace sourcemeta::core {

auto http_cache_control_valid(const HTTPCacheControl &directives) -> bool {
  // A visibility naming neither of the two directives names no directive at
  // all, so it cannot stand in for one below
  if (directives.visibility.has_value() &&
      directives.visibility != HTTPCacheVisibility::Public &&
      directives.visibility != HTTPCacheVisibility::Private) {
    return false;
  }

  // RFC 9111 §1.2.2: "The delta-seconds rule specifies a non-negative integer"
  if ((directives.max_age.has_value() && directives.max_age->count() < 0) ||
      (directives.shared_max_age.has_value() &&
       directives.shared_max_age->count() < 0)) {
    return false;
  }

  // RFC 9111 §5.2.2.7 qualifies the private directive, so a field list without
  // it limits nothing and names no directive to qualify
  if (!directives.private_fields.empty() &&
      directives.visibility != HTTPCacheVisibility::Private) {
    return false;
  }

  if (!fields_valid(directives.private_fields) ||
      !fields_valid(directives.no_cache_fields)) {
    return false;
  }

  // RFC 9111 §5.2: Cache-Control = #cache-directive, and RFC 9110 §5.6.1.1
  // forbids a sender from generating an empty list element, so a header naming
  // no directive at all is left unsent rather than emitted blank
  return directives.visibility.has_value() || directives.max_age.has_value() ||
         directives.shared_max_age.has_value() || directives.no_store ||
         directives.no_cache || !directives.no_cache_fields.empty() ||
         directives.must_revalidate || directives.proxy_revalidate ||
         directives.must_understand || directives.no_transform ||
         directives.immutable;
}

auto http_serialize_cache_control(const HTTPCacheControl &directives,
                                  std::string &out) -> bool {
  if (!http_cache_control_valid(directives)) {
    return false;
  }

  bool first{true};
  if (directives.visibility.has_value()) {
    const auto directive{visibility_directive(directives.visibility.value())};
    if (directives.private_fields.empty()) {
      append_directive(out, directive, first);
    } else {
      append_fields(out, directive, directives.private_fields, first);
    }
  }

  if (directives.no_store) {
    append_directive(out, "no-store", first);
  }

  if (directives.max_age.has_value()) {
    append_seconds(out, "max-age", directives.max_age.value(), first);
  }

  if (directives.shared_max_age.has_value()) {
    append_seconds(out, "s-maxage", directives.shared_max_age.value(), first);
  }

  // A field list is the qualified form of the very same directive, so only one
  // of the two spellings is ever emitted
  if (!directives.no_cache_fields.empty()) {
    append_fields(out, "no-cache", directives.no_cache_fields, first);
  } else if (directives.no_cache) {
    append_directive(out, "no-cache", first);
  }

  if (directives.must_revalidate) {
    append_directive(out, "must-revalidate", first);
  }

  if (directives.proxy_revalidate) {
    append_directive(out, "proxy-revalidate", first);
  }

  if (directives.must_understand) {
    append_directive(out, "must-understand", first);
  }

  if (directives.no_transform) {
    append_directive(out, "no-transform", first);
  }

  if (directives.immutable) {
    append_directive(out, "immutable", first);
  }

  return true;
}

auto http_serialize_cache_control(const HTTPCacheControl &directives)
    -> std::optional<std::string> {
  std::string out;
  if (!http_serialize_cache_control(directives, out)) {
    return std::nullopt;
  }

  return out;
}

} // namespace sourcemeta::core
