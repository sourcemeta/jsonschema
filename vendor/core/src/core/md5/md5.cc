#include <sourcemeta/core/md5.h>

#include <array>   // std::array
#include <iomanip> // std::setfill, std::setw
#include <ios>     // std::hex

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#elif defined(_MSC_VER)
#pragma warning(disable : 4244 4267)
#endif
extern "C" {
#include <bearssl.h>
}
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(default : 4244 4267)
#endif

namespace sourcemeta::core {

auto md5(std::string_view input, std::ostream &output) -> void {
  br_md5_context context;
  br_md5_init(&context);
  br_md5_update(&context, input.data(), input.size());
  std::array<unsigned char, br_md5_SIZE> hash;
  br_md5_out(&context, hash.data());
  std::string_view buffer{reinterpret_cast<const char *>(hash.data()),
                          br_md5_SIZE};
  output << std::hex << std::setfill('0');
  for (const auto character : buffer) {
    output << std::setw(2)
           << static_cast<unsigned int>(static_cast<unsigned char>(character));
  }

  output.unsetf(std::ios_base::hex);
}

} // namespace sourcemeta::core
