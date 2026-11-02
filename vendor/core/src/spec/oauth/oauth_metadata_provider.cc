#include <sourcemeta/core/oauth_metadata_provider.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_metadata.h>

#include "oauth_ttl.h"

#include <cassert>  // assert
#include <chrono>   // std::chrono::seconds, std::chrono::system_clock
#include <memory>   // std::make_shared, std::shared_ptr
#include <mutex>    // std::scoped_lock
#include <optional> // std::optional
#include <string>   // std::string
#include <utility>  // std::move

namespace sourcemeta::core {

OAuthMetadataProvider::OAuthMetadataProvider(std::string issuer,
                                             const OAuthWellKnownKind kind,
                                             Fetcher fetcher)
    : OAuthMetadataProvider{std::move(issuer), kind, std::move(fetcher),
                            Options{}} {}

OAuthMetadataProvider::OAuthMetadataProvider(std::string issuer,
                                             const OAuthWellKnownKind kind,
                                             Fetcher fetcher, Options options)
    : OAuthMetadataProvider{std::move(issuer), kind, std::move(fetcher),
                            options, Clock{&std::chrono::system_clock::now}} {}

OAuthMetadataProvider::OAuthMetadataProvider(std::string issuer,
                                             const OAuthWellKnownKind kind,
                                             Fetcher fetcher, Options options,
                                             Clock clock)
    : issuer_{std::move(issuer)}, kind_{kind}, fetcher_{std::move(fetcher)},
      options_{options}, clock_{std::move(clock)} {
  // A protected resource document is not authorization server metadata, so it
  // would never validate and the provider would permanently return no value
  assert(kind != OAuthWellKnownKind::ProtectedResource);
}

auto OAuthMetadataProvider::install_locked(
    const OAuthWellKnownKind kind,
    const std::chrono::system_clock::time_point now) -> bool {
  std::string url;
  if (!oauth_well_known_url(this->issuer_, kind, url)) {
    return false;
  }

  std::optional<FetchResult> fetched;
  try {
    fetched = this->fetcher_(url);
  } catch (...) {
    // A fetcher signals failure by returning no value, but a throwing transport
    // is treated as just another failed retrieval, so a misbehaving fetcher can
    // never escape a metadata lookup
    return false;
  }

  if (!fetched.has_value()) {
    return false;
  }

  auto document{try_parse_json(fetched.value().body)};
  if (!document.has_value()) {
    return false;
  }

  auto parsed{
      OAuthServerMetadata::from(std::move(document).value(), this->issuer_)};
  if (!parsed.has_value()) {
    return false;
  }

  this->metadata_ =
      std::make_shared<const OAuthServerMetadata>(std::move(parsed).value());
  this->next_refresh_ = now + oauth_clamp_ttl(fetched.value().max_age,
                                              this->options_.fallback_ttl,
                                              this->options_.minimum_ttl,
                                              this->options_.maximum_ttl);
  return true;
}

auto OAuthMetadataProvider::fetch_and_install_locked(
    const std::chrono::system_clock::time_point now) -> bool {
  if (this->install_locked(this->kind_, now)) {
    return true;
  }

  // RFC 8414 Section 5: the inserted openid-configuration form is tried first
  // and the legacy appended form only as a fallback
  if (this->kind_ == OAuthWellKnownKind::OpenIDConfigurationInserted) {
    return this->install_locked(OAuthWellKnownKind::OpenIDConfigurationAppended,
                                now);
  }

  return false;
}

auto OAuthMetadataProvider::metadata()
    -> std::shared_ptr<const OAuthServerMetadata> {
  const auto now{this->clock_()};
  const std::scoped_lock lock{this->mutex_};
  // The refresh deadline starts at the epoch, so the first call always
  // retrieves. Gating solely on it, rather than also on an absent document,
  // means a failed retrieval backs off for the minimum lifetime instead of
  // being retried on every call during an outage, whether or not a prior good
  // document exists to serve meanwhile
  if (now >= this->next_refresh_) {
    if (!this->fetch_and_install_locked(now)) {
      this->next_refresh_ = now + this->options_.minimum_ttl;
    }
  }

  return this->metadata_;
}

auto OAuthMetadataProvider::refresh()
    -> std::shared_ptr<const OAuthServerMetadata> {
  const auto now{this->clock_()};
  const std::scoped_lock lock{this->mutex_};
  this->fetch_and_install_locked(now);
  return this->metadata_;
}

OAuthResourceMetadataProvider::OAuthResourceMetadataProvider(
    std::string resource, Fetcher fetcher)
    : OAuthResourceMetadataProvider{std::move(resource), std::move(fetcher),
                                    Options{}} {}

OAuthResourceMetadataProvider::OAuthResourceMetadataProvider(
    std::string resource, Fetcher fetcher, Options options)
    : OAuthResourceMetadataProvider{std::move(resource), std::move(fetcher),
                                    options,
                                    Clock{&std::chrono::system_clock::now}} {}

OAuthResourceMetadataProvider::OAuthResourceMetadataProvider(
    std::string resource, Fetcher fetcher, Options options, Clock clock)
    : resource_{std::move(resource)}, fetcher_{std::move(fetcher)},
      options_{options}, clock_{std::move(clock)} {}

auto OAuthResourceMetadataProvider::fetch_and_install_locked(
    const std::chrono::system_clock::time_point now) -> bool {
  std::string url;
  if (!oauth_well_known_url(this->resource_,
                            OAuthWellKnownKind::ProtectedResource, url)) {
    return false;
  }

  std::optional<FetchResult> fetched;
  try {
    fetched = this->fetcher_(url);
  } catch (...) {
    // A fetcher signals failure by returning no value, but a throwing transport
    // is treated as just another failed retrieval, so a misbehaving fetcher can
    // never escape a metadata lookup
    return false;
  }

  if (!fetched.has_value()) {
    return false;
  }

  auto document{try_parse_json(fetched.value().body)};
  if (!document.has_value()) {
    return false;
  }

  auto parsed{OAuthResourceMetadata::from(std::move(document).value(),
                                          this->resource_)};
  if (!parsed.has_value()) {
    return false;
  }

  this->metadata_ =
      std::make_shared<const OAuthResourceMetadata>(std::move(parsed).value());
  this->next_refresh_ = now + oauth_clamp_ttl(fetched.value().max_age,
                                              this->options_.fallback_ttl,
                                              this->options_.minimum_ttl,
                                              this->options_.maximum_ttl);
  return true;
}

auto OAuthResourceMetadataProvider::metadata()
    -> std::shared_ptr<const OAuthResourceMetadata> {
  const auto now{this->clock_()};
  const std::scoped_lock lock{this->mutex_};
  // The refresh deadline starts at the epoch, so the first call always
  // retrieves. Gating solely on it, rather than also on an absent document,
  // means a failed retrieval backs off for the minimum lifetime instead of
  // being retried on every call during an outage, whether or not a prior good
  // document exists to serve meanwhile
  if (now >= this->next_refresh_) {
    if (!this->fetch_and_install_locked(now)) {
      this->next_refresh_ = now + this->options_.minimum_ttl;
    }
  }

  return this->metadata_;
}

auto OAuthResourceMetadataProvider::refresh()
    -> std::shared_ptr<const OAuthResourceMetadata> {
  const auto now{this->clock_()};
  const std::scoped_lock lock{this->mutex_};
  this->fetch_and_install_locked(now);
  return this->metadata_;
}

} // namespace sourcemeta::core
