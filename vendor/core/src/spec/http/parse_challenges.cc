#include <sourcemeta/core/http.h>

#include "helpers.h"

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace {

auto skip_whitespace(const std::string_view input, std::size_t position)
    -> std::size_t {
  while (position < input.size() &&
         sourcemeta::core::http_is_ows(input[position])) {
    position += 1;
  }

  return position;
}

auto scan_token(const std::string_view input, std::size_t position)
    -> std::size_t {
  while (position < input.size() &&
         sourcemeta::core::http_is_tchar(input[position])) {
    position += 1;
  }

  return position;
}

// RFC 9110 §11.2: token68 = 1*( ALPHA / DIGIT / "-" / "." / "_" / "~" / "+" /
// "/" ) *"=". The trailing padding is what makes a credential hard to tell
// from a parameter, since both can put an equals sign after a run of token
// characters, so the padding is taken greedily and the decision rests on what
// follows it
auto scan_token68(const std::string_view input, std::size_t position)
    -> std::size_t {
  const auto start{position};
  while (position < input.size() &&
         sourcemeta::core::http_is_b64token_char(input[position])) {
    position += 1;
  }

  if (position == start) {
    return start;
  }

  while (position < input.size() && input[position] == '=') {
    position += 1;
  }

  return position;
}

// A credential stands alone, so nothing but the end of the field or the
// separator before the next challenge may follow it. A parameter instead has
// its value there, which is what tells the two apart
auto ends_an_element(const std::string_view input, std::size_t position)
    -> bool {
  const auto next{skip_whitespace(input, position)};
  return next >= input.size() || input[next] == ',';
}

} // namespace

namespace sourcemeta::core {

auto http_parse_challenges(const std::string_view input,
                           std::vector<HTTPParsedChallenge> &challenges)
    -> bool {
  challenges.clear();
  std::size_t position{0};

  while (true) {
    // RFC 9110 §5.6.1.2: "A recipient MUST parse and ignore a reasonable
    // number of empty list elements"
    position = skip_whitespace(input, position);
    while (position < input.size() && input[position] == ',') {
      position = skip_whitespace(input, position + 1);
    }

    if (position >= input.size()) {
      break;
    }

    // RFC 9110 §11.1: auth-scheme = token
    const auto scheme_start{position};
    const auto scheme_end{scan_token(input, scheme_start)};
    if (scheme_end == scheme_start) {
      challenges.clear();
      return false;
    }

    HTTPParsedChallenge challenge;
    challenge.scheme =
        std::string{input.substr(scheme_start, scheme_end - scheme_start)};
    position = scheme_end;

    // RFC 9110 §11.3: the scheme is followed by at least one space before
    // either alternative, so without one the challenge carries neither
    const auto after_scheme{skip_whitespace(input, position)};
    if (after_scheme == position || after_scheme >= input.size() ||
        input[after_scheme] == ',') {
      challenges.push_back(std::move(challenge));
      position = after_scheme;
      continue;
    }

    position = after_scheme;
    const auto credential_end{scan_token68(input, position)};
    if (credential_end != position && ends_an_element(input, credential_end)) {
      challenge.token68 =
          std::string{input.substr(position, credential_end - position)};
      challenges.push_back(std::move(challenge));
      position = credential_end;
      continue;
    }

    // RFC 9110 §11.2: auth-param = token BWS "=" BWS ( token / quoted-string )
    bool first_parameter{true};
    while (true) {
      const auto name_start{position};
      const auto name_end{scan_token(input, name_start)};
      if (name_end == name_start) {
        challenges.clear();
        return false;
      }

      const auto after_name{skip_whitespace(input, name_end)};
      if (after_name >= input.size() || input[after_name] != '=') {
        // A token that no equals sign follows opens the next challenge rather
        // than continuing this one, so it is left for the outer loop to read.
        // RFC 9110 §11.6.1 delimits that list with a comma, so one may only
        // begin where a separator has just been passed
        if (first_parameter) {
          challenges.clear();
          return false;
        }

        position = name_start;
        break;
      }

      first_parameter = false;

      const auto value_start{skip_whitespace(input, after_name + 1)};
      if (value_start >= input.size()) {
        challenges.clear();
        return false;
      }

      // The unescaping, and the refusal of any control character a
      // quoted-string does not admit, is the module's own
      std::string storage;
      std::string_view scanned;
      std::string value;
      if (input[value_start] == '"') {
        const auto value_end{
            http_scan_quoted_string(input, value_start, storage, scanned)};
        if (!value_end.has_value()) {
          challenges.clear();
          return false;
        }

        value = std::string{scanned};
        position = value_end.value();
      } else {
        const auto value_end{scan_token(input, value_start)};
        if (value_end == value_start) {
          challenges.clear();
          return false;
        }

        value = std::string{input.substr(value_start, value_end - value_start)};
        position = value_end;
      }

      challenge.parameters.emplace_back(
          std::string{input.substr(name_start, name_end - name_start)},
          std::move(value));

      position = skip_whitespace(input, position);
      if (position >= input.size()) {
        break;
      }

      // RFC 9110 §11.6.1: the field is a comma-delimited list, so nothing but
      // a separator may follow a parameter
      if (input[position] != ',') {
        challenges.clear();
        return false;
      }

      // The separator between two parameters is the same one that separates
      // two challenges, so §5.6.1.2 has empty elements ignored here too
      while (position < input.size() && input[position] == ',') {
        position = skip_whitespace(input, position + 1);
      }

      if (position >= input.size()) {
        break;
      }
    }

    challenges.push_back(std::move(challenge));
  }

  return !challenges.empty();
}

} // namespace sourcemeta::core
