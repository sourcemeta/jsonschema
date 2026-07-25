#include <sourcemeta/core/oauth_authorization.h>

#include <sourcemeta/core/html.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "oauth_authorization_parse.h"
#include "oauth_decode.h"
#include "oauth_syntax.h"

#include <functional>  // std::function
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

struct HttpAuthority {
  std::string_view host;
  std::string_view rest;
};

// Split an "http://" URI into its host and the remainder after the authority,
// discarding the port, without constructing a URI (RFC 3986 Section 3.2)
auto split_http_authority(const std::string_view value)
    -> std::optional<HttpAuthority> {
  static constexpr std::string_view prefix{"http://"};
  if (!value.starts_with(prefix)) {
    return std::nullopt;
  }

  const auto after{value.substr(prefix.size())};
  // RFC 3986 Section 3.2: the authority ends at the next "/", "?", or "#", or
  // the end of the URI, so the query and fragment are part of the compared
  // remainder rather than folded into the authority
  const auto path_position{after.find_first_of("/?#")};
  auto authority{after.substr(0, path_position)};
  const auto rest{path_position == std::string_view::npos
                      ? std::string_view{}
                      : after.substr(path_position)};

  // RFC 3986 Section 3.2.1: userinfo is delimited from the host by an "@". A
  // loopback redirect never carries userinfo, and allowing it invites host
  // confusion, since a redirect like "http://127.0.0.1:2@evil/cb" has its real
  // host after the "@" yet its authority begins with the loopback literal, so
  // an authority with userinfo does not qualify for the loopback exception
  if (authority.find('@') != std::string_view::npos) {
    return std::nullopt;
  }

  std::string_view host;
  if (authority.starts_with('[')) {
    const auto close{authority.find(']')};
    if (close == std::string_view::npos) {
      return std::nullopt;
    }

    host = authority.substr(0, close + 1);
  } else {
    host = authority.substr(0, authority.find(':'));
  }

  return HttpAuthority{.host = host, .rest = rest};
}

auto oauth_append_hidden_input(HTMLWriter &page, const std::string_view name,
                               const std::string_view value) -> void {
  page.input()
      .attribute("type", "hidden")
      .attribute("name", name)
      .attribute("value", value);
}

// The page follows the shape of the example in OAuth 2.0 Form Post Response
// Mode Appendix A, where "the action attribute of the form MUST be the
// Client's Redirection URI" and "the method of the form attribute MUST be
// POST" (Section 2)
auto oauth_open_form_post_page(HTMLWriter &page,
                               const std::string_view redirect_uri,
                               const std::string_view title) -> void {
  page.html();
  page.head();
  page.title(title);
  page.close();
  page.body().attribute("onload", "javascript:document.forms[0].submit()");
  page.form().attribute("method", "post").attribute("action", redirect_uri);
}

} // namespace

auto oauth_build_authorization_url(const std::string_view endpoint,
                                   const OAuthAuthorizationRequest &request,
                                   std::string &sink) -> void {
  // A lower bound covering the endpoint, the known keys and separators, and the
  // raw value lengths, over which percent-escaping only ever grows
  sink.reserve(sink.size() + endpoint.size() + request.client_id.size() +
               request.redirect_uri.size() + request.scope.size() +
               request.state.size() + request.code_challenge.size() +
               request.code_challenge_method.size() +
               request.request_uri.size() + request.dpop_jkt.size() + 128);
  sink.append(endpoint);
  // '?' opens a query the endpoint lacks, and the parameter appender continues
  // one the endpoint already carries (RFC 6749 Section 3.1)
  if (endpoint.find('?') == std::string_view::npos) {
    sink.push_back('?');
  }

  // RFC 6749 Section 3.1.1: the request selects the response type, defaulting
  // to the authorization code flow when the caller leaves it unset
  URI::append_query_parameter(
      sink, "response_type",
      request.response_type.empty() ? "code" : request.response_type);

  if (!request.client_id.empty()) {
    URI::append_query_parameter(sink, "client_id", request.client_id);
  }

  if (!request.redirect_uri.empty()) {
    URI::append_query_parameter(sink, "redirect_uri", request.redirect_uri);
  }

  if (!request.scope.empty()) {
    URI::append_query_parameter(sink, "scope", request.scope);
  }

  if (!request.state.empty()) {
    URI::append_query_parameter(sink, "state", request.state);
  }

  if (!request.code_challenge.empty()) {
    URI::append_query_parameter(sink, "code_challenge", request.code_challenge);
    // RFC 7636 Section 4.3: the method qualifies a challenge, so it is
    // meaningless and omitted when no challenge is present
    if (!request.code_challenge_method.empty()) {
      URI::append_query_parameter(sink, "code_challenge_method",
                                  request.code_challenge_method);
    }
  }

  if (!request.request_uri.empty()) {
    URI::append_query_parameter(sink, "request_uri", request.request_uri);
  }

  if (!request.dpop_jkt.empty()) {
    URI::append_query_parameter(sink, "dpop_jkt", request.dpop_jkt);
  }

  for (const auto &resource : request.resources) {
    URI::append_query_parameter(sink, resource.name, resource.value);
  }

  for (const auto &parameter : request.extra) {
    URI::append_query_parameter(sink, parameter.name, parameter.value);
  }
}

auto oauth_parse_authorization_request(
    const std::string_view query, std::string &storage,
    OAuthAuthorizationRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool {
  return oauth_parse_authorization_into(query, storage, result, on_other,
                                        false);
}

auto oauth_redirect_uri_matches(const std::string_view registered,
                                const std::string_view presented,
                                const OAuthProfile profile) -> bool {
  // RFC 6749 Section 3.1.2.3: the default is an exact match of the two URIs
  if (registered == presented) {
    return true;
  }

  const auto registered_parts{split_http_authority(registered)};
  const auto presented_parts{split_http_authority(presented)};
  if (!registered_parts.has_value() || !presented_parts.has_value()) {
    return false;
  }

  // RFC 8252 Section 7.3: for a loopback http redirect the client may vary the
  // port, so the two match when only the port differs. RFC 8252 Section 8.3 and
  // OAuth 2.1 Section 8.4.2 keep "localhost" out of the loopback set under the
  // strict profile
  const auto host{registered_parts.value().host};
  const bool loopback{
      host == "127.0.0.1" || host == "[::1]" ||
      (profile == OAuthProfile::Compatible && host == "localhost")};
  return loopback && host == presented_parts.value().host &&
         registered_parts.value().rest == presented_parts.value().rest;
}

auto oauth_is_private_use_scheme(const std::string_view scheme) noexcept
    -> bool {
  // RFC 3986 Section 3.1: "scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "."
  // )". A reverse domain name has no empty label, so a leading or trailing
  // period is rejected here, and an interior empty label below
  if (scheme.empty() || !is_alpha(scheme.front()) || scheme.back() == '.') {
    return false;
  }

  // RFC 8252 Section 7.1 and Section 8.4: a private-use scheme is a reverse
  // domain name, so it must contain at least one period and no empty label
  bool has_period{false};
  char previous{'\0'};
  for (const auto character : scheme) {
    if (!is_alphanum(character) && character != '+' && character != '-' &&
        character != '.') {
      return false;
    }

    if (character == '.') {
      if (previous == '.') {
        return false;
      }

      has_period = true;
    }

    previous = character;
  }

  return has_period;
}

auto oauth_build_authorization_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, const OAuthResponseMode mode,
    std::string &sink) -> bool {
  // A form post response is an HTML page rather than a redirect (OAuth 2.0
  // Form Post Response Mode Section 2)
  if (mode == OAuthResponseMode::FormPost) {
    return false;
  }

  // A success response carries the code, and RFC 9207 Section 2 constrains the
  // issuer syntax when one is echoed
  if (response.code.empty()) {
    return false;
  }

  // RFC 6749 Section 3.1.2: a redirection endpoint "MUST NOT include a fragment
  // component", and appending the query after one would place the parameters
  // inside the fragment rather than the query
  if (redirect_uri.find('#') != std::string_view::npos) {
    return false;
  }

  if (!response.iss.empty() && !oauth_is_issuer_identifier(response.iss)) {
    return false;
  }

  const auto opener{mode == OAuthResponseMode::Fragment ? '#' : '?'};
  sink.reserve(sink.size() + redirect_uri.size() + response.code.size() +
               response.state.size() + response.iss.size() + 32);
  sink.append(redirect_uri);
  // In the fragment response mode the parameters are "encoded in the fragment
  // added to the redirect_uri when redirecting back to the Client" (OAuth 2.0
  // Multiple Response Types Section 2.1), while a query joins any existing one
  if (mode == OAuthResponseMode::Fragment ||
      redirect_uri.find('?') == std::string_view::npos) {
    sink.push_back(opener);
  }

  URI::append_query_parameter(sink, "code", response.code, opener);
  // RFC 6749 Section 4.1.2: state is returned when the request carried one,
  // which the caller reflects by setting it
  if (!response.state.empty()) {
    URI::append_query_parameter(sink, "state", response.state, opener);
  }

  if (!response.iss.empty()) {
    URI::append_query_parameter(sink, "iss", response.iss, opener);
  }

  return true;
}

auto oauth_build_authorization_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink) -> bool {
  return oauth_build_authorization_redirect(redirect_uri, response,
                                            OAuthResponseMode::Query, sink);
}

auto oauth_build_authorization_error_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, const OAuthResponseMode mode,
    std::string &sink) -> bool {
  // A form post response is an HTML page rather than a redirect (OAuth 2.0
  // Form Post Response Mode Section 2)
  if (mode == OAuthResponseMode::FormPost) {
    return false;
  }

  // RFC 6749 Section 4.1.2.1: this builder must not be called when the redirect
  // URI or client identifier failed validation, since the error is then shown
  // to the resource owner rather than redirected. An error response carries the
  // error code
  if (response.error.empty()) {
    return false;
  }

  // RFC 6749 Section 3.1.2: a redirection endpoint "MUST NOT include a fragment
  // component", and appending the query after one would place the parameters
  // inside the fragment rather than the query
  if (redirect_uri.find('#') != std::string_view::npos) {
    return false;
  }

  if (!response.iss.empty() && !oauth_is_issuer_identifier(response.iss)) {
    return false;
  }

  const auto opener{mode == OAuthResponseMode::Fragment ? '#' : '?'};
  sink.reserve(sink.size() + redirect_uri.size() + response.error.size() +
               response.error_description.size() + response.error_uri.size() +
               response.state.size() + response.iss.size() + 64);
  sink.append(redirect_uri);
  // In the fragment response mode the parameters are "encoded in the fragment
  // added to the redirect_uri when redirecting back to the Client" (OAuth 2.0
  // Multiple Response Types Section 2.1), while a query joins any existing one
  if (mode == OAuthResponseMode::Fragment ||
      redirect_uri.find('?') == std::string_view::npos) {
    sink.push_back(opener);
  }

  URI::append_query_parameter(sink, "error", response.error, opener);
  if (!response.error_description.empty()) {
    URI::append_query_parameter(sink, "error_description",
                                response.error_description, opener);
  }

  if (!response.error_uri.empty()) {
    URI::append_query_parameter(sink, "error_uri", response.error_uri, opener);
  }

  if (!response.state.empty()) {
    URI::append_query_parameter(sink, "state", response.state, opener);
  }

  if (!response.iss.empty()) {
    URI::append_query_parameter(sink, "iss", response.iss, opener);
  }

  return true;
}

auto oauth_build_authorization_error_redirect(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink) -> bool {
  return oauth_build_authorization_error_redirect(
      redirect_uri, response, OAuthResponseMode::Query, sink);
}

auto oauth_default_response_mode(const std::string_view response_type)
    -> std::optional<OAuthResponseMode> {
  // "If a Response Type contains one of more space characters (%20), it is
  // compared as a space-delimited list of values in which the order of values
  // does not matter" (OAuth 2.0 Multiple Response Types Section 1.2)
  auto has_code{false};
  auto has_token{false};
  auto has_id_token{false};
  auto has_none{false};
  std::size_t position{0};
  while (true) {
    const auto space{response_type.find(' ', position)};
    const auto value{space == std::string_view::npos
                         ? response_type.substr(position)
                         : response_type.substr(position, space - position)};
    if (value == "code" && !has_code) {
      has_code = true;
    } else if (value == "token" && !has_token) {
      has_token = true;
    } else if (value == "id_token" && !has_id_token) {
      has_id_token = true;
    } else if (value == "none" && !has_none) {
      has_none = true;
    } else {
      return std::nullopt;
    }

    if (space == std::string_view::npos) {
      break;
    }

    position = space + 1;
  }

  // The none response type is registered on its own and no combination with it
  // is (OAuth 2.0 Multiple Response Types Sections 4 and 5)
  if (has_none && (has_code || has_token || has_id_token)) {
    return std::nullopt;
  }

  // The code and none response types default to "the query encoding" while
  // the token and id_token response types and every registered combination
  // default to "the fragment encoding" (OAuth 2.0 Multiple Response Types
  // Sections 2.1, 3, 4, and 5)
  return has_token || has_id_token ? OAuthResponseMode::Fragment
                                   : OAuthResponseMode::Query;
}

auto oauth_is_response_mode_allowed(const std::string_view response_type,
                                    const OAuthResponseMode mode) -> bool {
  const auto default_mode{oauth_default_response_mode(response_type)};
  if (!default_mode.has_value()) {
    return false;
  }

  // Every response type that defaults to the fragment encoding states that
  // "the query encoding MUST NOT be used" (OAuth 2.0 Multiple Response Types
  // Sections 3 and 5), and "in no case should a set of Authorization Response
  // parameters whose default Response Mode is the fragment encoding be encoded
  // using the query encoding" (Section 7)
  return mode != OAuthResponseMode::Query ||
         default_mode.value() != OAuthResponseMode::Fragment;
}

auto oauth_build_authorization_form_post(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink,
    const std::string_view title) -> bool {
  // A success response carries the code, and RFC 9207 Section 2 constrains the
  // issuer syntax when one is echoed
  if (response.code.empty()) {
    return false;
  }

  // RFC 6749 Section 3.1.2: a redirection endpoint "MUST NOT include a
  // fragment component"
  if (redirect_uri.find('#') != std::string_view::npos) {
    return false;
  }

  if (!response.iss.empty() && !oauth_is_issuer_identifier(response.iss)) {
    return false;
  }

  HTMLWriter page;
  page.reserve(redirect_uri.size() + title.size() + response.code.size() +
               response.state.size() + response.iss.size() + 256);
  oauth_open_form_post_page(page, redirect_uri, title);
  oauth_append_hidden_input(page, "code", response.code);
  if (!response.state.empty()) {
    oauth_append_hidden_input(page, "state", response.state);
  }

  if (!response.iss.empty()) {
    oauth_append_hidden_input(page, "iss", response.iss);
  }

  // Close the form, the body, and the document
  page.close().close().close();
  sink.append(page.str());
  return true;
}

auto oauth_build_authorization_error_form_post(
    const std::string_view redirect_uri,
    const OAuthAuthorizationResponse &response, std::string &sink,
    const std::string_view title) -> bool {
  // RFC 6749 Section 4.1.2.1: this builder must not be called when the redirect
  // URI or client identifier failed validation, since the error is then shown
  // to the resource owner rather than redirected. An error response carries the
  // error code
  if (response.error.empty()) {
    return false;
  }

  // RFC 6749 Section 3.1.2: a redirection endpoint "MUST NOT include a
  // fragment component"
  if (redirect_uri.find('#') != std::string_view::npos) {
    return false;
  }

  if (!response.iss.empty() && !oauth_is_issuer_identifier(response.iss)) {
    return false;
  }

  HTMLWriter page;
  page.reserve(redirect_uri.size() + title.size() + response.error.size() +
               response.error_description.size() + response.error_uri.size() +
               response.state.size() + response.iss.size() + 256);
  oauth_open_form_post_page(page, redirect_uri, title);
  oauth_append_hidden_input(page, "error", response.error);
  if (!response.error_description.empty()) {
    oauth_append_hidden_input(page, "error_description",
                              response.error_description);
  }

  if (!response.error_uri.empty()) {
    oauth_append_hidden_input(page, "error_uri", response.error_uri);
  }

  if (!response.state.empty()) {
    oauth_append_hidden_input(page, "state", response.state);
  }

  if (!response.iss.empty()) {
    oauth_append_hidden_input(page, "iss", response.iss);
  }

  // Close the form, the body, and the document
  page.close().close().close();
  sink.append(page.str());
  return true;
}

auto oauth_parse_authorization_response(const std::string_view query,
                                        std::string &storage,
                                        OAuthAuthorizationResponse &result)
    -> bool {
  result = {};
  // A single up-front reserve keeps every later decode from reallocating the
  // arena and dangling an earlier borrowed view (design convention 1)
  storage.reserve(storage.size() + query.size());
  const URI::Query parsed{query};
  bool has_code{false};
  bool has_state{false};
  bool has_iss{false};
  bool has_error{false};
  bool has_error_description{false};
  bool has_error_uri{false};
  for (const auto &parameter : parsed) {
    // RFC 6749 Appendix B: the application/x-www-form-urlencoded format encodes
    // names too, so a name is decoded before it is recognized, the same rule
    // the request parsers apply, and a malformed escape fails the parse
    std::string_view name;
    if (!oauth_form_decode_into(parameter.first, storage, name)) {
      return false;
    }

    const auto value{parameter.second};
    // RFC 6749 Section 3.1: "Request and response parameters MUST NOT be
    // included more than once", so a duplicate is malformed, while an
    // unrecognized parameter is ignored (RFC 6749 Section 4.1.2)
    if (name == "code") {
      if (has_code) {
        return false;
      }

      has_code = true;
      if (!oauth_form_decode_into(value, storage, result.code)) {
        return false;
      }
    } else if (name == "state") {
      if (has_state) {
        return false;
      }

      has_state = true;
      if (!oauth_form_decode_into(value, storage, result.state)) {
        return false;
      }
    } else if (name == "iss") {
      if (has_iss) {
        return false;
      }

      has_iss = true;
      if (!oauth_form_decode_into(value, storage, result.iss)) {
        return false;
      }
    } else if (name == "error") {
      if (has_error) {
        return false;
      }

      has_error = true;
      if (!oauth_form_decode_into(value, storage, result.error)) {
        return false;
      }
    } else if (name == "error_description") {
      if (has_error_description) {
        return false;
      }

      has_error_description = true;
      if (!oauth_form_decode_into(value, storage, result.error_description)) {
        return false;
      }
    } else if (name == "error_uri") {
      if (has_error_uri) {
        return false;
      }

      has_error_uri = true;
      if (!oauth_form_decode_into(value, storage, result.error_uri)) {
        return false;
      }
    }
  }

  return true;
}

} // namespace sourcemeta::core
