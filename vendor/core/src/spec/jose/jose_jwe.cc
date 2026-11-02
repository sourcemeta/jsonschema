#include <sourcemeta/core/jose_compact.h>
#include <sourcemeta/core/jose_jwe.h>

#include <sourcemeta/core/crypto.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::move

namespace {
using namespace std::string_view_literals;

constexpr auto HASH_ALG{sourcemeta::core::JSON::Object::hash("alg"sv)};
constexpr auto HASH_ENC{sourcemeta::core::JSON::Object::hash("enc"sv)};
constexpr auto HASH_CRIT{sourcemeta::core::JSON::Object::hash("crit"sv)};
constexpr auto HASH_ZIP{sourcemeta::core::JSON::Object::hash("zip"sv)};
constexpr auto HASH_KID{sourcemeta::core::JSON::Object::hash("kid"sv)};

} // namespace

namespace sourcemeta::core {

auto JWE::parse(const std::string_view input, JWE &result) -> bool {
  // The compact serialization is exactly five base64url segments joined by dots
  // (RFC 7516 Section 7.1)
  const auto segments{jose_compact_segments<5>(input)};
  if (!segments.has_value()) {
    return false;
  }

  const auto header_segment{segments->at(0)};
  const auto encrypted_key_segment{segments->at(1)};
  const auto iv_segment{segments->at(2)};
  const auto ciphertext_segment{segments->at(3)};
  const auto tag_segment{segments->at(4)};

  auto header_bytes{base64url_decode(header_segment)};
  auto encrypted_key_bytes{base64url_decode(encrypted_key_segment)};
  auto iv_bytes{base64url_decode(iv_segment)};
  auto ciphertext_bytes{base64url_decode(ciphertext_segment)};
  auto tag_bytes{base64url_decode(tag_segment)};
  if (!header_bytes.has_value() || !encrypted_key_bytes.has_value() ||
      !iv_bytes.has_value() || !ciphertext_bytes.has_value() ||
      !tag_bytes.has_value()) {
    return false;
  }

  // The JSON layer preserves repeated members rather than collapsing them, so
  // the header parameter names must be unique (RFC 7516 Section 4, RFC 8725
  // Section 2.4)
  auto header_json{try_parse_json(header_bytes.value())};
  if (!header_json.has_value() || !header_json.value().is_object() ||
      !header_json.value().unique_keys()) {
    return false;
  }

  // The algorithm and content encryption header parameters are required and
  // must be strings (RFC 7516 Sections 4.1.1 and 4.1.2)
  const auto *algorithm{header_json.value().try_at("alg", HASH_ALG)};
  const auto *encryption{header_json.value().try_at("enc", HASH_ENC)};
  if (algorithm == nullptr || !algorithm->is_string() ||
      encryption == nullptr || !encryption->is_string()) {
    return false;
  }

  // Critical header extensions are not understood and must be rejected (RFC
  // 7516 Section 4.1.13)
  if (header_json.value().try_at("crit", HASH_CRIT) != nullptr) {
    return false;
  }

  // Compression is not implemented, so a compressed object is rejected rather
  // than returning the still-compressed octets as the plaintext (RFC 7516
  // Section 5.2 step 17)
  if (header_json.value().try_at("zip", HASH_ZIP) != nullptr) {
    return false;
  }

  result.algorithm_ = to_jwe_algorithm(algorithm->to_string());
  result.encryption_ = to_jwe_encryption(encryption->to_string());
  result.protected_header_ = header_segment;
  result.encrypted_key_ = std::move(encrypted_key_bytes).value();
  result.initialization_vector_ = std::move(iv_bytes).value();
  result.ciphertext_ = std::move(ciphertext_bytes).value();
  result.tag_ = std::move(tag_bytes).value();
  result.header_ = std::move(header_json).value();
  return true;
}

JWE::JWE(const std::string_view input) {
  if (!parse(input, *this)) {
    throw JWEParseError{};
  }
}

auto JWE::from(const std::string_view input) -> std::optional<JWE> {
  JWE result;
  if (parse(input, result)) {
    return result;
  }

  return std::nullopt;
}

auto JWE::key_id() const noexcept -> std::optional<std::string_view> {
  const auto *member{this->header_.try_at("kid"sv, HASH_KID)};
  if (member == nullptr || !member->is_string()) {
    return std::nullopt;
  }

  return std::string_view{member->to_string()};
}

} // namespace sourcemeta::core
