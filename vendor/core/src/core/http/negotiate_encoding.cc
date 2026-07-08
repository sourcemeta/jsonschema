#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view

namespace sourcemeta::core {

// NOLINTNEXTLINE(bugprone-exception-escape)
auto http_negotiate_encoding(
    const std::string_view accept_encoding_header,
    const HTTPContentEncoding server_preference) noexcept
    -> std::optional<HTTPContentEncoding> {
  if (http_trim_leading_ows(accept_encoding_header).empty()) {
    return HTTPContentEncoding::Identity;
  }

  float gzip_quality{0.0f};
  float identity_quality{0.0f};
  float wildcard_quality{0.0f};
  bool gzip_listed{false};
  bool identity_listed{false};
  bool wildcard_listed{false};

  http_for_each_accept_entry(
      accept_encoding_header,
      [&](const std::string_view token, const float quality) -> void {
        if (equals_ignore_case(token, "gzip") ||
            equals_ignore_case(token, "x-gzip")) {
          gzip_listed = true;
          if (quality > gzip_quality) {
            gzip_quality = quality;
          }
        } else if (equals_ignore_case(token, "identity")) {
          identity_listed = true;
          if (quality > identity_quality) {
            identity_quality = quality;
          }
        } else if (token == "*") {
          wildcard_listed = true;
          if (quality > wildcard_quality) {
            wildcard_quality = quality;
          }
        }
      });

  if (!gzip_listed && wildcard_listed) {
    gzip_quality = wildcard_quality;
  }
  if (!identity_listed) {
    if (wildcard_listed) {
      identity_quality = wildcard_quality;
    } else {
      identity_quality = 1.0f;
    }
  }

  const bool gzip_acceptable{gzip_quality > 0.0f};
  const bool identity_acceptable{identity_quality > 0.0f};

  if (!gzip_acceptable && !identity_acceptable) {
    return std::nullopt;
  }
  if (!gzip_acceptable) {
    return HTTPContentEncoding::Identity;
  }
  if (!identity_acceptable) {
    return HTTPContentEncoding::GZIP;
  }
  if (gzip_quality > identity_quality) {
    return HTTPContentEncoding::GZIP;
  }
  if (identity_quality > gzip_quality) {
    return HTTPContentEncoding::Identity;
  }
  return server_preference;
}

} // namespace sourcemeta::core
