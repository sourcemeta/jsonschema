#include <sourcemeta/core/oidc_discovery.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

constexpr auto HASH_LINKS{JSON::Object::hash("links"sv)};
constexpr auto HASH_REL{JSON::Object::hash("rel"sv)};
constexpr auto HASH_HREF{JSON::Object::hash("href"sv)};

constexpr std::string_view ISSUER_RELATION{
    "http://openid.net/specs/connect/1.0/issuer"};

// OpenID Connect Discovery 1.0 Section 2: an issuer identifier is an https URL
// with a host and no query or fragment. RFC 3986 Section 3.1 makes the scheme
// case-insensitive
auto is_issuer_identifier(const std::string_view value) -> bool {
  try {
    const URI uri{value};
    return uri.scheme().has_value() &&
           equals_ignore_case(uri.scheme().value(), "https") &&
           uri.host().has_value() && !uri.host().value().empty() &&
           !uri.query().has_value() && !uri.fragment().has_value();
  } catch (const URIParseError &) {
    return false;
  }
}

} // namespace

auto oidc_discovery_url(const std::string_view issuer)
    -> std::optional<std::string> {
  // OpenID Connect Discovery 1.0 Section 4.1: the configuration URL is the
  // issuer with "/.well-known/openid-configuration" appended, the appended form
  // rather than the RFC 8414 inserted form
  std::string sink;
  if (!oauth_well_known_url(
          issuer, OAuthWellKnownKind::OpenIDConfigurationAppended, sink)) {
    return std::nullopt;
  }

  return sink;
}

auto oidc_webfinger_request(const std::string_view identifier)
    -> std::optional<OIDCWebFingerRequest> {
  OIDCWebFingerRequest request;

  // OpenID Connect Discovery 1.0 Section 2.1: an acct URI or a URL is kept, a
  // bare user@host becomes an acct URI, and any other input is an https URL
  if (identifier.starts_with("acct:") ||
      identifier.find("://") != std::string_view::npos) {
    request.resource = identifier;
  } else if (identifier.find('@') != std::string_view::npos) {
    request.resource = "acct:";
    request.resource.append(identifier);
  } else {
    request.resource = "https://";
    request.resource.append(identifier);
  }

  // OpenID Connect Discovery 1.0 Section 2.1: the host is the domain after the
  // last "@" of an acct resource, or the authority of a URL resource. An acct
  // URI carries no authority component, so the URI parser cannot expose its
  // host, but a URL resource is parsed rather than scanned by hand. The host is
  // copied out because the parsed URI does not outlive this scope
  std::string host;
  if (request.resource.starts_with("acct:")) {
    const auto account{
        rsplit_once(std::string_view{request.resource}.substr(5), '@')};
    if (!account.has_value()) {
      return std::nullopt;
    }

    host = account.value().second;
  } else {
    try {
      const URI resource{request.resource};
      // OpenID Connect Discovery 1.0 Section 2.1: a URL resource uses the https
      // scheme and carries a host, so a non-https URL identifier is rejected.
      // RFC 3986 Section 3.1 makes the scheme case-insensitive
      if (!resource.scheme().has_value() ||
          !equals_ignore_case(resource.scheme().value(), "https") ||
          !resource.host().has_value() || resource.host().value().empty()) {
        return std::nullopt;
      }

      // RFC 3986 Section 3.2.2: an IPv6 host is wrapped in brackets so the
      // WebFinger URL it is interpolated into stays valid
      if (resource.is_ipv6()) {
        host.push_back('[');
        host.append(resource.host().value());
        host.push_back(']');
      } else {
        host = resource.host().value();
      }

      // RFC 7033 Section 4: the WebFinger host is the full authority, so a
      // non-default port on the resource is preserved rather than dropped
      if (resource.port().has_value()) {
        host.push_back(':');
        host.append(std::to_string(resource.port().value()));
      }
    } catch (const URIParseError &) {
      return std::nullopt;
    }
  }

  if (host.empty()) {
    return std::nullopt;
  }

  request.url = "https://";
  request.url.append(host);
  request.url.append("/.well-known/webfinger?");
  URI::append_query_parameter(request.url, "resource", request.resource);
  URI::append_query_parameter(request.url, "rel", ISSUER_RELATION);
  return request;
}

auto oidc_webfinger_issuer(const JSON &descriptor)
    -> std::optional<std::string_view> {
  if (!descriptor.is_object()) {
    return std::nullopt;
  }

  const auto *links{descriptor.try_at("links"sv, HASH_LINKS)};
  if (links == nullptr || !links->is_array()) {
    return std::nullopt;
  }

  for (const auto &link : links->as_array()) {
    if (!link.is_object()) {
      continue;
    }

    const auto *relation{link.try_at("rel"sv, HASH_REL)};
    const auto *href{link.try_at("href"sv, HASH_HREF)};
    if (relation == nullptr || !relation->is_string() ||
        relation->to_string() != ISSUER_RELATION || href == nullptr ||
        !href->is_string()) {
      continue;
    }

    // OpenID Connect Discovery 1.0 Section 2: a matching link whose href is not
    // a valid issuer identifier is skipped rather than returned, and the search
    // continues
    const std::string_view value{href->to_string()};
    if (is_issuer_identifier(value)) {
      return value;
    }
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
