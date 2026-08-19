#include <sourcemeta/core/oidc_subject.h>

#include <sourcemeta/core/crypto.h>

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace sourcemeta::core {

auto oidc_subject_type_name(const OIDCSubjectType type) noexcept
    -> std::string_view {
  switch (type) {
    case OIDCSubjectType::Public:
      return "public";
    case OIDCSubjectType::Pairwise:
      return "pairwise";
  }

  std::unreachable();
}

auto to_oidc_subject_type(const std::string_view name) noexcept
    -> std::optional<OIDCSubjectType> {
  if (name == "public") {
    return OIDCSubjectType::Public;
  }
  if (name == "pairwise") {
    return OIDCSubjectType::Pairwise;
  }
  return std::nullopt;
}

auto oidc_pairwise_subject(const std::string_view sector_identifier,
                           const std::string_view local_account_identifier,
                           const std::string_view provider_secret)
    -> std::string {
  // OpenID Connect Core 1.0 Section 8.1: the pairwise value is derived from the
  // sector identifier, a local account identifier, and a provider secret. The
  // HMAC input is length-prefixed so the field boundary cannot collide with a
  // separator that also appears in the data, keeping distinct pairs distinct
  const auto sector_length{std::to_string(sector_identifier.size())};
  std::string message;
  message.reserve(sector_length.size() + 1 + sector_identifier.size() +
                  local_account_identifier.size());
  message.append(sector_length);
  message.push_back(':');
  message.append(sector_identifier);
  message.append(local_account_identifier);

  const auto digest{hmac_sha256_digest(provider_secret, message)};
  return base64url_encode(digest);
}

} // namespace sourcemeta::core
