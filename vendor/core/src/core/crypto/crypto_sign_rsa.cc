#include <sourcemeta/core/crypto_base64.h>
#include <sourcemeta/core/crypto_sign.h>
#include <sourcemeta/core/text.h>

#include "crypto_der.h"
#include "crypto_helpers.h"

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace {

// The rsaEncryption AlgorithmIdentifier (1.2.840.113549.1.1.1 with a NULL
// parameter), taken verbatim as it never varies for an RSA key
constexpr std::array<std::uint8_t, 15> RSA_ALGORITHM_IDENTIFIER{
    {0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
     0x01, 0x05, 0x00}};

} // namespace

namespace sourcemeta::core {

// The private key components are assembled into a PKCS#8 document, which every
// backend already parses, rather than teaching each backend to build a native
// key from raw integers
auto make_rsa_private_key(
    const std::string_view modulus, const std::string_view public_exponent,
    const std::string_view private_exponent, const std::string_view prime1,
    const std::string_view prime2, const std::string_view exponent1,
    const std::string_view exponent2, const std::string_view coefficient)
    -> std::optional<PrivateKey> {
  // Reject empty or oversized components before they drive the document
  // allocation, matching the key size limit the parsing paths enforce
  const std::array<std::string_view, 8> components{
      {modulus, public_exponent, private_exponent, prime1, prime2, exponent1,
       exponent2, coefficient}};
  for (const auto component : components) {
    const auto stripped{strip_left(component, '\x00')};
    if (stripped.empty() || stripped.size() > MAXIMUM_KEY_BYTES) {
      return std::nullopt;
    }
  }

  // Every intermediate below holds private key material, so each is guarded so
  // that it is wiped when the function unwinds, including on an exception from
  // a later allocation. This does not reach copies left behind by earlier
  // string reallocations, which would need a secret-zeroing allocator to
  // eliminate

  // RSAPrivateKey (RFC 8017 Appendix A.1.2), whose version is zero for the
  // two-prime form
  std::string rsa_private_key_body;
  const SecureStringScope rsa_private_key_body_scope{rsa_private_key_body};
  der_append_unsigned_integer(rsa_private_key_body, std::string_view{});
  der_append_unsigned_integer(rsa_private_key_body, modulus);
  der_append_unsigned_integer(rsa_private_key_body, public_exponent);
  der_append_unsigned_integer(rsa_private_key_body, private_exponent);
  der_append_unsigned_integer(rsa_private_key_body, prime1);
  der_append_unsigned_integer(rsa_private_key_body, prime2);
  der_append_unsigned_integer(rsa_private_key_body, exponent1);
  der_append_unsigned_integer(rsa_private_key_body, exponent2);
  der_append_unsigned_integer(rsa_private_key_body, coefficient);
  std::string rsa_private_key;
  const SecureStringScope rsa_private_key_scope{rsa_private_key};
  der_append_element(rsa_private_key, 0x30, rsa_private_key_body);

  // PrivateKeyInfo (RFC 5958 Section 2), whose version is likewise zero
  std::string document_body;
  const SecureStringScope document_body_scope{document_body};
  der_append_unsigned_integer(document_body, std::string_view{});
  document_body.append(
      reinterpret_cast<const char *>(RSA_ALGORITHM_IDENTIFIER.data()),
      RSA_ALGORITHM_IDENTIFIER.size());
  der_append_element(document_body, 0x04, rsa_private_key);
  std::string document;
  const SecureStringScope document_scope{document};
  der_append_element(document, 0x30, document_body);

  auto encoded{base64_encode(document)};
  const SecureStringScope encoded_scope{encoded};
  std::string pem{"-----BEGIN PRIVATE KEY-----\n"};
  const SecureStringScope pem_scope{pem};
  for (std::size_t offset{0}; offset < encoded.size(); offset += 64) {
    pem.append(encoded.substr(offset, 64));
    pem.push_back('\n');
  }

  pem.append("-----END PRIVATE KEY-----\n");
  return make_private_key(pem);
}

} // namespace sourcemeta::core
