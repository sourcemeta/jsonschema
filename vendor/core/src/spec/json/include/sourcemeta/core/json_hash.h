#ifndef SOURCEMETA_CORE_JSON_HASH_H_
#define SOURCEMETA_CORE_JSON_HASH_H_

#include <sourcemeta/core/numeric.h>

#include <bit>        // std::endian
#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <cstdint>    // std::uint8_t, std::uint64_t
#include <cstring>    // std::memcpy
#include <functional> // std::reference_wrapper

namespace sourcemeta::core {

/// @ingroup json
/// A hash function object for JSON values
template <typename T> struct HashJSON {
  using hash_type = std::uint64_t;

  auto operator()(const T &value) const noexcept -> hash_type {
    if constexpr (requires { value.get().fast_hash(); }) {
      return value.get().fast_hash();
    } else {
      return value.fast_hash();
    }
  }

  /// Check whether the given hash is a perfect hash
  [[nodiscard]]
  auto is_perfect([[maybe_unused]] const hash_type hash) const noexcept
      -> bool {
    return false;
  }
};

/// @ingroup json
/// A hash function object for JSON object property keys
template <typename T> struct PropertyHashJSON {
  /// The two halves that make up a property key hash
  struct HashType {
    /// The unsigned integer type each half is stored as
    using type = sourcemeta::core::uint128_t;
    /// The first half of the hash
    type a{0};
    /// The second half of the hash
    type b{0};

    auto operator==(const HashType &) const noexcept -> bool = default;
  };

  /// Compute a hash from up to 31 bytes of raw data, behind a leading byte
  [[nodiscard]]
  constexpr auto packed(const char *data, const std::size_t size,
                        const std::uint8_t leading) const noexcept -> HashType {
    HashType result;
    assert(size > 0);
    if consteval {
      // A constant evaluation cannot memcpy through a reinterpret_cast, so pack
      // the bytes arithmetically instead. This reproduces the runtime memcpy on
      // a little-endian target, where the packed bytes read back as the same
      // integer, which the static assertion enforces so the two can never
      // silently diverge
      static_assert(std::endian::native == std::endian::little);
      result.a = static_cast<HashType::type>(leading);
      for (std::size_t index = 0; index < size; index += 1) {
        const auto byte{static_cast<HashType::type>(
            static_cast<unsigned char>(data[index]))};
        const auto position{index + 1};
        if (position < 16) {
          result.a |= byte << static_cast<int>(8 * position);
        } else {
          result.b |= byte << static_cast<int>(8 * (position - 16));
        }
      }
    } else {
      // The leading byte goes in before the key bytes rather than being merged
      // in afterwards, as reading the whole hash back to modify it costs more
      // than computing it in the first place
      auto *const bytes{reinterpret_cast<unsigned char *>(&result)};
      bytes[0] = leading;
      std::memcpy(bytes + 1, data, size);
    }

    return result;
  }

  /// Compute a perfect hash from raw data
  [[nodiscard]]
  constexpr auto perfect(const char *data,
                         const std::size_t size) const noexcept -> HashType {
    // The key bytes alone do not tell a key from the same key followed by NUL
    // bytes, so the leading byte records the size. That makes the hash unique
    // for every key of at most 31 bytes, and a lookup that matches it has
    // compared the keys themselves
    return this->packed(data, size, static_cast<std::uint8_t>(size));
  }

  // GCC does not optimise well across implicit type conversions such as
  // std::string to std::string_view, so we provide separate overloads with
  // duplicated logic instead of unifying on a single parameter type

  constexpr auto operator()(const T &value) const noexcept -> HashType {
    const auto size{value.size()};
    switch (size) {
      case 0:
        return {};
      case 1:
        return this->perfect(value.data(), 1);
      case 2:
        return this->perfect(value.data(), 2);
      case 3:
        return this->perfect(value.data(), 3);
      case 4:
        return this->perfect(value.data(), 4);
      case 5:
        return this->perfect(value.data(), 5);
      case 6:
        return this->perfect(value.data(), 6);
      case 7:
        return this->perfect(value.data(), 7);
      case 8:
        return this->perfect(value.data(), 8);
      case 9:
        return this->perfect(value.data(), 9);
      case 10:
        return this->perfect(value.data(), 10);
      case 11:
        return this->perfect(value.data(), 11);
      case 12:
        return this->perfect(value.data(), 12);
      case 13:
        return this->perfect(value.data(), 13);
      case 14:
        return this->perfect(value.data(), 14);
      case 15:
        return this->perfect(value.data(), 15);
      case 16:
        return this->perfect(value.data(), 16);
      case 17:
        return this->perfect(value.data(), 17);
      case 18:
        return this->perfect(value.data(), 18);
      case 19:
        return this->perfect(value.data(), 19);
      case 20:
        return this->perfect(value.data(), 20);
      case 21:
        return this->perfect(value.data(), 21);
      case 22:
        return this->perfect(value.data(), 22);
      case 23:
        return this->perfect(value.data(), 23);
      case 24:
        return this->perfect(value.data(), 24);
      case 25:
        return this->perfect(value.data(), 25);
      case 26:
        return this->perfect(value.data(), 26);
      case 27:
        return this->perfect(value.data(), 27);
      case 28:
        return this->perfect(value.data(), 28);
      case 29:
        return this->perfect(value.data(), 29);
      case 30:
        return this->perfect(value.data(), 30);
      case 31:
        return this->perfect(value.data(), 31);
      default:
        // This case is specifically designed to be constant with regards to
        // string length, and to exploit the fact that most JSON objects don't
        // have a lot of entries, so hash collision is not as common. The sum
        // is kept in 64 bits on purpose, as a modulo of the 128-bit hash type
        // compiles to a library call rather than to an instruction, and the
        // leading byte stays past every size that a perfect hash records there
        const auto leading{32 +
                           ((size + static_cast<unsigned char>(value.front()) +
                             static_cast<unsigned char>(value.back())) %
                            223)};
        return this->packed(value.data(), 31,
                            static_cast<std::uint8_t>(leading));
    }
  }

  constexpr auto operator()(const char *data,
                            const std::size_t size) const noexcept -> HashType {
    switch (size) {
      case 0:
        return {};
      case 1:
        return this->perfect(data, 1);
      case 2:
        return this->perfect(data, 2);
      case 3:
        return this->perfect(data, 3);
      case 4:
        return this->perfect(data, 4);
      case 5:
        return this->perfect(data, 5);
      case 6:
        return this->perfect(data, 6);
      case 7:
        return this->perfect(data, 7);
      case 8:
        return this->perfect(data, 8);
      case 9:
        return this->perfect(data, 9);
      case 10:
        return this->perfect(data, 10);
      case 11:
        return this->perfect(data, 11);
      case 12:
        return this->perfect(data, 12);
      case 13:
        return this->perfect(data, 13);
      case 14:
        return this->perfect(data, 14);
      case 15:
        return this->perfect(data, 15);
      case 16:
        return this->perfect(data, 16);
      case 17:
        return this->perfect(data, 17);
      case 18:
        return this->perfect(data, 18);
      case 19:
        return this->perfect(data, 19);
      case 20:
        return this->perfect(data, 20);
      case 21:
        return this->perfect(data, 21);
      case 22:
        return this->perfect(data, 22);
      case 23:
        return this->perfect(data, 23);
      case 24:
        return this->perfect(data, 24);
      case 25:
        return this->perfect(data, 25);
      case 26:
        return this->perfect(data, 26);
      case 27:
        return this->perfect(data, 27);
      case 28:
        return this->perfect(data, 28);
      case 29:
        return this->perfect(data, 29);
      case 30:
        return this->perfect(data, 30);
      case 31:
        return this->perfect(data, 31);
      default:
        // This case is specifically designed to be constant with regards to
        // string length, and to exploit the fact that most JSON objects don't
        // have a lot of entries, so hash collision is not as common. The sum
        // is kept in 64 bits on purpose, as a modulo of the 128-bit hash type
        // compiles to a library call rather than to an instruction, and the
        // leading byte stays past every size that a perfect hash records there
        const auto leading{32 + ((size + static_cast<unsigned char>(data[0]) +
                                  static_cast<unsigned char>(data[size - 1])) %
                                 223)};
        return this->packed(data, 31, static_cast<std::uint8_t>(leading));
    }
  }

  /// Check whether the given hash is a perfect hash
  [[nodiscard]]
  constexpr auto is_perfect(const HashType &hash) const noexcept -> bool {
    // The first byte holds the size of a perfectly hashed key, so it is at
    // most 31 and leaves the three bits above that range clear, which no
    // imperfect hash does. Testing those three bits settles it with a single
    // masked compare, where comparing the whole byte against 31 needs a mask
    // and a compare of its own
    return (hash.a & 224) == 0;
  }
};

/// @ingroup json
/// Until C++26, `std::reference_wrapper` does not overload `operator==`,
/// so we need custom comparisons for use in i.e. `unordered_set`
/// See
/// https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper/operator_cmp.html
template <typename T> struct EqualJSON {
  auto operator()(const T &left, const T &right) const -> bool {
    if constexpr (requires { left.get() == right.get(); }) {
      return left.get() == right.get();
    } else {
      return left == right;
    }
  }
};

} // namespace sourcemeta::core

#endif
