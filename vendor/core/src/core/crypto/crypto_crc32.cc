#include <sourcemeta/core/crypto_crc32.h>

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t, std::uint32_t, std::uint64_t, std::uintptr_t
#include <cstring> // std::memcpy

// Only enable the hardware CRC32 path when the target ISA explicitly promises
// the optional ARMv8 CRC32 extension. Running these instructions on a CPU
// without the extension would trap with SIGILL, so we gate on the standard
// feature macro that the compiler sets when the right -march or -mcpu is in
// effect
#if defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
#define SOURCEMETA_CORE_CRYPTO_CRC32_ARM 1
#endif

// The CRC32 operations are reached through compiler builtins rather than the
// <arm_acle.h> intrinsics. On aarch64-apple-darwin, GCC's header fails to
// compile under warnings-as-errors because of unrelated random number
// intrinsics whose inline bodies pass a pointer of the wrong width, and merely
// including the header type-checks those bodies even though we never call them
#ifdef SOURCEMETA_CORE_CRYPTO_CRC32_ARM
namespace {

auto crc32_hardware_word(const std::uint32_t checksum,
                         const std::uint64_t value) noexcept -> std::uint32_t {
#if defined(__clang__)
  return __builtin_arm_crc32d(checksum, value);
#else
  return __builtin_aarch64_crc32x(checksum, value);
#endif
}

auto crc32_hardware_byte(const std::uint32_t checksum,
                         const std::uint8_t value) noexcept -> std::uint32_t {
#if defined(__clang__)
  return __builtin_arm_crc32b(checksum, value);
#else
  return __builtin_aarch64_crc32b(checksum, value);
#endif
}

} // namespace
#endif

#ifndef SOURCEMETA_CORE_CRYPTO_CRC32_ARM
namespace {

constexpr auto compute_crc32_table_entry(const std::uint32_t value) noexcept
    -> std::uint32_t {
  auto entry{value};
  for (int bit_index = 0; bit_index < 8; ++bit_index) {
    const auto mask{
        static_cast<std::uint32_t>(-static_cast<std::int32_t>(entry & 1u))};
    entry = (entry >> 1u) ^ (0xEDB88320u & mask);
  }
  return entry;
}

constexpr auto build_crc32_tables() noexcept
    -> std::array<std::array<std::uint32_t, 256>, 8> {
  std::array<std::array<std::uint32_t, 256>, 8> tables{};
  for (std::uint32_t index = 0; index < 256u; ++index) {
    tables[0][index] = compute_crc32_table_entry(index);
  }
  // Slice-by-8 extension: T[k][b] = (T[k-1][b] >> 8) ^ T[0][T[k-1][b] & 0xff]
  for (std::size_t slice = 1; slice < 8; ++slice) {
    for (std::uint32_t index = 0; index < 256u; ++index) {
      const auto previous{tables[slice - 1][index]};
      tables[slice][index] = (previous >> 8u) ^ tables[0][previous & 0xffu];
    }
  }
  return tables;
}

constexpr std::array<std::array<std::uint32_t, 256>, 8> CRC32_TABLES{
    build_crc32_tables()};

} // namespace
#endif

namespace sourcemeta::core {

auto crc32(const std::string_view input) -> std::uint32_t {
  return crc32_update(0u, input);
}

auto crc32_update(const std::uint32_t previous, const std::string_view input)
    -> std::uint32_t {
  auto checksum{previous ^ 0xFFFFFFFFu};
  const auto *data{reinterpret_cast<const std::uint8_t *>(input.data())};
  auto remaining{input.size()};

#ifdef SOURCEMETA_CORE_CRYPTO_CRC32_ARM
  // ARMv8 hardware CRC32 instruction (~8 bytes per cycle)
  while (remaining >= 8) {
    std::uint64_t chunk{0};
    std::memcpy(&chunk, data, sizeof(chunk));
    checksum = crc32_hardware_word(checksum, chunk);
    data += 8;
    remaining -= 8;
  }
  while (remaining > 0) {
    checksum = crc32_hardware_byte(checksum, *data++);
    --remaining;
  }
  return checksum ^ 0xFFFFFFFFu;
#else
  // Slice-by-8 software fallback: consume 8 bytes per iteration
  while (remaining >= 8) {
    const std::uint32_t one{(static_cast<std::uint32_t>(data[0])) |
                            (static_cast<std::uint32_t>(data[1]) << 8u) |
                            (static_cast<std::uint32_t>(data[2]) << 16u) |
                            (static_cast<std::uint32_t>(data[3]) << 24u)};
    const std::uint32_t two{(static_cast<std::uint32_t>(data[4])) |
                            (static_cast<std::uint32_t>(data[5]) << 8u) |
                            (static_cast<std::uint32_t>(data[6]) << 16u) |
                            (static_cast<std::uint32_t>(data[7]) << 24u)};
    const auto mixed{checksum ^ one};
    checksum =
        CRC32_TABLES[0][(two >> 24u) & 0xffu] ^
        CRC32_TABLES[1][(two >> 16u) & 0xffu] ^
        CRC32_TABLES[2][(two >> 8u) & 0xffu] ^ CRC32_TABLES[3][two & 0xffu] ^
        CRC32_TABLES[4][(mixed >> 24u) & 0xffu] ^
        CRC32_TABLES[5][(mixed >> 16u) & 0xffu] ^
        CRC32_TABLES[6][(mixed >> 8u) & 0xffu] ^ CRC32_TABLES[7][mixed & 0xffu];
    data += 8;
    remaining -= 8;
  }

  // Tail: byte-by-byte for the final 0..7 bytes
  while (remaining > 0) {
    checksum = CRC32_TABLES[0][(checksum ^ *data++) & 0xffu] ^ (checksum >> 8u);
    --remaining;
  }

  return checksum ^ 0xFFFFFFFFu;
#endif
}

} // namespace sourcemeta::core
