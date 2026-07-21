#include <sourcemeta/core/oauth_par.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_authorization.h>
#include <sourcemeta/core/oauth_random.h>
#include <sourcemeta/core/uri.h>

#include "oauth_authorization_parse.h"
#include "oauth_encode.h"
#include "oauth_json.h"

#include <chrono>      // std::chrono::seconds
#include <cstdint>     // std::int64_t
#include <functional>  // std::function
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_REQUEST_URI{JSON::Object::hash("request_uri"sv)};
const auto HASH_EXPIRES_IN{JSON::Object::hash("expires_in"sv)};

// RFC 9126 Section 2.2: the recommended request URI form, a URN whose final
// segment is the random reference
constexpr std::string_view REQUEST_URI_PREFIX{
    "urn:ietf:params:oauth:request_uri:"};

} // namespace

auto oauth_build_par_request(const OAuthAuthorizationRequest &request,
                             SecureString &sink) -> void {
  // RFC 6749 Section 4.1.1: the authorization code flow is the response type
  // this builder assumes when the request names none, and the client may
  // override it
  oauth_append_form_parameter(sink, "response_type",
                              request.response_type.empty()
                                  ? std::string_view{"code"}
                                  : request.response_type);
  if (!request.redirect_uri.empty()) {
    oauth_append_form_parameter(sink, "redirect_uri", request.redirect_uri);
  }

  if (!request.scope.empty()) {
    oauth_append_form_parameter(sink, "scope", request.scope);
  }

  if (!request.state.empty()) {
    oauth_append_form_parameter(sink, "state", request.state);
  }

  if (!request.code_challenge.empty()) {
    oauth_append_form_parameter(sink, "code_challenge", request.code_challenge);
    // RFC 7636 Section 4.3: the method qualifies a challenge, so it is omitted
    // when no challenge is present
    if (!request.code_challenge_method.empty()) {
      oauth_append_form_parameter(sink, "code_challenge_method",
                                  request.code_challenge_method);
    }
  }

  if (!request.dpop_jkt.empty()) {
    oauth_append_form_parameter(sink, "dpop_jkt", request.dpop_jkt);
  }

  // RFC 9126 Section 2.1: a pushed request MUST NOT provide request_uri, so it
  // is dropped even when a caller routes it through the repeatable or extension
  // parameters rather than the dedicated field
  for (const auto &resource : request.resources) {
    if (resource.name != "request_uri") {
      oauth_append_form_parameter(sink, resource.name, resource.value);
    }
  }

  for (const auto &parameter : request.extra) {
    if (parameter.name != "request_uri") {
      oauth_append_form_parameter(sink, parameter.name, parameter.value);
    }
  }
}

auto oauth_build_par_authorization_url(const std::string_view endpoint,
                                       const std::string_view client_id,
                                       const std::string_view request_uri,
                                       std::string &sink) -> void {
  // A lower bound over the endpoint, the two keys and separators, and the raw
  // value lengths, over which percent-escaping only ever grows
  sink.reserve(sink.size() + endpoint.size() + client_id.size() +
               request_uri.size() + 128);
  sink.append(endpoint);
  // The separator before the first parameter: '?' opens a query the endpoint
  // lacks, nothing when the endpoint already ends its query, and '&' continues
  // one (RFC 6749 Section 3.1)
  char separator{'?'};
  if (endpoint.find('?') != std::string_view::npos) {
    separator =
        (endpoint.empty() || endpoint.back() == '?' || endpoint.back() == '&')
            ? '\0'
            : '&';
  }

  if (separator != '\0') {
    sink.push_back(separator);
  }

  sink.append("client_id=");
  URI::escape(client_id, sink);
  sink.append("&request_uri=");
  URI::escape(request_uri, sink);
}

OAuthPARResponse::OAuthPARResponse(const JSON &data) : data_{&data} {}

auto OAuthPARResponse::request_uri() const -> std::optional<std::string_view> {
  return oauth_json_string_member(*this->data_, "request_uri"sv,
                                  HASH_REQUEST_URI);
}

auto OAuthPARResponse::expires_in() const
    -> std::optional<std::chrono::seconds> {
  // RFC 9126 Section 2.2: expires_in is a positive integer, so a zero or
  // negative value is malformed and reported as absent
  const auto value{
      oauth_json_seconds_member(*this->data_, "expires_in"sv, HASH_EXPIRES_IN)};
  return (value.has_value() && value.value() > std::chrono::seconds{0})
             ? value
             : std::nullopt;
}

auto OAuthPARResponse::data() const -> const JSON & { return *this->data_; }

auto oauth_parse_par_request(
    const std::string_view body, SecureString &storage,
    OAuthAuthorizationRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other)
    -> bool {
  // RFC 9126 Section 2.1: "The request_uri authorization request parameter is
  // one exception, and it MUST NOT be provided", so the shared parse rejects it
  // on presence, whatever its value, and the body decodes into wiping storage
  // because it may carry a client secret
  return oauth_parse_authorization_into(body, storage, result, on_other, true);
}

auto oauth_par_request_uri() -> std::string {
  const auto reference{oauth_random_token()};
  std::string result;
  result.reserve(REQUEST_URI_PREFIX.size() + reference.size());
  result.append(REQUEST_URI_PREFIX);
  result.append(reference.data(), reference.size());
  return result;
}

auto oauth_make_par_response(const std::string_view request_uri,
                             const std::chrono::seconds expires_in)
    -> std::optional<JSON> {
  // RFC 9126 Section 2.2: request_uri and a positive expires_in lifetime are
  // REQUIRED, so a missing or non-positive one yields no document
  if (request_uri.empty() || expires_in <= std::chrono::seconds{0}) {
    return std::nullopt;
  }

  auto response{JSON::make_object()};
  response.assign_assume_new("request_uri", JSON{request_uri},
                             HASH_REQUEST_URI);
  response.assign_assume_new(
      "expires_in", JSON{static_cast<std::int64_t>(expires_in.count())},
      HASH_EXPIRES_IN);
  return response;
}

auto oauth_par_dpop_binding(
    const std::string_view dpop_jkt,
    const std::optional<std::string_view> proof_thumbprint)
    -> std::optional<std::string_view> {
  const bool has_jkt{!dpop_jkt.empty()};
  const bool has_proof{proof_thumbprint.has_value() &&
                       !proof_thumbprint.value().empty()};
  if (has_jkt && has_proof) {
    // RFC 9449 Section 10.1: with both mechanisms the request MUST be rejected
    // when the thumbprint in dpop_jkt does not match the DPoP header key
    if (!secure_equals(dpop_jkt, proof_thumbprint.value())) {
      return std::nullopt;
    }

    return dpop_jkt;
  }

  if (has_jkt) {
    return dpop_jkt;
  }

  if (has_proof) {
    return proof_thumbprint;
  }

  return std::string_view{};
}

} // namespace sourcemeta::core
