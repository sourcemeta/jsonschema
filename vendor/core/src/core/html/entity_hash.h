#ifndef SOURCEMETA_CORE_HTML_ENTITY_HASH_H_
#define SOURCEMETA_CORE_HTML_ENTITY_HASH_H_

#include <cstdint>     // std::uint32_t, std::uint64_t
#include <string_view> // std::string_view

// The perfect hash of the named character references, shared by the code
// generator that builds the table and the lookup that reads it. The upper half
// of the hash of a name picks a bucket, and the lower half, mixed with the
// displacement that the generator chose for that bucket, picks the slot

namespace sourcemeta::core {

inline auto html_entity_hash(const std::string_view name,
                             const std::uint64_t seed) noexcept
    -> std::uint64_t {
  std::uint64_t hash{0xCBF29CE484222325ULL ^ seed};
  for (const auto character : name) {
    hash ^= static_cast<unsigned char>(character);
    hash *= 0x100000001B3ULL;
  }

  hash ^= hash >> 33U;
  hash *= 0xFF51AFD7ED558CCDULL;
  hash ^= hash >> 33U;
  return hash;
}

// Map a value onto a range with a multiplication instead of a division
inline auto html_entity_reduce(const std::uint32_t value,
                               const std::uint32_t range) noexcept
    -> std::uint32_t {
  return static_cast<std::uint32_t>(
      (static_cast<std::uint64_t>(value) * range) >> 32U);
}

inline auto html_entity_bucket(const std::uint64_t hash,
                               const std::uint32_t buckets) noexcept
    -> std::uint32_t {
  return html_entity_reduce(static_cast<std::uint32_t>(hash >> 32U), buckets);
}

inline auto html_entity_slot(const std::uint64_t hash,
                             const std::uint32_t displacement,
                             const std::uint32_t slots) noexcept
    -> std::uint32_t {
  auto value{static_cast<std::uint32_t>(hash) ^ (displacement * 0x9E3779B9U)};
  value ^= value >> 16U;
  value *= 0x85EBCA6BU;
  value ^= value >> 13U;
  value *= 0xC2B2AE35U;
  value ^= value >> 16U;
  return html_entity_reduce(value, slots);
}

} // namespace sourcemeta::core

#endif
