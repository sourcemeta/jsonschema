#include <sourcemeta/core/crypto_hmac_sha256.h>

#include <openssl/core_names.h> // OSSL_MAC_PARAM_DIGEST
#include <openssl/evp.h>        // EVP_MAC_*
#include <openssl/params.h>     // OSSL_PARAM, OSSL_PARAM_construct_*

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <span>        // std::span
#include <stdexcept>   // std::runtime_error
#include <string_view> // std::string_view

namespace {

// Fetching the algorithm on every digest would repeat a provider lookup, so
// the handle is fetched once and reused, as OpenSSL recommends for frequently
// used algorithms. A failed fetch throws so that initialization is retried on
// a later call, as a provider may be loaded after the first attempt
struct HmacAlgorithm {
  HmacAlgorithm() : handle{EVP_MAC_fetch(nullptr, "HMAC", nullptr)} {
    if (this->handle == nullptr) {
      throw std::runtime_error("Could not fetch the OpenSSL HMAC algorithm");
    }
  }
  ~HmacAlgorithm() { EVP_MAC_free(this->handle); }
  HmacAlgorithm(const HmacAlgorithm &) = delete;
  HmacAlgorithm(HmacAlgorithm &&) = delete;
  auto operator=(const HmacAlgorithm &) -> HmacAlgorithm & = delete;
  auto operator=(HmacAlgorithm &&) -> HmacAlgorithm & = delete;
  EVP_MAC *handle;
};

} // namespace

namespace sourcemeta::core {

auto hmac_sha256_digest(const std::string_view key,
                        const std::span<const std::string_view> message)
    -> std::array<std::uint8_t, 32> {
  static const HmacAlgorithm algorithm;
  auto *context = EVP_MAC_CTX_new(algorithm.handle);
  if (context == nullptr) {
    throw std::runtime_error("Could not allocate OpenSSL MAC context");
  }

  // The parameter interface is not const-qualified but never writes through
  // the pointer
  std::array<OSSL_PARAM, 2> parameters{
      {OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,
                                        const_cast<char *>("SHA256"), 0),
       OSSL_PARAM_construct_end()}};

  // An empty view may carry a null pointer, which the MAC interface would
  // interpret as a request to reuse a previously set key
  static constexpr unsigned char empty_key{0x00};
  const auto *key_data{
      key.empty() ? &empty_key
                  : reinterpret_cast<const unsigned char *>(key.data())};

  auto success{EVP_MAC_init(context, key_data, key.size(), parameters.data()) ==
               1};
  for (const auto part : message) {
    success = success &&
              EVP_MAC_update(
                  context, reinterpret_cast<const unsigned char *>(part.data()),
                  part.size()) == 1;
  }

  std::array<std::uint8_t, 32> digest{};
  std::size_t length{0};
  success = success &&
            EVP_MAC_final(context, digest.data(), &length, digest.size()) == 1;
  EVP_MAC_CTX_free(context);
  if (!success) {
    throw std::runtime_error("Could not compute HMAC-SHA256 digest");
  }

  return digest;
}

auto hmac_sha256_digest(const std::string_view key,
                        const std::string_view message)
    -> std::array<std::uint8_t, 32> {
  return hmac_sha256_digest(key,
                            std::span<const std::string_view>{&message, 1});
}

} // namespace sourcemeta::core
