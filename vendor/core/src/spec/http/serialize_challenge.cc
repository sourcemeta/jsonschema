#include <sourcemeta/core/http.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>   // std::ranges::all_of
#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

constexpr std::string_view CHALLENGE_SCHEME_SEPARATOR{" "};
constexpr std::string_view CHALLENGE_PARAMETER_SEPARATOR{", "};
constexpr std::string_view CHALLENGE_PARAMETER_EQUALS{"="};

// RFC 6749 Appendix A: NQCHAR = %x21 / %x23-5B / %x5D-7E
auto is_nqchar(const char character) noexcept -> bool {
  const auto value{static_cast<unsigned char>(character)};
  return value == 0x21 || (value >= 0x23 && value <= 0x5B) ||
         (value >= 0x5D && value <= 0x7E);
}

// RFC 6749 Appendix A: NQSCHAR = %x20-21 / %x23-5B / %x5D-7E
auto is_nqschar(const char character) noexcept -> bool {
  const auto value{static_cast<unsigned char>(character)};
  return (value >= 0x20 && value <= 0x21) || (value >= 0x23 && value <= 0x5B) ||
         (value >= 0x5D && value <= 0x7E);
}

// RFC 6749 Appendix A.4: scope = scope-token *( SP scope-token ), where
// scope-token = 1*NQCHAR. The space delimits two values rather than being
// content, so it can neither open nor close the list nor stand doubled, which
// is what RFC 6750 §3 means by admitting it only "for delimiters between
// scope values"
auto is_scope(const std::string_view value) noexcept -> bool {
  if (value.empty() || value.front() == ' ' || value.back() == ' ') {
    return false;
  }

  bool previous_was_space{false};
  for (const auto character : value) {
    if (character == ' ') {
      if (previous_was_space) {
        return false;
      }

      previous_was_space = true;
      continue;
    }

    if (!is_nqchar(character)) {
      return false;
    }

    previous_was_space = false;
  }

  return true;
}

// The parameters RFC 6750 §3 constrains beyond the RFC 9110 grammar, which
// only bind when the challenge carries the scheme that specification defines
auto bearer_parameter_valid(const std::string_view name,
                            const std::string_view value) -> bool {
  if (sourcemeta::core::equals_ignore_case(name, "scope")) {
    return is_scope(value);
  }

  // RFC 6749 Appendix A.7 and A.8: error = 1*NQSCHAR and
  // error-description = 1*NQSCHAR, so neither may be empty
  if (sourcemeta::core::equals_ignore_case(name, "error") ||
      sourcemeta::core::equals_ignore_case(name, "error_description")) {
    return !value.empty() && std::ranges::all_of(value, is_nqschar);
  }

  // RFC 6749 Appendix A.9: error-uri = URI-reference, which RFC 6750 §3 notes
  // "thus MUST NOT include characters outside the set %x21 / %x23-5B /
  // %x5D-7E", so the grammar is checked rather than only its consequence.
  // Alone among the four it carries no repetition bound, and RFC 3986 §4.1
  // admits an empty reference through a relative one whose path is empty, so
  // this value may be empty where the others may not
  if (sourcemeta::core::equals_ignore_case(name, "error_uri")) {
    return std::ranges::all_of(value, is_nqchar) &&
           sourcemeta::core::URI::is_uri_reference(value);
  }

  return true;
}

auto required_size(const sourcemeta::core::HTTPChallenge &challenge) noexcept
    -> std::size_t {
  std::size_t size{challenge.scheme.size()};
  if (challenge.token68.has_value()) {
    return size + CHALLENGE_SCHEME_SEPARATOR.size() + challenge.token68->size();
  }

  for (const auto &[name, value] : challenge.parameters) {
    // Two quotes, and room for escaping every octet of the value
    size += CHALLENGE_PARAMETER_SEPARATOR.size() + name.size() +
            CHALLENGE_PARAMETER_EQUALS.size() + (value.size() * 2) + 2;
  }

  return size;
}

} // namespace

namespace sourcemeta::core {

auto http_challenge_valid(const HTTPChallenge &challenge) -> bool {
  // RFC 9110 §11.1: auth-scheme = token
  if (!http_is_token(challenge.scheme)) {
    return false;
  }

  // RFC 9110 §11.3: challenge = auth-scheme [ 1*SP ( token68 / #auth-param ) ],
  // so the two alternatives are exclusive
  if (challenge.token68.has_value()) {
    if (!challenge.parameters.empty() ||
        !http_is_b64token(challenge.token68.value())) {
      return false;
    }
  }

  // RFC 6750 §3: "All challenges defined by this specification MUST use the
  // auth-scheme value "Bearer". This scheme MUST be followed by one or more
  // auth-param values", which rules out both a bare scheme and a credential
  const auto is_bearer{equals_ignore_case(challenge.scheme, "Bearer")};
  if (is_bearer && challenge.parameters.empty()) {
    return false;
  }

  for (std::size_t index{0}; index < challenge.parameters.size(); index += 1) {
    const auto &[name, value] = challenge.parameters[index];
    // RFC 9110 §11.2: auth-param = token BWS "=" BWS ( token / quoted-string )
    if (!http_is_token(name)) {
      return false;
    }

    // RFC 9110 §11.2: "each parameter name MUST only occur once per
    // challenge", where the name "is matched case-insensitively"
    for (std::size_t other{0}; other < index; other += 1) {
      if (equals_ignore_case(challenge.parameters[other].first, name)) {
        return false;
      }
    }

    if (is_bearer && !bearer_parameter_valid(name, value)) {
      return false;
    }

    // RFC 9110 §11.5 requires a sender to spell a realm as a quoted-string,
    // and every value is spelled that way, so each one has to be encodable
    std::string scratch;
    if (!http_encode_quoted_string(value, scratch)) {
      return false;
    }
  }

  return true;
}

auto http_serialize_challenge(const HTTPChallenge &challenge, std::string &out)
    -> bool {
  if (!http_challenge_valid(challenge)) {
    return false;
  }

  out.reserve(out.size() + required_size(challenge));
  out.append(challenge.scheme);

  if (challenge.token68.has_value()) {
    out.append(CHALLENGE_SCHEME_SEPARATOR);
    out.append(challenge.token68.value());
    return true;
  }

  // RFC 9110 §11.2 admits a token as well, but the quoted-string is the only
  // spelling §11.5 lets a sender use for a realm, so every value takes it and
  // no caller has to weigh which one a given value needs
  bool first{true};
  for (const auto &[name, value] : challenge.parameters) {
    out.append(first ? CHALLENGE_SCHEME_SEPARATOR
                     : CHALLENGE_PARAMETER_SEPARATOR);
    out.append(name);
    // RFC 9110 §5.6.3: a sender must not generate the bad whitespace the
    // grammar tolerates around the equals sign
    out.append(CHALLENGE_PARAMETER_EQUALS);
    http_encode_quoted_string(value, out);
    first = false;
  }

  return true;
}

auto http_serialize_challenge(const HTTPChallenge &challenge)
    -> std::optional<std::string> {
  std::string out;
  if (!http_serialize_challenge(challenge, out)) {
    return std::nullopt;
  }

  return out;
}

auto http_serialize_challenges(std::span<const HTTPChallenge> challenges,
                               std::string &out) -> bool {
  // RFC 9110 §11.6.1: a server answering with a 401 "MUST send a
  // WWW-Authenticate header field containing at least one challenge"
  if (challenges.empty()) {
    return false;
  }

  for (const auto &challenge : challenges) {
    if (!http_challenge_valid(challenge)) {
      return false;
    }
  }

  http_serialize_challenge(challenges.front(), out);
  for (const auto &challenge : challenges.subspan(1)) {
    out.append(CHALLENGE_PARAMETER_SEPARATOR);
    http_serialize_challenge(challenge, out);
  }

  return true;
}

auto http_serialize_challenges(std::span<const HTTPChallenge> challenges)
    -> std::optional<std::string> {
  std::string out;
  if (!http_serialize_challenges(challenges, out)) {
    return std::nullopt;
  }

  return out;
}

} // namespace sourcemeta::core
