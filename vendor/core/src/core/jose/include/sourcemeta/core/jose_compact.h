#ifndef SOURCEMETA_CORE_JOSE_COMPACT_H_
#define SOURCEMETA_CORE_JOSE_COMPACT_H_

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

/// @ingroup jose
/// Split a compact serialization into its dot-separated segments, with no
/// result unless it carries exactly as many as were asked for. Both the JWS
/// (RFC 7515 Section 7.1) and the JWE (RFC 7516 Section 7.1) compact
/// serializations are a fixed count of base64url encoded parts joined by dots,
/// and base64url never spells a dot, so the split is unambiguous and a
/// trailing separator is a malformed input rather than an extra empty part.
///
/// A segment is allowed to be empty, since detached content leaves the payload
/// blank (RFC 7515 Appendix F) and direct encryption leaves the encrypted key
/// blank (RFC 7516 Section 5.1). The segments are returned as written, so
/// decoding and validating them is left to the caller, and they borrow from
/// the input, which must outlive them. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jose.h>
///
/// #include <cassert>
///
/// const auto segments{sourcemeta::core::jose_compact_segments<3>(
///     "eyJhbGciOiJSUzI1NiJ9.eyJpc3MiOiJhY21lIn0.c2ln")};
/// assert(segments.has_value());
/// assert(segments.value().at(0) == "eyJhbGciOiJSUzI1NiJ9");
/// ```
template <std::size_t Count>
auto jose_compact_segments(const std::string_view input) noexcept
    -> std::optional<std::array<std::string_view, Count>> {
  static_assert(Count > 0, "A compact serialization carries at least one part");
  std::array<std::string_view, Count> segments{};

  std::string_view rest{input};
  for (std::size_t index{0}; index + 1 < Count; index += 1) {
    const auto separator{rest.find('.')};
    if (separator == std::string_view::npos) {
      return std::nullopt;
    }

    std::string_view segment{rest};
    segment.remove_suffix(rest.size() - separator);
    segments[index] = segment;
    rest.remove_prefix(separator + 1);
  }

  if (rest.find('.') != std::string_view::npos) {
    return std::nullopt;
  }

  segments[Count - 1] = rest;
  return segments;
}

} // namespace sourcemeta::core

#endif
