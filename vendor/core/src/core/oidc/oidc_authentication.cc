#include <sourcemeta/core/oidc_authentication.h>

#include <sourcemeta/core/oauth.h>
#include <sourcemeta/core/text.h>

#include <algorithm>   // std::ranges::all_of
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

// Whether a space-delimited list carries a token (OpenID Connect Core 1.0
// Section 3.1.2.1, which compares such parameters as space-delimited lists)
auto space_list_contains(const std::string_view list,
                         const std::string_view token) -> bool {
  bool found{false};
  split(list, ' ', [&found, token](const std::string_view value) -> void {
    if (value == token) {
      found = true;
    }
  });
  return found;
}

// OpenID Connect Core 1.0 Section 3.1.2.1: the none prompt value "MUST NOT be
// used with any other value", so it is only valid alone
auto prompt_is_valid(const std::string_view prompt) -> bool {
  if (prompt.empty() || !space_list_contains(prompt, "none")) {
    return true;
  }

  std::size_t token_count{0};
  split(prompt, ' ', [&token_count](const std::string_view value) -> void {
    if (!value.empty()) {
      token_count += 1;
    }
  });
  return token_count == 1;
}

// OpenID Connect Core 1.0 Section 11: obtaining a refresh token through the
// offline_access scope requires the end user's consent, which the none prompt
// forbids gathering, so the two cannot be requested together
auto offline_access_is_valid(const std::string_view scope,
                             const std::string_view prompt) -> bool {
  return !space_list_contains(scope, "offline_access") ||
         !space_list_contains(prompt, "none");
}

// OpenID Connect Core 1.0 Section 3, module design Section 14: the
// Authorization Code flow is always permitted, the Hybrid "code id_token" flow
// only under the legacy profile, and every flow that returns an access token
// from the authorization endpoint, as well as the pure implicit "id_token"
// flow, is never permitted. A response_type is an unordered space-delimited set
// (OAuth 2.0 Section 3.1.1)
auto response_type_is_allowed(const std::string_view response_type,
                              const OIDCProfile profile) -> bool {
  // An unset response_type is left to the OAuth layer, which applies the
  // default "code", so only a value that is actually present is validated here
  if (response_type.empty()) {
    return true;
  }

  bool has_token{false};
  bool has_code{false};
  bool has_id_token{false};
  bool has_unsupported{false};
  split(response_type, ' ',
        [&has_token, &has_code, &has_id_token,
         &has_unsupported](const std::string_view value) -> void {
          if (value.empty()) {
            return;
          }

          has_token = true;
          if (value == "code") {
            has_code = true;
          } else if (value == "id_token") {
            has_id_token = true;
          } else {
            has_unsupported = true;
          }
        });

  // A present value that carries no token, such as one that is only whitespace,
  // is not a meaningful response type
  if (!has_token || has_unsupported) {
    return false;
  }

  if (has_id_token) {
    return has_code && profile == OIDCProfile::Legacy;
  }

  return true;
}

// OpenID Connect Core 1.0 Section 3.1.2.1: nonce is REQUIRED whenever an ID
// Token is returned from the authorization endpoint, which binds it to the
// session in the implicit and hybrid flows
auto response_type_requires_nonce(const std::string_view response_type)
    -> bool {
  bool requires_nonce{false};
  split(response_type, ' ',
        [&requires_nonce](const std::string_view value) -> void {
          if (value == "id_token") {
            requires_nonce = true;
          }
        });
  return requires_nonce;
}

// The only supported PKCE method is S256, whose code challenge is the base64url
// encoding of a SHA-256 digest (RFC 7636 Section 4.2, Appendix A), so it is
// exactly 43 characters drawn from the base64url alphabet. The "." and "~" of
// the wider unreserved grammar can only appear under the plain method and never
// in a redeemable S256 challenge
auto code_challenge_is_valid(const std::string_view code_challenge) -> bool {
  return code_challenge.size() == 43 &&
         std::ranges::all_of(code_challenge, [](const char character) -> bool {
           return (character >= 'A' && character <= 'Z') ||
                  (character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9') || character == '-' ||
                  character == '_';
         });
}

// RFC 9700 Section 2.1.1: the strict profile binds the authorization code to
// the client with PKCE using the S256 method, protecting against code
// interception, so a strict request carries a well-formed code_challenge and
// the S256 method. The legacy profile leaves this to the caller for older
// deployments
auto pkce_is_valid(const std::string_view code_challenge,
                   const std::string_view code_challenge_method,
                   const OIDCProfile profile) -> bool {
  if (profile != OIDCProfile::Strict) {
    return true;
  }

  return code_challenge_method == "S256" &&
         code_challenge_is_valid(code_challenge);
}

// OpenID Connect Core 1.0 Section 3.1.2.1: store a parsed OpenID Connect
// authentication parameter that the OAuth layer treats as an extension
auto assign_openid_parameter(OIDCAuthenticationRequest &result,
                             const std::string_view name,
                             const std::string_view value) -> void {
  if (name == "nonce") {
    result.nonce = value;
  } else if (name == "display") {
    result.display = value;
  } else if (name == "prompt") {
    result.prompt = value;
  } else if (name == "max_age") {
    result.max_age = value;
  } else if (name == "ui_locales") {
    result.ui_locales = value;
  } else if (name == "id_token_hint") {
    result.id_token_hint = value;
  } else if (name == "login_hint") {
    result.login_hint = value;
  } else if (name == "acr_values") {
    result.acr_values = value;
  } else if (name == "claims") {
    result.claims = value;
  } else if (name == "request") {
    result.request = value;
  } else if (name == "response_mode") {
    result.response_mode = value;
  }
}

// OpenID Connect Core 1.0 Section 3.1.2.1: collect a present OpenID Connect
// authentication parameter that the OAuth layer carries as an extension
auto append_extra_parameter(std::array<OAuthParameter, 11> &storage,
                            std::size_t &count, const std::string_view name,
                            const std::string_view value) -> void {
  if (!value.empty()) {
    storage[count] = OAuthParameter{.name = name, .value = value};
    count += 1;
  }
}

} // namespace

auto oidc_nonce() -> std::array<char, 43> { return oauth_random_token(); }

auto oidc_build_authentication_url(const std::string_view endpoint,
                                   const OIDCAuthenticationRequest &request,
                                   std::string &sink, const OIDCProfile profile)
    -> bool {
  // OpenID Connect Core 1.0 Section 3.1.2.1: the client identifier and the
  // redirection URI are REQUIRED, so a request missing either cannot yield a
  // usable URL
  if (request.client_id.empty() || request.redirect_uri.empty()) {
    return false;
  }

  // OpenID Connect Core 1.0 Section 3.1.2.1: scope is REQUIRED and must contain
  // the openid value, which is what makes the request an OpenID Connect one
  if (!space_list_contains(request.scope, "openid")) {
    return false;
  }

  if (!prompt_is_valid(request.prompt)) {
    return false;
  }

  if (!offline_access_is_valid(request.scope, request.prompt)) {
    return false;
  }

  if (!response_type_is_allowed(request.response_type, profile) ||
      (response_type_requires_nonce(request.response_type) &&
       request.nonce.empty())) {
    return false;
  }

  if (!pkce_is_valid(request.code_challenge, request.code_challenge_method,
                     profile)) {
    return false;
  }

  std::array<OAuthParameter, 11> extra_storage;
  std::size_t extra_count{0};
  append_extra_parameter(extra_storage, extra_count, "nonce", request.nonce);
  append_extra_parameter(extra_storage, extra_count, "display",
                         request.display);
  append_extra_parameter(extra_storage, extra_count, "prompt", request.prompt);
  append_extra_parameter(extra_storage, extra_count, "max_age",
                         request.max_age);
  append_extra_parameter(extra_storage, extra_count, "ui_locales",
                         request.ui_locales);
  append_extra_parameter(extra_storage, extra_count, "id_token_hint",
                         request.id_token_hint);
  append_extra_parameter(extra_storage, extra_count, "login_hint",
                         request.login_hint);
  append_extra_parameter(extra_storage, extra_count, "acr_values",
                         request.acr_values);
  append_extra_parameter(extra_storage, extra_count, "claims", request.claims);
  append_extra_parameter(extra_storage, extra_count, "request",
                         request.request);
  append_extra_parameter(extra_storage, extra_count, "response_mode",
                         request.response_mode);

  OAuthAuthorizationRequest base;
  base.client_id = request.client_id;
  base.redirect_uri = request.redirect_uri;
  base.scope = request.scope;
  base.state = request.state;
  base.code_challenge = request.code_challenge;
  base.code_challenge_method = request.code_challenge_method;
  base.response_type = request.response_type;
  base.request_uri = request.request_uri;
  base.extra =
      std::span<const OAuthParameter>{extra_storage.data(), extra_count};

  oauth_build_authorization_url(endpoint, base, sink);
  return true;
}

auto oidc_authorization_url(const std::string_view authorization_endpoint,
                            const std::string_view client_id,
                            const std::string_view redirect_uri,
                            const std::string_view state,
                            const std::string_view code_challenge,
                            const std::string_view nonce)
    -> std::optional<std::string> {
  // This convenience guarantees PKCE, so an empty code challenge, which would
  // silently produce a non-PKCE URL, is refused (RFC 7636 Section 4.3)
  if (code_challenge.empty()) {
    return std::nullopt;
  }

  OIDCAuthenticationRequest request;
  request.client_id = client_id;
  request.redirect_uri = redirect_uri;
  request.scope = "openid";
  request.response_type = "code";
  request.state = state;
  request.code_challenge = code_challenge;
  request.code_challenge_method = "S256";
  request.nonce = nonce;

  std::string url;
  if (!oidc_build_authentication_url(authorization_endpoint, request, url)) {
    return std::nullopt;
  }

  return url;
}

auto oidc_parse_authentication_request(const std::string_view query,
                                       std::string &storage,
                                       OIDCAuthenticationRequest &result,
                                       const OIDCProfile profile) -> bool {
  // Reset so a parameter absent from this query cannot retain a stale value
  // when the caller reuses the result across parses
  result = OIDCAuthenticationRequest{};

  OAuthAuthorizationRequest base;
  const auto parsed{oauth_parse_authorization_request(
      query, storage, base,
      [&result](const std::string_view name, const std::string_view value)
          -> void { assign_openid_parameter(result, name, value); })};
  if (!parsed) {
    return false;
  }

  result.client_id = base.client_id;
  result.redirect_uri = base.redirect_uri;
  result.scope = base.scope;
  result.response_type = base.response_type;
  result.state = base.state;
  result.code_challenge = base.code_challenge;
  result.code_challenge_method = base.code_challenge_method;
  result.request_uri = base.request_uri;

  // The same OpenID Connect well-formedness the builder enforces, so parsing
  // never accepts a request the module could not build: the client identifier
  // and redirection URI are REQUIRED, the scope must contain openid, a none
  // prompt must appear alone, offline_access cannot pair with a none prompt,
  // the response_type is REQUIRED and limited by the profile, a returned ID
  // Token requires a nonce, and the strict profile requires PKCE (OpenID
  // Connect Core 1.0 Section 3.1.2.1, Section 3.3.2.11, Section 11, RFC 9700
  // Section 2.1.1)
  return !result.client_id.empty() && !result.redirect_uri.empty() &&
         space_list_contains(result.scope, "openid") &&
         prompt_is_valid(result.prompt) &&
         offline_access_is_valid(result.scope, result.prompt) &&
         !result.response_type.empty() &&
         response_type_is_allowed(result.response_type, profile) &&
         !(response_type_requires_nonce(result.response_type) &&
           result.nonce.empty()) &&
         pkce_is_valid(result.code_challenge, result.code_challenge_method,
                       profile);
}

} // namespace sourcemeta::core
