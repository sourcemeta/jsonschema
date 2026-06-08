#include <sourcemeta/core/crypto_crc32.h>

#include <array>   // std::array
#include <cstddef> // std::size_t
#include <cstdint> // std::uint8_t, std::uint32_t

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

constexpr auto build_crc32_table() noexcept -> std::array<std::uint32_t, 256> {
  std::array<std::uint32_t, 256> table{};
  for (std::uint32_t index = 0; index < 256u; ++index) {
    table[index] = compute_crc32_table_entry(index);
  }
  return table;
}

constexpr std::array<std::uint32_t, 256> CRC32_TABLE{build_crc32_table()};

} // namespace

namespace sourcemeta::core {

auto crc32(const std::string_view input) -> std::uint32_t {
  return crc32_update(0u, input);
}

auto crc32_update(const std::uint32_t previous, const std::string_view input)
    -> std::uint32_t {
  auto checksum{previous ^ 0xFFFFFFFFu};
  for (const auto character : input) {
    const auto byte{static_cast<std::uint8_t>(character)};
    checksum = CRC32_TABLE[(checksum ^ byte) & 0xffu] ^ (checksum >> 8u);
  }
  return checksum ^ 0xFFFFFFFFu;
}

} // namespace sourcemeta::core
