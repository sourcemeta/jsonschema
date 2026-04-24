#include <sourcemeta/core/gzip.h>

extern "C" {
#include <libdeflate.h>
}

#include <memory> // std::unique_ptr

namespace sourcemeta::core {

auto gzip(const std::uint8_t *input, const std::size_t size) -> std::string {
  std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)>
      compressor{libdeflate_alloc_compressor(1), libdeflate_free_compressor};
  if (!compressor) {
    throw GZIPError{"Could not allocate compressor"};
  }

  const auto max_size{libdeflate_gzip_compress_bound(compressor.get(), size)};
  std::string output;
  output.resize(max_size);

  const auto actual_size{libdeflate_gzip_compress(
      compressor.get(), input, size, output.data(), output.size())};

  if (actual_size == 0) {
    throw GZIPError{"Could not compress input"};
  }

  output.resize(actual_size);
  return output;
}

auto gunzip(const std::uint8_t *input, const std::size_t size,
            const std::size_t output_hint) -> std::string {
  std::unique_ptr<libdeflate_decompressor,
                  decltype(&libdeflate_free_decompressor)>
      decompressor{libdeflate_alloc_decompressor(),
                   libdeflate_free_decompressor};
  if (!decompressor) {
    throw GZIPError{"Could not allocate decompressor"};
  }

  std::string output;
  auto capacity{output_hint > 0 ? output_hint : size * 4};

  for (;;) {
    output.resize(capacity);
    std::size_t actual_size{0};
    const auto result{libdeflate_gzip_decompress(decompressor.get(), input,
                                                 size, output.data(),
                                                 output.size(), &actual_size)};

    if (result == LIBDEFLATE_SUCCESS) {
      output.resize(actual_size);
      return output;
    }

    if (result == LIBDEFLATE_INSUFFICIENT_SPACE) {
      capacity *= 2;
      continue;
    }

    throw GZIPError{"Could not decompress input"};
  }
}

} // namespace sourcemeta::core
