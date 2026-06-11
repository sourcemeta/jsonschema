#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "normalize.h"

#include <cctype>      // std::isxdigit
#include <cstddef>     // std::size_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {

auto apply_leading_slash_transform(std::optional<std::string> parsed_path,
                                   const bool needs_leading_slash)
    -> std::optional<std::string> {
  if (!parsed_path.has_value()) {
    return parsed_path;
  }

  const auto &path_value = parsed_path.value();

  if (needs_leading_slash) {
    if (path_value.empty() || !path_value.starts_with("/")) {
      return "/" + path_value;
    }
  }

  return parsed_path;
}

auto normalize_fragment(const std::string_view input) -> std::string {
  if (input.empty()) {
    return "";
  }

  // Strip leading '#' and store raw value
  return std::string{input.starts_with('#') ? input.substr(1) : input};
}

// Raw string validation against the RFC 3986 Section 3.3 path productions. The
// input is not parsed as a URI because that would misclassify ':' in the first
// segment as a scheme delimiter and silently drop a '?' or '#' suffix
auto validate_raw_path(const std::string_view path) -> void {
  if (path.starts_with("//")) {
    throw sourcemeta::core::URIError{
        "You cannot set a path that contains an authority"};
  }

  for (std::size_t index = 0; index < path.size(); ++index) {
    const char character = path[index];
    if (character == '%') {
      if (index + 2 >= path.size() ||
          !std::isxdigit(static_cast<unsigned char>(path[index + 1])) ||
          !std::isxdigit(static_cast<unsigned char>(path[index + 2]))) {
        throw sourcemeta::core::URIError{
            "You cannot set a path with an invalid percent-encoded sequence"};
      }
      index += 2;
      continue;
    }
    if (sourcemeta::core::uri_is_unreserved(character) ||
        sourcemeta::core::uri_is_sub_delim(character) || character == ':' ||
        character == '@' || character == '/') {
      continue;
    }
    if (character == '?') {
      throw sourcemeta::core::URIError{
          "You cannot set a path that contains a query"};
    }
    if (character == '#') {
      throw sourcemeta::core::URIError{
          "You cannot set a path that contains a fragment"};
    }
    throw sourcemeta::core::URIError{
        "You cannot set a path that contains an invalid character"};
  }
}

} // namespace

namespace sourcemeta::core {

auto URI::path(const std::string &path) -> URI & {
  if (path.empty()) {
    this->path_ = std::nullopt;
    return *this;
  }

  if (path.starts_with(".") && this->is_absolute()) {
    throw URIError{"You cannot set a relative path to an absolute URI"};
  }

  validate_raw_path(path);

  // Determine if this URI needs a leading slash
  // (URIs with scheme/authority need leading slash, except URNs/tags/mailto)
  const auto needs_leading_slash =
      (!this->is_urn() && !this->is_tag() && !this->is_mailto() &&
       this->scheme_.has_value()) ||
      this->port_.has_value() || this->host_.has_value();

  this->path_ = apply_leading_slash_transform(std::optional<std::string>{path},
                                              needs_leading_slash);
  return *this;
}

auto URI::path(std::string &&path) -> URI & {
  if (path.empty()) {
    this->path_ = std::nullopt;
    return *this;
  }

  if (path.starts_with(".") && this->is_absolute()) {
    throw URIError{"You cannot set a relative path to an absolute URI"};
  }

  validate_raw_path(path);

  // Determine if this URI needs a leading slash
  // (URIs with scheme/authority need leading slash, except URNs/tags/mailto)
  const auto needs_leading_slash =
      (!this->is_urn() && !this->is_tag() && !this->is_mailto() &&
       this->scheme_.has_value()) ||
      this->port_.has_value() || this->host_.has_value();

  this->path_ = apply_leading_slash_transform(
      std::optional<std::string>{std::move(path)}, needs_leading_slash);
  return *this;
}

namespace {

auto merge_reference_path(std::optional<std::string> &current_path,
                          const bool needs_leading_slash,
                          const std::string_view reference_path) -> void {
  std::string merged_path;
  if (current_path.has_value()) {
    const auto &existing = current_path.value();
    const auto current_ends_with_slash = existing.ends_with('/');
    const auto reference_starts_with_slash = reference_path.starts_with('/');

    if (current_ends_with_slash && reference_starts_with_slash) {
      merged_path = existing;
      merged_path.append(reference_path.substr(1));
    } else if (!current_ends_with_slash && !reference_starts_with_slash) {
      merged_path = existing;
      merged_path += '/';
      merged_path += reference_path;
    } else {
      merged_path = existing;
      merged_path += reference_path;
    }
  } else if (needs_leading_slash && !reference_path.starts_with('/')) {
    merged_path = "/";
    merged_path += reference_path;
  } else {
    merged_path = reference_path;
  }

  sourcemeta::core::normalize_path(merged_path);
  if (merged_path.empty()) {
    current_path = std::nullopt;
  } else {
    current_path = std::move(merged_path);
  }
}

} // namespace

auto URI::append_path(std::string_view path) -> URI & {
  if (path.empty()) {
    return *this;
  }

  // Raw string validation. The input is not parsed as a URI because that
  // would misclassify ':' in the first segment as a scheme delimiter
  // (RFC 3986 grammar ambiguity between path-noscheme and scheme), which
  // would reject valid path appends like "foo:bar".
  if (path.starts_with("//")) {
    throw URIError{"Cannot append a URI as a path that contains an authority"};
  }
  for (std::size_t index = 0; index < path.size(); ++index) {
    if (path[index] == '/') {
      break;
    }
    if (path[index] == ':') {
      if (index + 2 < path.size() && path[index + 1] == '/' &&
          path[index + 2] == '/') {
        throw URIError{
            "Cannot append a URI as a path that contains a scheme and "
            "authority"};
      }
      break;
    }
  }

  // Per-character validation against the RFC 3986 Section 3.3 path
  // productions: pchar = unreserved / pct-encoded / sub-delims / ":" / "@",
  // plus '/' as segment separator.
  for (std::size_t index = 0; index < path.size(); ++index) {
    const char character = path[index];
    if (character == '%') {
      if (index + 2 >= path.size() ||
          !std::isxdigit(static_cast<unsigned char>(path[index + 1])) ||
          !std::isxdigit(static_cast<unsigned char>(path[index + 2]))) {
        throw URIError{
            "Cannot append a URI as a path that has an invalid percent-encoded "
            "sequence"};
      }
      index += 2;
      continue;
    }
    if (uri_is_unreserved(character) || uri_is_sub_delim(character) ||
        character == ':' || character == '@' || character == '/') {
      continue;
    }
    if (character == '?') {
      throw URIError{"Cannot append a URI as a path that contains a query"};
    }
    if (character == '#') {
      throw URIError{"Cannot append a URI as a path that contains a fragment"};
    }
    throw URIError{
        "Cannot append a URI as a path that contains an invalid character"};
  }

  const auto needs_leading_slash =
      (!this->is_urn() && !this->is_tag() && !this->is_mailto() &&
       this->scheme_.has_value()) ||
      this->port_.has_value() || this->host_.has_value();
  merge_reference_path(this->path_, needs_leading_slash, path);
  return *this;
}

namespace {

auto validate_uri_reference_components(const URI &reference) -> void {
  if (reference.scheme().has_value() || reference.userinfo().has_value() ||
      reference.host().has_value() || reference.port().has_value()) {
    throw URIError{
        "Cannot append a URI as a path that contains a scheme or authority"};
  }

  if (reference.query().has_value() || reference.fragment().has_value()) {
    throw URIError{
        "Cannot append a URI as a path that contains a query or fragment"};
  }
}

} // namespace

auto URI::append_path(const URI &reference) -> URI & {
  validate_uri_reference_components(reference);

  if (!reference.path_.has_value()) {
    return *this;
  }

  const auto needs_leading_slash =
      (!this->is_urn() && !this->is_tag() && !this->is_mailto() &&
       this->scheme_.has_value()) ||
      this->port_.has_value() || this->host_.has_value();
  merge_reference_path(this->path_, needs_leading_slash,
                       reference.path_.value());
  return *this;
}

auto URI::append_path(URI &&reference) -> URI & {
  validate_uri_reference_components(reference);

  if (!reference.path_.has_value()) {
    return *this;
  }

  const auto needs_leading_slash =
      (!this->is_urn() && !this->is_tag() && !this->is_mailto() &&
       this->scheme_.has_value()) ||
      this->port_.has_value() || this->host_.has_value();
  std::string reference_path{std::move(reference.path_.value())};
  reference.path_ = std::nullopt;
  merge_reference_path(this->path_, needs_leading_slash, reference_path);
  return *this;
}

auto URI::extension(std::string &&extension) -> URI & {
  if (!this->path_.has_value()) {
    return *this;
  }

  auto &path = this->path_.value();
  if (path.empty() || path.ends_with('/')) {
    return *this;
  }

  // Find the last dot after the last slash (if any)
  const auto last_slash_pos = path.find_last_of('/');
  const auto last_dot_pos = path.find_last_of('.');

  // Only consider the dot if it's after the last slash (or there's no slash)
  const auto has_extension =
      last_dot_pos != std::string::npos &&
      (last_slash_pos == std::string::npos || last_dot_pos > last_slash_pos);

  // Remove existing extension if present
  if (has_extension) {
    path.erase(last_dot_pos);
  }

  // Add new extension if not empty
  if (!extension.empty()) {
    // Strip leading dot from extension if present
    if (extension.starts_with('.')) {
      path += extension;
    } else {
      path += '.';
      path += extension;
    }
  }

  return *this;
}

auto URI::fragment(const std::string_view fragment) -> URI & {
  auto value{normalize_fragment(fragment)};
  uri_unescape_unreserved_inplace(value);
  this->fragment_ = std::move(value);
  return *this;
}

auto URI::query(const std::string_view query) -> URI & {
  if (query.empty()) {
    this->query_ = std::nullopt;
    return *this;
  }

  std::string value{query.starts_with('?') ? query.substr(1) : query};
  uri_unescape_unreserved_inplace(value);
  this->query_ = std::move(value);
  return *this;
}

auto URI::userinfo(const std::string_view userinfo) -> URI & {
  if (userinfo.empty()) {
    this->userinfo_ = std::nullopt;
    return *this;
  }

  std::string value{userinfo};
  uri_unescape_unreserved_inplace(value);
  this->userinfo_ = std::move(value);
  return *this;
}

} // namespace sourcemeta::core
