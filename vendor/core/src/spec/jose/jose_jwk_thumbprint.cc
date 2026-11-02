#include <sourcemeta/core/jose_jwk.h>
#include <sourcemeta/core/jose_jwk_private.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>

#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// The minimal big-endian magnitude of an integer, so the RSA members serialize
// and thumbprint as the Base64urlUInt form RFC 7518 Section 2 requires, with no
// leading zero octets, regardless of how the source JWK padded them. A single
// octet is kept so the value zero stays a valid member rather than an empty one
auto minimal(std::string_view value) -> std::string_view {
  while (value.size() > 1 && value.front() == '\x00') {
    value.remove_prefix(1);
  }

  return value;
}

// The public members are mutually exclusive by construction, so a non-empty
// modulus identifies an RSA key, a non-empty coordinate an elliptic curve key,
// and a non-empty point an octet key pair. All empty means a symmetric key or
// a key whose public part could not be recovered, which has no public form
auto build_public_jwk(const std::string_view curve,
                      const std::string_view modulus,
                      const std::string_view exponent,
                      const std::string_view coordinate_x,
                      const std::string_view coordinate_y,
                      const std::string_view point)
    -> std::optional<sourcemeta::core::JSON> {
  using sourcemeta::core::base64url_encode;
  using sourcemeta::core::JSON;
  if (!modulus.empty()) {
    auto object{JSON::make_object()};
    object.assign("kty", JSON{std::string{"RSA"}});
    object.assign("n", JSON{base64url_encode(minimal(modulus))});
    object.assign("e", JSON{base64url_encode(minimal(exponent))});
    return object;
  }

  if (!coordinate_x.empty()) {
    auto object{JSON::make_object()};
    object.assign("kty", JSON{std::string{"EC"}});
    object.assign("crv", JSON{std::string{curve}});
    object.assign("x", JSON{base64url_encode(coordinate_x)});
    object.assign("y", JSON{base64url_encode(coordinate_y)});
    return object;
  }

  if (!point.empty()) {
    auto object{JSON::make_object()};
    object.assign("kty", JSON{std::string{"OKP"}});
    object.assign("crv", JSON{std::string{curve}});
    object.assign("x", JSON{base64url_encode(point)});
    return object;
  }

  return std::nullopt;
}

// RFC 7638 Section 3: the hash input is a JSON object of the required members
// only, in lexicographic member order, with no whitespace, the values being
// the base64url component strings, which contain no character JSON must escape
auto build_thumbprint(
    const std::string_view curve, const std::string_view modulus,
    const std::string_view exponent, const std::string_view coordinate_x,
    const std::string_view coordinate_y, const std::string_view point,
    const std::string_view secret) -> std::optional<std::string> {
  using sourcemeta::core::base64url_encode;
  // The canonical form embeds the secret for a symmetric key, so it is held in
  // wiping storage that clears itself on every growth and on the way out
  sourcemeta::core::SecureString canonical;
  if (!modulus.empty()) {
    canonical.append(R"({"e":")");
    canonical.append(base64url_encode(minimal(exponent)));
    canonical.append(R"(","kty":"RSA","n":")");
    canonical.append(base64url_encode(minimal(modulus)));
    canonical.append(R"("})");
  } else if (!coordinate_x.empty()) {
    canonical.append(R"({"crv":")");
    canonical.append(curve);
    canonical.append(R"(","kty":"EC","x":")");
    canonical.append(base64url_encode(coordinate_x));
    canonical.append(R"(","y":")");
    canonical.append(base64url_encode(coordinate_y));
    canonical.append(R"("})");
  } else if (!point.empty()) {
    canonical.append(R"({"crv":")");
    canonical.append(curve);
    canonical.append(R"(","kty":"OKP","x":")");
    canonical.append(base64url_encode(point));
    canonical.append(R"("})");
  } else if (!secret.empty()) {
    // RFC 7638 Section 3.2.1: the octet sequence thumbprint is over the secret,
    // encoded straight into the wiping buffer so it leaves no intermediate copy
    canonical.append(R"({"k":")");
    base64url_encode(secret, canonical);
    canonical.append(R"(","kty":"oct"})");
  } else {
    return std::nullopt;
  }

  const auto digest{sourcemeta::core::sha256_digest(canonical)};
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return base64url_encode(std::string_view{
      reinterpret_cast<const char *>(digest.data()), digest.size()});
}

} // namespace

namespace sourcemeta::core {

auto JWK::public_jwk() const -> std::optional<JSON> {
  return build_public_jwk(this->curve_, this->modulus_, this->exponent_,
                          this->coordinate_x_, this->coordinate_y_,
                          this->public_point_);
}

auto JWK::thumbprint() const -> std::optional<std::string> {
  return build_thumbprint(this->curve_, this->modulus_, this->exponent_,
                          this->coordinate_x_, this->coordinate_y_,
                          this->public_point_, this->secret_);
}

auto JWKPrivate::public_jwk() const -> std::optional<JSON> {
  return build_public_jwk(this->curve_, this->modulus_, this->exponent_,
                          this->coordinate_x_, this->coordinate_y_,
                          this->public_point_);
}

auto JWKPrivate::thumbprint() const -> std::optional<std::string> {
  return build_thumbprint(this->curve_, this->modulus_, this->exponent_,
                          this->coordinate_x_, this->coordinate_y_,
                          this->public_point_, this->secret_);
}

} // namespace sourcemeta::core
