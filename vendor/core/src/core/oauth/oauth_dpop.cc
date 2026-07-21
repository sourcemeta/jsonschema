#include <sourcemeta/core/oauth_dpop.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/jose_algorithm.h>
#include <sourcemeta/core/jose_jwk.h>
#include <sourcemeta/core/jose_jwt.h>
#include <sourcemeta/core/jose_sign.h>
#include <sourcemeta/core/jose_verify.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/oauth_random.h>

#include "oauth_json.h"
#include "oauth_syntax.h"

#include <algorithm> // std::ranges::find, std::ranges::all_of, std::ranges::count_if
#include <array>       // std::array
#include <chrono>      // std::chrono::seconds, std::chrono::duration_cast
#include <cstdint>     // std::int64_t, std::uint8_t
#include <mutex>       // std::scoped_lock
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move
#include <vector>      // std::erase_if

namespace sourcemeta::core {

namespace {

using namespace std::literals::string_view_literals;

const auto HASH_TYP{JSON::Object::hash("typ"sv)};
const auto HASH_ALG{JSON::Object::hash("alg"sv)};
const auto HASH_JWK{JSON::Object::hash("jwk"sv)};
const auto HASH_JTI{JSON::Object::hash("jti"sv)};
const auto HASH_HTM{JSON::Object::hash("htm"sv)};
const auto HASH_HTU{JSON::Object::hash("htu"sv)};
const auto HASH_IAT{JSON::Object::hash("iat"sv)};
const auto HASH_ATH{JSON::Object::hash("ath"sv)};
const auto HASH_NONCE{JSON::Object::hash("nonce"sv)};
const auto HASH_JKT{JSON::Object::hash("jkt"sv)};

// The private JSON Web Key members whose presence marks a private key (RFC 7518
// Sections 6.2.2 and 6.3.2, RFC 8037 Section 2, RFC 7518 Section 6.4)
const auto HASH_D{JSON::Object::hash("d"sv)};
const auto HASH_P{JSON::Object::hash("p"sv)};
const auto HASH_Q{JSON::Object::hash("q"sv)};
const auto HASH_DP{JSON::Object::hash("dp"sv)};
const auto HASH_DQ{JSON::Object::hash("dq"sv)};
const auto HASH_QI{JSON::Object::hash("qi"sv)};
const auto HASH_OTH{JSON::Object::hash("oth"sv)};
const auto HASH_K{JSON::Object::hash("k"sv)};

// RFC 9449 Section 4.2: the HTTP target URI without query and fragment, with
// the syntax and scheme normalization Section 4.3 recommends applied so the
// same request compares equal on both sides
auto dpop_normalize_target(const std::string_view url)
    -> std::optional<std::string> {
  auto uri{oauth_try_parse_uri(url)};
  if (!uri.has_value()) {
    return std::nullopt;
  }

  // RFC 9449 Section 4.3 recommends the syntax and scheme normalization of
  // RFC 3986 Sections 6.2.2 and 6.2.3, and Section 4.2 excludes the query and
  // fragment. RFC 9110 Section 4.2.1 gives an "http" or "https" target URI no
  // userinfo, so it is dropped rather than signed into the proof or compared
  uri->canonicalize();
  uri->query("");
  uri->userinfo("");
  if (!uri->scheme().has_value() || !uri->host().has_value()) {
    return std::nullopt;
  }

  // RFC 3986 Section 6.2.3: an authority with an empty path is equivalent to
  // one with a path of "/", a scheme-based normalization the general
  // canonicalizer leaves to the caller
  if (!uri->path().has_value() || uri->path().value().empty()) {
    uri->path(std::string{"/"});
  }

  return uri->recompose_without_fragment();
}

// RFC 9449 Section 4.2 and Section 7: the base64url-encoded SHA-256 hash of the
// access token, the value bound into a proof through the hash claim
auto dpop_access_token_hash(const std::string_view access_token)
    -> std::string {
  const auto digest{sha256_digest(access_token)};
  return base64url_encode(std::string_view{
      reinterpret_cast<const char *>(digest.data()), digest.size()});
}

// RFC 9449 Section 4.3 check 7: the public key MUST NOT contain a private key,
// whose presence is marked by any private JSON Web Key member
auto dpop_has_private_material(const JSON &key) -> bool {
  return key.try_at("d"sv, HASH_D) != nullptr ||
         key.try_at("p"sv, HASH_P) != nullptr ||
         key.try_at("q"sv, HASH_Q) != nullptr ||
         key.try_at("dp"sv, HASH_DP) != nullptr ||
         key.try_at("dq"sv, HASH_DQ) != nullptr ||
         key.try_at("qi"sv, HASH_QI) != nullptr ||
         key.try_at("oth"sv, HASH_OTH) != nullptr ||
         key.try_at("k"sv, HASH_K) != nullptr;
}

} // namespace

OAuthDPoPProofer::OAuthDPoPProofer(JWKPrivate key, const JWSAlgorithm algorithm)
    : key_{std::move(key)}, algorithm_{algorithm} {}

auto OAuthDPoPProofer::proof(const std::string_view server,
                             const std::string_view method,
                             const std::string_view url,
                             const std::string_view access_token,
                             const std::chrono::system_clock::time_point now,
                             std::string &sink) -> bool {
  // RFC 9449 Section 4.2: a DPoP proof MUST be signed with an asymmetric
  // algorithm, so a symmetric one yields no proof rather than an invalid one
  if (!jws_algorithm_is_asymmetric(this->algorithm_)) {
    return false;
  }

  auto target{dpop_normalize_target(url)};
  if (!target.has_value()) {
    return false;
  }

  auto public_key{this->key_.public_jwk()};
  if (!public_key.has_value()) {
    return false;
  }

  auto header{JSON::make_object()};
  header.assign_assume_new("typ", JSON{"dpop+jwt"}, HASH_TYP);
  header.assign_assume_new("alg", JSON{jws_algorithm_name(this->algorithm_)},
                           HASH_ALG);
  header.assign_assume_new("jwk", std::move(public_key.value()), HASH_JWK);

  auto payload{JSON::make_object()};
  const auto identifier{oauth_random_token()};
  payload.assign_assume_new(
      "jti", JSON{std::string_view{identifier.data(), identifier.size()}},
      HASH_JTI);
  payload.assign_assume_new("htm", JSON{method}, HASH_HTM);
  payload.assign_assume_new("htu", JSON{std::move(target.value())}, HASH_HTU);
  payload.assign_assume_new(
      "iat",
      JSON{static_cast<std::int64_t>(
          std::chrono::duration_cast<std::chrono::seconds>(
              now.time_since_epoch())
              .count())},
      HASH_IAT);
  if (!access_token.empty()) {
    payload.assign_assume_new("ath", JSON{dpop_access_token_hash(access_token)},
                              HASH_ATH);
  }

  {
    const std::scoped_lock lock{this->mutex_};
    const auto iterator{this->nonces_.find(server)};
    if (iterator != this->nonces_.end()) {
      payload.assign_assume_new("nonce", JSON{iterator->second}, HASH_NONCE);
    }
  }

  const auto token{jwt_sign(header, payload, this->key_)};
  if (!token.has_value()) {
    return false;
  }

  sink.append(token.value());
  return true;
}

auto OAuthDPoPProofer::observe(const std::string_view server,
                               const std::string_view nonce) -> void {
  // RFC 9449 Section 8.1: a nonce is a non-empty string of the nonce character
  // set, so a malformed one is dropped rather than echoed into a later proof
  if (!oauth_is_valid_dpop_nonce(nonce)) {
    return;
  }

  const std::scoped_lock lock{this->mutex_};
  this->nonces_.insert_or_assign(std::string{server}, std::string{nonce});
}

auto OAuthDPoPProofer::thumbprint() const -> std::optional<std::string> {
  return this->key_.thumbprint();
}

auto oauth_dpop_confirmation(const std::string_view thumbprint) -> JSON {
  auto confirmation{JSON::make_object()};
  confirmation.assign_assume_new("jkt", JSON{thumbprint}, HASH_JKT);
  return confirmation;
}

auto oauth_dpop_proof_thumbprint(const std::string_view proof)
    -> std::optional<std::string> {
  const auto token{JWT::from(proof)};
  if (!token.has_value()) {
    return std::nullopt;
  }

  const auto &header{token.value().header()};
  if (!header.is_object()) {
    return std::nullopt;
  }

  const auto *key{header.try_at("jwk"sv, HASH_JWK)};
  if (key == nullptr || !key->is_object()) {
    return std::nullopt;
  }

  // RFC 9449 Section 4.3 check 7: a proof whose embedded key carries private
  // material is invalid, so no thumbprint is derived to bind or match against
  if (dpop_has_private_material(*key)) {
    return std::nullopt;
  }

  const auto parsed{JWK::from(*key)};
  if (!parsed.has_value()) {
    return std::nullopt;
  }

  return parsed.value().thumbprint();
}

auto oauth_dpop_verify(const std::string_view proof,
                       const std::string_view method,
                       const std::string_view url,
                       const std::chrono::system_clock::time_point now,
                       const OAuthDPoPVerifyOptions &options)
    -> std::optional<OAuthDPoPError> {
  // RFC 9449 Section 4.3 check 1: exactly one DPoP header field
  if (options.proof_count != 1) {
    return OAuthDPoPError::ProofCount;
  }

  // Check 2: a single well-formed JSON Web Token
  const auto token{JWT::from(proof)};
  if (!token.has_value()) {
    return OAuthDPoPError::Malformed;
  }

  const auto &parsed{token.value()};

  // Check 4: the token type is dpop+jwt
  const auto type{parsed.type()};
  if (!type.has_value() || type.value() != "dpop+jwt") {
    return OAuthDPoPError::UnexpectedType;
  }

  // Check 5: an asymmetric algorithm, not none, within local policy
  const auto algorithm{parsed.algorithm()};
  if (!algorithm.has_value() ||
      !jws_algorithm_is_asymmetric(algorithm.value())) {
    return OAuthDPoPError::UnsupportedAlgorithm;
  }

  if (!options.allowed_algorithms.empty() &&
      std::ranges::find(options.allowed_algorithms, algorithm.value()) ==
          options.allowed_algorithms.end()) {
    return OAuthDPoPError::UnsupportedAlgorithm;
  }

  // Check 3: all required header parameters and claims are present
  const auto &header{parsed.header()};
  const auto *key{header.is_object() ? header.try_at("jwk"sv, HASH_JWK)
                                     : nullptr};
  if (key == nullptr || !key->is_object()) {
    return OAuthDPoPError::MissingClaim;
  }

  const auto identifier{parsed.token_id()};
  const auto method_claim{
      oauth_json_string_member(parsed.payload(), "htm"sv, HASH_HTM)};
  const auto target_claim{
      oauth_json_string_member(parsed.payload(), "htu"sv, HASH_HTU)};
  const auto issued{parsed.issued_at()};
  if (!identifier.has_value() || !method_claim.has_value() ||
      !target_claim.has_value() || !issued.has_value()) {
    return OAuthDPoPError::MissingClaim;
  }

  // Check 7: the embedded key carries no private material
  if (dpop_has_private_material(*key)) {
    return OAuthDPoPError::PrivateKey;
  }

  // Check 6: the signature verifies with the embedded key
  const auto public_key{JWK::from(*key)};
  if (!public_key.has_value() ||
      !jwt_verify_signature(parsed, public_key.value())) {
    return OAuthDPoPError::Signature;
  }

  // Check 8: the method claim matches the request method
  if (method_claim.value() != method) {
    return OAuthDPoPError::MethodMismatch;
  }

  // Check 9: the target claim matches the request target
  const auto normalized_claim{dpop_normalize_target(target_claim.value())};
  const auto normalized_request{dpop_normalize_target(url)};
  if (!normalized_claim.has_value() || !normalized_request.has_value() ||
      normalized_claim.value() != normalized_request.value()) {
    return OAuthDPoPError::TargetMismatch;
  }

  // Check 10: the nonce claim matches the nonce the server issued
  const auto nonce_claim{
      oauth_json_string_member(parsed.payload(), "nonce"sv, HASH_NONCE)};
  if (options.expected_nonce.has_value()) {
    if (!nonce_claim.has_value()) {
      return OAuthDPoPError::MissingNonce;
    }

    if (!secure_equals(nonce_claim.value(), options.expected_nonce.value())) {
      return OAuthDPoPError::NonceMismatch;
    }
  }

  // Check 11: the creation time is within the acceptable window
  if (issued.value() < now - options.past_window ||
      issued.value() > now + options.future_window) {
    return OAuthDPoPError::Expired;
  }

  // Check 12: the access token hash and the key binding, when a token is
  // presented
  if (options.access_token.has_value()) {
    const auto hash_claim{
        oauth_json_string_member(parsed.payload(), "ath"sv, HASH_ATH)};
    if (!hash_claim.has_value()) {
      return OAuthDPoPError::AccessTokenMismatch;
    }

    const auto expected{dpop_access_token_hash(options.access_token.value())};
    if (!secure_equals(hash_claim.value(), expected)) {
      return OAuthDPoPError::AccessTokenMismatch;
    }

    // RFC 9449 Section 7.1: a protected resource MUST confirm the proof key
    // matches the key the token is bound to, so presenting a token without a
    // binding to check against fails closed rather than skipping that
    // confirmation
    if (!options.bound_thumbprint.has_value()) {
      return OAuthDPoPError::KeyMismatch;
    }
  }

  if (options.bound_thumbprint.has_value()) {
    const auto proof_thumbprint{public_key.value().thumbprint()};
    if (!proof_thumbprint.has_value() ||
        !secure_equals(proof_thumbprint.value(),
                       options.bound_thumbprint.value())) {
      return OAuthDPoPError::KeyMismatch;
    }
  }

  return std::nullopt;
}

auto OAuthDPoPReplayStore::check_and_insert(
    const std::string_view identifier, const std::string_view target,
    const std::chrono::system_clock::time_point now,
    const std::chrono::seconds window, const bool normalize_target) -> bool {
  // RFC 9449 Section 11.1: the identifier is tracked in the context of the
  // target, so the digest covers both and one store safely spans targets. A
  // DPoP target is normalized the same way the verifier compares the htu claim
  // (Section 4.3 check 9), so a replay to an equivalent but differently spelled
  // target is still detected rather than admitted as a fresh identifier, while
  // a target compared verbatim, such as an assertion audience, is keyed as is
  const auto normalized{normalize_target ? dpop_normalize_target(target)
                                         : std::nullopt};
  const std::string_view effective{
      normalized.has_value() ? std::string_view{normalized.value()} : target};
  std::string material;
  material.reserve(identifier.size() + 1 + effective.size());
  material.append(identifier);
  material.push_back('\0');
  material.append(effective);
  const auto digest{sha256_digest(material)};

  const std::scoped_lock lock{this->mutex_};
  std::erase_if(this->entries_, [now](const Entry &entry) -> bool {
    return entry.expiry <= now;
  });

  for (const auto &existing : this->entries_) {
    if (existing.digest == digest) {
      return false;
    }
  }

  // RFC 9449 Section 11.1: once the expired entries are pruned every remaining
  // one still guards against a replay, so a full store fails closed rather than
  // evict a live entry. Evicting one would drop its guard and let that proof be
  // replayed, so the new proof is rejected until a slot frees on expiry. The
  // capacity bounds the memory an attacker minting distinct proofs can consume
  if (this->entries_.size() >= this->capacity_) {
    return false;
  }

  this->entries_.push_back(Entry{.digest = digest, .expiry = now + window});
  return true;
}

auto OAuthDPoPReplayStore::size(
    const std::chrono::system_clock::time_point now) const -> std::size_t {
  const std::scoped_lock lock{this->mutex_};
  return static_cast<std::size_t>(
      std::ranges::count_if(this->entries_, [now](const Entry &entry) -> bool {
        return entry.expiry > now;
      }));
}

auto oauth_is_valid_dpop_nonce(const std::string_view value) noexcept -> bool {
  if (value.empty()) {
    return false;
  }

  // RFC 6749 Appendix A: NQCHAR = %x21 / %x23-5B / %x5D-7E, the printable ASCII
  // set excluding the space, the double quote, and the backslash
  return std::ranges::all_of(value, [](const char character) -> bool {
    const auto byte{static_cast<unsigned char>(character)};
    return byte == 0x21 || (byte >= 0x23 && byte <= 0x5B) ||
           (byte >= 0x5D && byte <= 0x7E);
  });
}

} // namespace sourcemeta::core
