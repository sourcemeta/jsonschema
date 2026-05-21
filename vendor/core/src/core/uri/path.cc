#include <sourcemeta/core/uri.h>

#include "escaping.h"
#include "grammar.h"
#include "normalize.h"

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

auto canonicalize_path(const std::string_view input, std::string &output)
    -> bool {
  output.assign(input);
  if (output.empty()) {
    return true;
  }
  if (output.front() != sourcemeta::core::URI_SLASH) {
    return false;
  }

  for (std::size_t index{0}; index < output.size();) {
    const auto character{output[index]};
    if (character == sourcemeta::core::URI_PERCENT) {
      if (!sourcemeta::core::uri_is_percent_encoded(output, index)) {
        return false;
      }
      index += 3;
    } else if (character == sourcemeta::core::URI_SLASH ||
               sourcemeta::core::uri_is_pchar(character)) {
      ++index;
    } else {
      return false;
    }
  }

  sourcemeta::core::uri_normalize_percent_encoding_inplace(output);
  sourcemeta::core::uri_unescape_unreserved_inplace(output);
  sourcemeta::core::normalize_path(output);
  return true;
}

} // namespace

namespace sourcemeta::core {

auto URI::strip_path_prefix(const std::string_view path,
                            const std::string_view prefix)
    -> std::optional<std::string> {
  std::string path_canonical;
  std::string prefix_canonical;
  if (!canonicalize_path(path, path_canonical) ||
      !canonicalize_path(prefix, prefix_canonical)) {
    return std::nullopt;
  }

  std::size_t suffix_start{0};
  const bool prefix_provides_boundary{prefix_canonical.ends_with(URI_SLASH)};
  if (!prefix_canonical.empty()) {
    if (!path_canonical.starts_with(prefix_canonical)) {
      return std::nullopt;
    }
    if (!prefix_provides_boundary &&
        path_canonical.size() > prefix_canonical.size() &&
        path_canonical[prefix_canonical.size()] != URI_SLASH) {
      return std::nullopt;
    }
    suffix_start = prefix_canonical.size();
  }
  if (!prefix_provides_boundary && suffix_start < path_canonical.size() &&
      path_canonical[suffix_start] == URI_SLASH) {
    ++suffix_start;
  }

  path_canonical.erase(0, suffix_start);
  return path_canonical;
}

auto URI::rebase_path(const std::string_view path,
                      const std::string_view old_prefix,
                      const std::string_view new_prefix)
    -> std::optional<std::string> {
  const auto suffix{URI::strip_path_prefix(path, old_prefix)};
  if (!suffix.has_value()) {
    return std::nullopt;
  }
  const bool needs_separator{!suffix.value().empty() && !new_prefix.empty() &&
                             !new_prefix.ends_with(URI_SLASH)};
  std::string result;
  result.reserve(new_prefix.size() + (needs_separator ? 1 : 0) +
                 suffix.value().size());
  result.append(new_prefix);
  if (needs_separator) {
    result.push_back(URI_SLASH);
  }
  result.append(suffix.value());
  return result;
}

} // namespace sourcemeta::core
