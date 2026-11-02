#ifndef SOURCEMETA_CORE_OAUTH_AUTHORIZATION_PARSE_H_
#define SOURCEMETA_CORE_OAUTH_AUTHORIZATION_PARSE_H_

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/oauth_authorization.h>
#include <sourcemeta/core/uri.h>

#include "oauth_decode.h"

#include <functional>  // std::function
#include <string_view> // std::string_view
#include <type_traits> // std::is_same_v

namespace sourcemeta::core {

// Decode one value into the arena, borrowing when the arena is an ordinary
// string and always appending when it is a wiping one, so a body that carries
// secrets keeps its decoded values in wiping storage
template <typename Storage>
inline auto oauth_authorization_decode(const std::string_view value,
                                       Storage &storage,
                                       std::string_view &result) -> bool {
  if constexpr (std::is_same_v<Storage, SecureString>) {
    return oauth_form_decode_into_secure(value, storage, result);
  } else {
    return oauth_form_decode_into(value, storage, result);
  }
}

template <typename Storage>
inline auto oauth_authorization_assign_scalar(const std::string_view value,
                                              Storage &storage, bool &seen,
                                              std::string_view &field) -> bool {
  // RFC 6749 Section 3.1: "Parameters sent without a value MUST be treated as
  // if they were omitted", so an empty occurrence neither counts as a duplicate
  // nor marks the parameter present
  if (value.empty()) {
    return true;
  }

  // RFC 6749 Section 3.1: "Request and response parameters MUST NOT be included
  // more than once", so a second occurrence of a recognized parameter fails
  if (seen) {
    return false;
  }

  seen = true;
  return oauth_authorization_decode(value, storage, field);
}

// The shared authorization request parse (RFC 6749 Section 4.1.1). The storage
// type selects the arena posture, and reject_request_uri turns the presence of
// a request_uri parameter into a failure, which a pushed authorization request
// requires (RFC 9126 Section 2.1)
template <typename Storage>
inline auto oauth_parse_authorization_into(
    const std::string_view query, Storage &storage,
    OAuthAuthorizationRequest &result,
    const std::function<void(std::string_view, std::string_view)> &on_other,
    const bool reject_request_uri) -> bool {
  result = {};
  // A single up-front reserve keeps every later decode from reallocating the
  // arena and dangling an earlier borrowed view (design convention 1)
  storage.reserve(storage.size() + query.size());
  const URI::Query parsed{query};
  bool has_response_type{false};
  bool has_client_id{false};
  bool has_redirect_uri{false};
  bool has_scope{false};
  bool has_state{false};
  bool has_code_challenge{false};
  bool has_code_challenge_method{false};
  bool has_request_uri{false};
  bool has_dpop_jkt{false};
  for (const auto &parameter : parsed) {
    // RFC 6749 Appendix B: the application/x-www-form-urlencoded format encodes
    // names too, so a name is decoded before it is recognized, and a malformed
    // escape fails the parse
    std::string_view name;
    if (!oauth_authorization_decode(parameter.first, storage, name)) {
      return false;
    }

    const auto value{parameter.second};
    bool valid{true};
    if (name == "response_type") {
      valid = oauth_authorization_assign_scalar(
          value, storage, has_response_type, result.response_type);
    } else if (name == "client_id") {
      valid = oauth_authorization_assign_scalar(value, storage, has_client_id,
                                                result.client_id);
    } else if (name == "redirect_uri") {
      valid = oauth_authorization_assign_scalar(
          value, storage, has_redirect_uri, result.redirect_uri);
    } else if (name == "scope") {
      valid = oauth_authorization_assign_scalar(value, storage, has_scope,
                                                result.scope);
    } else if (name == "state") {
      valid = oauth_authorization_assign_scalar(value, storage, has_state,
                                                result.state);
    } else if (name == "code_challenge") {
      valid = oauth_authorization_assign_scalar(
          value, storage, has_code_challenge, result.code_challenge);
    } else if (name == "code_challenge_method") {
      valid = oauth_authorization_assign_scalar(value, storage,
                                                has_code_challenge_method,
                                                result.code_challenge_method);
    } else if (name == "request_uri") {
      // RFC 9126 Section 2.1: a pushed request MUST NOT provide request_uri, so
      // its mere presence, whatever its value, fails the parse
      if (reject_request_uri) {
        return false;
      }

      valid = oauth_authorization_assign_scalar(value, storage, has_request_uri,
                                                result.request_uri);
    } else if (name == "dpop_jkt") {
      valid = oauth_authorization_assign_scalar(value, storage, has_dpop_jkt,
                                                result.dpop_jkt);
    } else {
      // Repeatable resource indicators (RFC 8707) and extension parameters are
      // surfaced with their decoded value rather than stored on the result
      std::string_view decoded;
      valid = oauth_authorization_decode(value, storage, decoded);
      if (valid) {
        on_other(name, decoded);
      }
    }

    if (!valid) {
      return false;
    }
  }

  // RFC 7636 Section 4.3: "If the client does not send the
  // code_challenge_method parameter ... the default code_challenge_method value
  // is plain", which the strict profile must still see in order to reject
  if (has_code_challenge && !has_code_challenge_method) {
    result.code_challenge_method = "plain";
  }

  return true;
}

} // namespace sourcemeta::core

#endif
