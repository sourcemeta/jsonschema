#include <sourcemeta/core/oauth_client_authentication.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include "oauth_decode.h"
#include "oauth_encode.h"

#include <cstddef>     // std::size_t
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {

auto assign_secure_scalar(const std::string_view value, SecureString &storage,
                          bool &seen, std::string_view &field) -> bool {
  // RFC 6749 Section 3.2: "Parameters sent without a value MUST be treated as
  // if they were omitted", so an empty occurrence neither counts as a duplicate
  // nor marks the parameter present
  if (value.empty()) {
    return true;
  }

  // RFC 6749 Section 3.2: a recognized parameter must not appear twice
  if (seen) {
    return false;
  }

  seen = true;
  return oauth_form_decode_into_secure(value, storage, field);
}

} // namespace

auto oauth_client_secret_basic(const std::string_view client_id,
                               const std::string_view client_secret,
                               SecureString &sink) -> void {
  // RFC 6749 Section 2.3.1: each half is percent-encoded before it is joined
  // with a colon, so a colon inside the identifier or secret cannot be mistaken
  // for the delimiter, and only then is the pair Base64 encoded
  SecureString credential;
  credential.reserve(client_id.size() + client_secret.size() + 1);
  URI::escape(client_id, credential);
  credential.push_back(':');
  URI::escape(client_secret, credential);

  sink.append("Basic ");
  base64_encode(std::string_view{credential}, sink);
}

auto oauth_client_secret_post(const std::string_view client_id,
                              const std::string_view client_secret,
                              SecureString &sink) -> void {
  oauth_append_form_parameter(sink, "client_id", client_id);
  oauth_append_form_parameter(sink, "client_secret", client_secret);
}

auto oauth_client_id_only(const std::string_view client_id, SecureString &sink)
    -> void {
  oauth_append_form_parameter(sink, "client_id", client_id);
}

auto oauth_parse_client_authentication(const std::string_view authorization,
                                       const std::string_view body,
                                       SecureString &storage,
                                       OAuthClientCredentials &credentials)
    -> bool {
  credentials = {};
  credentials.method = OAuthClientAuthenticationMethod::None;
  // A single up-front reserve keeps every later decode from reallocating the
  // arena and dangling an earlier borrowed view. Every decoded value is at most
  // its raw length, so the two inputs bound the total
  storage.reserve(storage.size() + authorization.size() + body.size());

  bool basic_present{false};
  std::string_view basic_id;
  std::string_view basic_secret;
  const auto scheme_end{authorization.find(' ')};
  if (scheme_end != std::string_view::npos &&
      equals_ignore_case(authorization.substr(0, scheme_end), "Basic")) {
    basic_present = true;
    // RFC 7235 Section 2.1: the scheme is followed by one or more spaces before
    // the credential
    auto token_start{scheme_end};
    while (token_start < authorization.size() &&
           authorization[token_start] == ' ') {
      token_start += 1;
    }

    // RFC 6749 Section 2.3.1: the credential is the Base64 of the percent
    // encoded identifier and secret joined by a colon, so a decode failure or a
    // missing colon is a malformed request. The decoded credential is secret,
    // so it lands in a local wiping string
    SecureString credential;
    if (!base64_decode(authorization.substr(token_start), credential)) {
      return false;
    }

    const std::string_view decoded{credential};
    const auto colon{decoded.find(':')};
    if (colon == std::string_view::npos ||
        !oauth_form_decode_into_secure(decoded.substr(0, colon), storage,
                                       basic_id) ||
        !oauth_form_decode_into_secure(decoded.substr(colon + 1), storage,
                                       basic_secret)) {
      return false;
    }
  }

  bool has_client_id{false};
  bool has_client_secret{false};
  bool has_assertion{false};
  bool has_assertion_type{false};
  std::string_view body_client_id;
  std::string_view body_client_secret;
  std::string_view body_assertion;
  std::string_view body_assertion_type;
  const URI::Query parsed{body};
  for (const auto &parameter : parsed) {
    // RFC 6749 Appendix B: the application/x-www-form-urlencoded format encodes
    // names too, so a name is decoded before it is recognized, and a malformed
    // escape fails the parse
    std::string_view name;
    if (!oauth_form_decode_into_secure(parameter.first, storage, name)) {
      return false;
    }

    const auto value{parameter.second};
    bool valid{true};
    if (name == "client_id") {
      valid =
          assign_secure_scalar(value, storage, has_client_id, body_client_id);
    } else if (name == "client_secret") {
      valid = assign_secure_scalar(value, storage, has_client_secret,
                                   body_client_secret);
    } else if (name == "client_assertion") {
      valid =
          assign_secure_scalar(value, storage, has_assertion, body_assertion);
    } else if (name == "client_assertion_type") {
      valid = assign_secure_scalar(value, storage, has_assertion_type,
                                   body_assertion_type);
    }

    if (!valid) {
      return false;
    }
  }

  // RFC 6749 Section 2.3 and RFC 7521 Section 4.2.1: a request must use exactly
  // one authentication mechanism, and the assertion parameters count as one, so
  // presenting more than one is rejected. The caller chooses the error code,
  // which RFC 7521 Section 4.2.1 makes invalid_client when the collision
  // involves the assertion mechanism and RFC 6749 Section 5.2 makes
  // invalid_request otherwise. A bare body client_id is identification, not a
  // mechanism (RFC 6749 Section 3.2.1)
  const bool assertion_present{has_assertion || has_assertion_type};
  const auto mechanisms{static_cast<int>(basic_present) +
                        static_cast<int>(has_client_secret) +
                        static_cast<int>(assertion_present)};
  if (mechanisms > 1) {
    return false;
  }

  // RFC 6749 Section 5.2: a Basic username that conflicts with a body client_id
  // is a malformed request, so the two must identify the same client
  if (basic_present && has_client_id && basic_id != body_client_id) {
    return false;
  }

  // RFC 7521 Section 4.2: the assertion mechanism needs both parameters
  if (assertion_present && !(has_assertion && has_assertion_type)) {
    return false;
  }

  // RFC 6749 Section 2.3.1: client_secret_post carries both the identifier and
  // the secret, so a body secret with no client_id is an incomplete mechanism.
  // A Basic secret has its own identifier, and a Basic and body secret together
  // are already rejected as two mechanisms above
  if (has_client_secret && !has_client_id) {
    return false;
  }

  if (basic_present) {
    credentials.method = OAuthClientAuthenticationMethod::Basic;
    credentials.client_id = basic_id;
    credentials.client_secret = basic_secret;
  } else if (has_client_secret) {
    credentials.method = OAuthClientAuthenticationMethod::Post;
    credentials.client_id = body_client_id;
    credentials.client_secret = body_client_secret;
  } else if (assertion_present) {
    credentials.method = OAuthClientAuthenticationMethod::Assertion;
    credentials.client_id = body_client_id;
    credentials.assertion = body_assertion;
    credentials.assertion_type = body_assertion_type;
  } else if (has_client_id) {
    credentials.method = OAuthClientAuthenticationMethod::Public;
    credentials.client_id = body_client_id;
  }

  return true;
}

} // namespace sourcemeta::core
