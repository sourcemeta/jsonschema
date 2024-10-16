#include <sourcemeta/hydra/crypto.h>

#include <iomanip> // std::setfill, std::setw
#include <ios>     // std::hex

#include "bearssl.h"

namespace sourcemeta::hydra {

auto md5(std::string_view input, std::ostream &output) -> void {
  br_md5_context context;
  br_md5_init(&context);
  br_md5_update(&context, input.data(), input.size());
  unsigned char hash[br_md5_SIZE];
  br_md5_out(&context, hash);
  std::string_view buffer{reinterpret_cast<const char *>(hash), br_md5_SIZE};
  output << std::hex << std::setfill('0');
  for (const auto character : buffer) {
    output << std::setw(2)
           << static_cast<unsigned int>(static_cast<unsigned char>(character));
  }

  output.unsetf(std::ios_base::hex);
}

} // namespace sourcemeta::hydra
