#ifndef SOURCEMETA_CORE_CRYPTO_AES_KW_LOOP_H_
#define SOURCEMETA_CORE_CRYPTO_AES_KW_LOOP_H_

// The AES Key Wrap integrity register loop (RFC 3394), shared by the software
// backends that run it over their own AES block cipher. The single-block
// operation is passed in as a callback, so each backend supplies only the
// cipher and reports its own failures

#include <array>       // std::array
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t, std::uint64_t
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// AES Key Wrap operates on 64-bit semiblocks over 128-bit AES blocks
inline constexpr std::size_t aes_kw_semiblock_bytes{8};
using AesKeyWrapBlock = std::array<std::uint8_t, 16>;
using AesKeyWrapRegister = std::array<std::uint8_t, aes_kw_semiblock_bytes>;

// The default integrity value (RFC 3394 Section 2.2.3.1)
inline constexpr AesKeyWrapRegister aes_kw_default_iv{
    {0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6, 0xa6}};

// Fold the round counter into the integrity register as a 64-bit big-endian
// value (RFC 3394 Section 2.2.1)
inline auto aes_kw_exclusive_or_counter(AesKeyWrapRegister &value,
                                        const std::uint64_t counter) -> void {
  for (std::size_t index = 0; index < aes_kw_semiblock_bytes; ++index) {
    value[aes_kw_semiblock_bytes - 1 - index] ^=
        static_cast<std::uint8_t>((counter >> (8 * index)) & 0xffu);
  }
}

// The wrap loop over a block cipher that reports failure by returning no value
// (RFC 3394 Section 2.2.1)
template <typename Cipher>
auto aes_kw_wrap_loop(const Cipher &encrypt, const std::string_view key)
    -> std::optional<std::string> {
  const auto blocks{key.size() / aes_kw_semiblock_bytes};
  AesKeyWrapRegister integrity{aes_kw_default_iv};
  std::string registers{key};
  for (std::size_t round = 0; round < 6; ++round) {
    for (std::size_t index = 1; index <= blocks; ++index) {
      AesKeyWrapBlock input{};
      const auto offset{(index - 1) * aes_kw_semiblock_bytes};
      for (std::size_t byte = 0; byte < aes_kw_semiblock_bytes; ++byte) {
        input[byte] = integrity[byte];
        input[aes_kw_semiblock_bytes + byte] =
            static_cast<std::uint8_t>(registers[offset + byte]);
      }

      const auto output{encrypt(input)};
      if (!output.has_value()) {
        return std::nullopt;
      }

      for (std::size_t byte = 0; byte < aes_kw_semiblock_bytes; ++byte) {
        integrity[byte] = output.value()[byte];
        registers[offset + byte] =
            static_cast<char>(output.value()[aes_kw_semiblock_bytes + byte]);
      }

      aes_kw_exclusive_or_counter(integrity, (blocks * round) + index);
    }
  }

  std::string wrapped;
  wrapped.reserve(key.size() + aes_kw_semiblock_bytes);
  wrapped.append(reinterpret_cast<const char *>(integrity.data()),
                 aes_kw_semiblock_bytes);
  wrapped.append(registers);
  return wrapped;
}

// The unwrap loop, verifying the integrity register returns to the default
// value without an early exit so the comparison does not leak where a mismatch
// occurred (RFC 3394 Section 2.2.2)
template <typename Cipher>
auto aes_kw_unwrap_loop(const Cipher &decrypt,
                        const std::string_view wrapped_key)
    -> std::optional<std::string> {
  const auto blocks{(wrapped_key.size() / aes_kw_semiblock_bytes) - 1};
  AesKeyWrapRegister integrity{};
  for (std::size_t byte = 0; byte < aes_kw_semiblock_bytes; ++byte) {
    integrity[byte] = static_cast<std::uint8_t>(wrapped_key[byte]);
  }

  std::string registers{wrapped_key.substr(aes_kw_semiblock_bytes)};
  for (std::size_t round = 6; round-- > 0;) {
    for (std::size_t index = blocks; index >= 1; --index) {
      aes_kw_exclusive_or_counter(integrity, (blocks * round) + index);
      AesKeyWrapBlock input{};
      const auto offset{(index - 1) * aes_kw_semiblock_bytes};
      for (std::size_t byte = 0; byte < aes_kw_semiblock_bytes; ++byte) {
        input[byte] = integrity[byte];
        input[aes_kw_semiblock_bytes + byte] =
            static_cast<std::uint8_t>(registers[offset + byte]);
      }

      const auto output{decrypt(input)};
      if (!output.has_value()) {
        return std::nullopt;
      }

      for (std::size_t byte = 0; byte < aes_kw_semiblock_bytes; ++byte) {
        integrity[byte] = output.value()[byte];
        registers[offset + byte] =
            static_cast<char>(output.value()[aes_kw_semiblock_bytes + byte]);
      }
    }
  }

  std::uint8_t difference{0};
  for (std::size_t byte = 0; byte < aes_kw_semiblock_bytes; ++byte) {
    difference = static_cast<std::uint8_t>(
        difference | (integrity[byte] ^ aes_kw_default_iv[byte]));
  }

  if (difference != 0) {
    return std::nullopt;
  }

  return registers;
}

} // namespace sourcemeta::core

#endif
