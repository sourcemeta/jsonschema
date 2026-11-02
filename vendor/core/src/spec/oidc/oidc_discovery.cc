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

// OpenID Connect Discovery 1.0 Section 2.1: an explicit scheme such as acct or
// https suppresses normalization. A bare "host:port" such as "example.com:8080"
// is listed as scheme-less input, so a colon that begins a port rather than an
// opaque or hierarchical part is not treated as a scheme delimiter (RFC 3986
// Section 3.1: "scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )")
auto identifier_has_scheme(const std::string_view identifier) -> bool {
  const auto delimiter{identifier.find_first_of(":/?#@")};
  if (delimiter == std::string_view::npos || identifier[delimiter] != ':') {
    return false;
  }

  const auto candidate{identifier.substr(0, delimiter)};
  if (candidate.empty() || !is_alpha(candidate.front())) {
    return false;
  }

  for (const auto character : candidate) {
    if (!is_alphanum(character) && character != '+' && character != '-' &&
        character != '.') {
      return false;
    }
  }

  // A colon that begins an all-digit port up to the next component delimiter is
  // the "host:port" shape rather than a scheme, so only that shape overrides
  // the RFC 3986 scheme detection
  const auto rest{identifier.substr(delimiter + 1)};
  const auto port{rest.substr(0, rest.find_first_of("/?#"))};
  if (port.empty()) {
    return true;
  }

  for (const auto character : port) {
    if (!is_digit(character)) {
      return true;
    }
  }

  return false;
}

// OpenID Connect Discovery 1.0 Section 2.1: "If the userinfo and host
// components are present and all of the scheme, path, query, port, and fragment
// components are absent, the acct scheme is assumed". A scheme-less input is
// read as "[ userinfo "@" ] host [ ":" port ]", so this shape carries an "@"
// and none of the port, path, query, or fragment delimiters
auto identifier_is_acct_shaped(const std::string_view identifier) -> bool {
  return identifier.contains('@') &&
         identifier.find_first_of(":/?#") == std::string_view::npos;
}

// OpenID Connect Discovery 1.0 Section 2: an issuer identifier is an https URL
// with a host and no query or fragment
auto is_issuer_identifier(const std::string_view value) -> bool {
  try {
    const URI uri{value};
    return uri.is_https() && uri.host().has_value() &&
           !uri.host().value().empty() && !uri.query().has_value() &&
           !uri.fragment().has_value();
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

  // OpenID Connect Discovery 1.0 Section 2.1: an input that already carries a
  // scheme is kept, a scheme-less userinfo-and-host-only input takes the acct
  // scheme, and "For all other inputs without a scheme component, the https
  // scheme is assumed"
  if (identifier_has_scheme(identifier)) {
    request.resource = identifier;
  } else if (identifier_is_acct_shaped(identifier)) {
    request.resource = "acct:";
    request.resource.append(identifier);
  } else {
    request.resource = "https://";
    request.resource.append(identifier);
  }

  // OpenID Connect Discovery 1.0 Section 2.1: "If the resulting URI contains a
  // fragment component, it MUST be stripped off, together with the fragment
  // delimiter character"
  const auto fragment{request.resource.find('#')};
  if (fragment != std::string::npos) {
    request.resource.erase(fragment);
  }

  // OpenID Connect Discovery 1.0 Section 2.1: the host is the domain after the
  // last "@" of an acct resource, or the authority of a URL resource. An acct
  // URI carries no authority component, so the URI parser cannot expose its
  // host, but a URL resource is parsed rather than scanned by hand. The host is
  // copied out because the parsed URI does not outlive this scope
  std::string host;
  // RFC 3986 Section 3.1: a scheme is case-insensitive, so an acct resource is
  // recognized regardless of the case the caller used
  if (starts_with_ignore_case(request.resource, "acct:")) {
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
      // scheme and carries a host, so a non-https URL identifier is rejected
      if (!resource.is_https() || !resource.host().has_value() ||
          resource.host().value().empty()) {
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
