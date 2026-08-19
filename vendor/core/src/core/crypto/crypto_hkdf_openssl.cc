#include "crypto_kdf.h"

#include <openssl/core_names.h> // OSSL_KDF_PARAM_*
#include <openssl/kdf.h>        // EVP_KDF_*, EVP_KDF_HKDF_MODE_*
#include <openssl/params.h>     // OSSL_PARAM, OSSL_PARAM_construct_*

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <stdexcept>   // std::runtime_error
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {

// Performing the derivation through the key derivation interface, rather than
// composing it from separate HMAC calls, is what keeps the operation inside a
// validated provider such as the FIPS one
struct HkdfAlgorithm {
  HkdfAlgorithm() : handle{EVP_KDF_fetch(nullptr, "HKDF", nullptr)} {
    if (this->handle == nullptr) {
      throw std::runtime_error("Could not fetch the OpenSSL HKDF algorithm");
    }
  }
  ~HkdfAlgorithm() { EVP_KDF_free(this->handle); }
  HkdfAlgorithm(const HkdfAlgorithm &) = delete;
  HkdfAlgorithm(HkdfAlgorithm &&) = delete;
  auto operator=(const HkdfAlgorithm &) -> HkdfAlgorithm & = delete;
  auto operator=(HkdfAlgorithm &&) -> HkdfAlgorithm & = delete;
  EVP_KDF *handle;
};

auto digest_name(const sourcemeta::core::KDFHash hash) noexcept -> const
    char * {
  switch (hash) {
    case sourcemeta::core::KDFHash::SHA256:
      return "SHA256";
    case sourcemeta::core::KDFHash::SHA384:
      return "SHA384";
    case sourcemeta::core::KDFHash::SHA512:
      return "SHA512";
  }

  std::unreachable();
}

// An empty view may carry a null pointer, which the parameter interface would
// read as an absent parameter rather than as a zero-length one
constexpr unsigned char EMPTY{0x00};

auto octets(const std::string_view value) noexcept -> unsigned char * {
  return value.empty()
             ? const_cast<unsigned char *>(&EMPTY)
             : const_cast<unsigned char *>(
                   reinterpret_cast<const unsigned char *>(value.data()));
}

// The parameter interface is not const-qualified but never writes through the
// pointers it is handed
auto derive(const sourcemeta::core::KDFHash hash, const int mode,
            const std::string_view key, const std::string_view salt,
            const std::string_view info, unsigned char *const output,
            const std::size_t length) -> bool {
  static const HkdfAlgorithm algorithm;
  auto *context{EVP_KDF_CTX_new(algorithm.handle)};
  if (context == nullptr) {
    return false;
  }

  int kdf_mode{mode};
  std::array<OSSL_PARAM, 6> parameters{
      {OSSL_PARAM_construct_utf8_string(
           OSSL_KDF_PARAM_DIGEST, const_cast<char *>(digest_name(hash)), 0),
       OSSL_PARAM_construct_int(OSSL_KDF_PARAM_MODE, &kdf_mode),
       OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, octets(key),
                                         key.size()),
       OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, octets(salt),
                                         salt.size()),
       OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, octets(info),
                                         info.size()),
       OSSL_PARAM_construct_end()}};

  const auto success{
      EVP_KDF_derive(context, output, length, parameters.data()) == 1};
  EVP_KDF_CTX_free(context);
  return success;
}

} // namespace

namespace sourcemeta::core {

auto hkdf_extract(const KDFHash hash, const std::string_view salt,
                  const std::string_view input_key_material,
                  unsigned char *const output) -> bool {
  // The extract step yields exactly one digest, which is the only length the
  // provider accepts in this mode
  return derive(hash, EVP_KDF_HKDF_MODE_EXTRACT_ONLY, input_key_material, salt,
                {}, output, kdf_digest_bytes(hash));
}

auto hkdf_expand(const KDFHash hash, const std::string_view pseudorandom_key,
                 const std::string_view info, unsigned char *const output,
                 const std::size_t length) -> bool {
  return derive(hash, EVP_KDF_HKDF_MODE_EXPAND_ONLY, pseudorandom_key, {}, info,
                output, length);
}

auto hkdf_derive(const KDFHash hash, const std::string_view input_key_material,
                 const std::string_view salt, const std::string_view info,
                 unsigned char *const output, const std::size_t length)
    -> bool {
  // Both steps in one call, so the pseudorandom key never leaves the provider
  return derive(hash, EVP_KDF_HKDF_MODE_EXTRACT_AND_EXPAND, input_key_material,
                salt, info, output, length);
}

} // namespace sourcemeta::core
