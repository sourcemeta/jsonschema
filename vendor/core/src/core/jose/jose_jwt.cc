#include <sourcemeta/core/jose_compact.h>
#include <sourcemeta/core/jose_jwt.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/time.h>

#include <chrono>      // std::chrono::duration, std::chrono::system_clock
#include <optional>    // std::optional, std::nullopt
#include <stdexcept>   // std::out_of_range
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
using namespace std::string_view_literals;

constexpr auto HASH_ALG{sourcemeta::core::JSON::Object::hash("alg"sv)};
constexpr auto HASH_CRIT{sourcemeta::core::JSON::Object::hash("crit"sv)};
constexpr auto HASH_KID{sourcemeta::core::JSON::Object::hash("kid"sv)};
constexpr auto HASH_TYP{sourcemeta::core::JSON::Object::hash("typ"sv)};
constexpr auto HASH_ISS{sourcemeta::core::JSON::Object::hash("iss"sv)};
constexpr auto HASH_SUB{sourcemeta::core::JSON::Object::hash("sub"sv)};
constexpr auto HASH_AUD{sourcemeta::core::JSON::Object::hash("aud"sv)};
constexpr auto HASH_EXP{sourcemeta::core::JSON::Object::hash("exp"sv)};
constexpr auto HASH_NBF{sourcemeta::core::JSON::Object::hash("nbf"sv)};
constexpr auto HASH_IAT{sourcemeta::core::JSON::Object::hash("iat"sv)};
constexpr auto HASH_JTI{sourcemeta::core::JSON::Object::hash("jti"sv)};

auto string_claim(const sourcemeta::core::JSON &object,
                  const sourcemeta::core::JSON::StringView name,
                  const sourcemeta::core::JSON::Object::hash_type hash)
    -> std::optional<std::string_view> {
  const auto *member{object.try_at(name, hash)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  return std::string_view{member->to_string()};
}

// RFC 7519 Section 5.1: a "typ" value not containing a slash is treated as if
// "application/" were prepended, so the compact and prefixed forms compare
// equal, and RFC 7515 Section 4.1.9 makes the media type case-insensitive
auto strip_application_prefix(const std::string_view value)
    -> std::string_view {
  constexpr std::string_view PREFIX{"application/"};
  if (value.size() > PREFIX.size() &&
      sourcemeta::core::equals_ignore_case(value.substr(0, PREFIX.size()),
                                           PREFIX) &&
      value.find('/', PREFIX.size()) == std::string_view::npos) {
    return value.substr(PREFIX.size());
  }

  return value;
}

auto date_claim(const sourcemeta::core::JSON &object,
                const sourcemeta::core::JSON::StringView name,
                const sourcemeta::core::JSON::Object::hash_type hash)
    -> std::optional<std::chrono::system_clock::time_point> {
  const auto *member{object.try_at(name, hash)};
  if (member == nullptr || !member->is_number()) {
    return std::nullopt;
  }

  // A NumericDate is the number of seconds since the Unix epoch, possibly
  // non-integer (RFC 7519 Section 2). A decimal-backed number (such as the
  // exponent form "1e9") whose magnitude exceeds the range of a double cannot
  // stand for a usable timestamp, and untrusted input must not abort
  double seconds{0};
  try {
    seconds = member->as_real();
  } catch (const std::out_of_range &) {
    return std::nullopt;
  }

  return sourcemeta::core::from_unix_timestamp(
      std::chrono::duration<double>{seconds});
}

} // namespace

namespace sourcemeta::core {

auto JWT::parse(const std::string_view input, JWT &result) -> bool {
  // The compact serialization is exactly three base64url segments joined by
  // dots (RFC 7515 Section 7.1)
  const auto segments{jose_compact_segments<3>(input)};
  if (!segments.has_value()) {
    return false;
  }

  const auto header_segment{segments->at(0)};
  const auto payload_segment{segments->at(1)};
  const auto signature_segment{segments->at(2)};

  auto header_bytes{base64url_decode(header_segment)};
  auto payload_bytes{base64url_decode(payload_segment)};
  auto signature_bytes{base64url_decode(signature_segment)};
  if (!header_bytes.has_value() || !payload_bytes.has_value() ||
      !signature_bytes.has_value()) {
    return false;
  }

  auto header_json{try_parse_json(header_bytes.value())};
  auto payload_json{try_parse_json(payload_bytes.value())};
  if (!header_json.has_value() || !header_json.value().is_object() ||
      !payload_json.has_value() || !payload_json.value().is_object()) {
    return false;
  }

  // RFC 7515 Section 4: the header parameter names must be unique, and RFC 7519
  // Section 4: the claim names must be unique, so a duplicate in either the
  // header or the payload is rejected (RFC 8725 Section 2.4)
  if (!header_json.value().unique_keys() ||
      !payload_json.value().unique_keys()) {
    return false;
  }

  // The algorithm header parameter is required and must be a string (RFC 7515
  // Section 4.1.1)
  const auto *algorithm{header_json.value().try_at("alg", HASH_ALG)};
  if (algorithm == nullptr || !algorithm->is_string()) {
    return false;
  }

  // Critical header extensions are not understood and must be rejected (RFC
  // 7515 Section 4.1.11)
  if (header_json.value().try_at("crit", HASH_CRIT) != nullptr) {
    return false;
  }

  result.algorithm_ = to_jws_algorithm(algorithm->to_string());
  result.signing_input_ =
      input.substr(0, header_segment.size() + payload_segment.size() + 1);
  result.signature_ = std::move(signature_bytes).value();
  result.header_ = std::move(header_json).value();
  result.payload_ = std::move(payload_json).value();
  return true;
}

JWT::JWT(const std::string_view input) {
  if (!parse(input, *this)) {
    throw JWTParseError{};
  }
}

auto JWT::from(const std::string_view input) -> std::optional<JWT> {
  JWT result;
  if (parse(input, result)) {
    return result;
  }

  return std::nullopt;
}

auto JWT::key_id() const noexcept -> std::optional<std::string_view> {
  return string_claim(this->header_, "kid", HASH_KID);
}

auto JWT::type() const noexcept -> std::optional<std::string_view> {
  return string_claim(this->header_, "typ", HASH_TYP);
}

auto JWT::has_type(const std::string_view media_type) const -> bool {
  const auto value{this->type()};
  return value.has_value() &&
         equals_ignore_case(strip_application_prefix(value.value()),
                            strip_application_prefix(media_type));
}

auto JWT::issuer() const noexcept -> std::optional<std::string_view> {
  return string_claim(this->payload_, "iss", HASH_ISS);
}

auto JWT::subject() const noexcept -> std::optional<std::string_view> {
  return string_claim(this->payload_, "sub", HASH_SUB);
}

auto JWT::token_id() const noexcept -> std::optional<std::string_view> {
  return string_claim(this->payload_, "jti", HASH_JTI);
}

auto JWT::has_audience(const std::string_view audience) const noexcept -> bool {
  const auto *member{this->payload_.try_at("aud"sv, HASH_AUD)};
  if (member == nullptr) {
    return false;
  }

  // The audience claim is either a single string or an array of strings (RFC
  // 7519 Section 4.1.3)
  if (member->is_string()) {
    return member->to_string() == audience;
  }

  if (member->is_array()) {
    for (const auto &element : member->as_array()) {
      if (element.is_string() && element.to_string() == audience) {
        return true;
      }
    }
  }

  return false;
}

auto JWT::expires_at() const
    -> std::optional<std::chrono::system_clock::time_point> {
  return date_claim(this->payload_, "exp", HASH_EXP);
}

auto JWT::not_before() const
    -> std::optional<std::chrono::system_clock::time_point> {
  return date_claim(this->payload_, "nbf", HASH_NBF);
}

auto JWT::issued_at() const
    -> std::optional<std::chrono::system_clock::time_point> {
  return date_claim(this->payload_, "iat", HASH_IAT);
}

} // namespace sourcemeta::core
