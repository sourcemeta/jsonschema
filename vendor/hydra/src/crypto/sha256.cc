#include <sourcemeta/hydra/crypto.h>

#include <iomanip> // std::setfill, std::setw
#include <ios>     // std::hex

#include "bearssl.h"

namespace sourcemeta::hydra {

auto sha256(std::string_view input, std::ostream &output) -> void {
  br_sha256_context context;
  br_sha256_init(&context);
  br_sha256_update(&context, input.data(), input.size());
  unsigned char hash[br_sha256_SIZE];
  br_sha256_out(&context, hash);
  std::string_view buffer{reinterpret_cast<const char *>(hash), br_sha256_SIZE};
  output << std::hex << std::setfill('0');
  for (const auto character : buffer) {
    output << std::setw(2)
           << static_cast<unsigned int>(static_cast<unsigned char>(character));
  }

  output.unsetf(std::ios_base::hex);
}

auto hmac_sha256(std::string_view secret, std::string_view value,
                 std::ostream &output) -> void {
  br_hmac_key_context key_context;
  br_hmac_key_init(&key_context, &br_sha256_vtable, secret.data(),
                   secret.size());
  br_hmac_context context;
  br_hmac_init(&context, &key_context, 0);
  br_hmac_update(&context, value.data(), value.size());
  unsigned char hash[br_sha256_SIZE];
  br_hmac_out(&context, hash);
  std::string_view buffer{reinterpret_cast<const char *>(hash), br_sha256_SIZE};
  output << buffer;
}

} // namespace sourcemeta::hydra
