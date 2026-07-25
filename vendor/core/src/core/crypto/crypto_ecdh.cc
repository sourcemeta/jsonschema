#include <sourcemeta/core/crypto_ecdh.h>
#include <sourcemeta/core/crypto_sha256.h>

#include "crypto_helpers.h"

#include <algorithm>   // std::min
#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint32_t
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional, std::nullopt
#include <span>        // std::span
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

namespace {
auto append_length_prefixed(std::string &target, const std::string_view data)
    -> void {
  append_big_endian_uint32(target, static_cast<std::uint32_t>(data.size()));
  target.append(data);
}
} // namespace

auto kdf_concat(const std::string_view shared_secret,
                const std::string_view algorithm_id,
                const std::string_view party_u_info,
                const std::string_view party_v_info,
                const std::size_t derived_key_bytes)
    -> std::optional<std::string> {
  // The construction encodes each length and the key length in bits as a 32-bit
  // field, so an input that would not fit cannot be represented
  constexpr std::size_t maximum_field{
      std::numeric_limits<std::uint32_t>::max()};
  if (algorithm_id.size() > maximum_field ||
      party_u_info.size() > maximum_field ||
      party_v_info.size() > maximum_field ||
      derived_key_bytes > maximum_field / 8u) {
    return std::nullopt;
  }

  // OtherInfo (RFC 7518 Section 4.6.2) concatenates the algorithm identifier
  // and the two party information strings, each as a 32-bit big-endian length
  // followed by the data, then the key length in bits as SuppPubInfo and an
  // empty SuppPrivInfo
  std::string other_info;
  append_length_prefixed(other_info, algorithm_id);
  append_length_prefixed(other_info, party_u_info);
  append_length_prefixed(other_info, party_v_info);
  append_big_endian_uint32(other_info,
                           static_cast<std::uint32_t>(derived_key_bytes * 8u));

  // Each round hashes the big-endian round counter, the shared secret, and
  // OtherInfo, taking blocks until the requested length is reached
  std::string derived;
  derived.reserve(derived_key_bytes);
  std::uint32_t counter{1};
  while (derived.size() < derived_key_bytes) {
    std::string counter_bytes;
    append_big_endian_uint32(counter_bytes, counter);
    const std::array<std::string_view, 3> parts{
        {counter_bytes, shared_secret, other_info}};
    const auto block{sha256_digest(std::span<const std::string_view>{parts})};
    const auto available{
        std::min(derived_key_bytes - derived.size(), block.size())};
    derived.append(reinterpret_cast<const char *>(block.data()), available);
    counter += 1;
  }

  return derived;
}

} // namespace sourcemeta::core
