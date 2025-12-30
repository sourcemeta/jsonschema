#ifndef SOURCEMETA_CORE_JSON_HASH_H_
#define SOURCEMETA_CORE_JSON_HASH_H_

#include <cassert>    // assert
#include <cstdint>    // std::uint64_t
#include <cstring>    // std::memcpy
#include <functional> // std::reference_wrapper

namespace sourcemeta::core {

/// @ingroup json
template <typename T> struct HashJSON {
  using hash_type = std::uint64_t;

  inline auto operator()(const T &value) const noexcept -> hash_type {
    if constexpr (requires { value.get().fast_hash(); }) {
      return value.get().fast_hash();
    } else {
      return value.fast_hash();
    }
  }

  [[nodiscard]]
  inline auto is_perfect(const hash_type) const noexcept -> bool {
    return false;
  }
};

/// @ingroup json
template <typename T> struct PropertyHashJSON {
  struct hash_type {
    // For performance when the platform allows it
#if defined(__SIZEOF_INT128__)
    using type = __uint128_t;
    type a{0};
    type b{0};
#else
    using type = std::uint64_t;
    type a{0};
    type b{0};
    type c{0};
    type d{0};
#endif

    inline auto operator==(const hash_type &other) const noexcept -> bool {
#if defined(__SIZEOF_INT128__)
      return this->a == other.a && this->b == other.b;
#else
      return this->a == other.a && this->b == other.b && this->c == other.c &&
             this->d == other.d;
#endif
    }
  };

  [[nodiscard]]
  inline auto perfect(const T &value, const std::size_t size) const noexcept
      -> hash_type {
    hash_type result;
    assert(!value.empty());
    // Copy starting a byte 2
    std::memcpy(reinterpret_cast<char *>(&result) + 1, value.data(), size);
    return result;
  }

  inline auto operator()(const T &value) const noexcept -> hash_type {
    const auto size{value.size()};
    switch (size) {
      case 0:
        return {};
      case 1:
        return this->perfect(value, 1);
      case 2:
        return this->perfect(value, 2);
      case 3:
        return this->perfect(value, 3);
      case 4:
        return this->perfect(value, 4);
      case 5:
        return this->perfect(value, 5);
      case 6:
        return this->perfect(value, 6);
      case 7:
        return this->perfect(value, 7);
      case 8:
        return this->perfect(value, 8);
      case 9:
        return this->perfect(value, 9);
      case 10:
        return this->perfect(value, 10);
      case 11:
        return this->perfect(value, 11);
      case 12:
        return this->perfect(value, 12);
      case 13:
        return this->perfect(value, 13);
      case 14:
        return this->perfect(value, 14);
      case 15:
        return this->perfect(value, 15);
      case 16:
        return this->perfect(value, 16);
      case 17:
        return this->perfect(value, 17);
      case 18:
        return this->perfect(value, 18);
      case 19:
        return this->perfect(value, 19);
      case 20:
        return this->perfect(value, 20);
      case 21:
        return this->perfect(value, 21);
      case 22:
        return this->perfect(value, 22);
      case 23:
        return this->perfect(value, 23);
      case 24:
        return this->perfect(value, 24);
      case 25:
        return this->perfect(value, 25);
      case 26:
        return this->perfect(value, 26);
      case 27:
        return this->perfect(value, 27);
      case 28:
        return this->perfect(value, 28);
      case 29:
        return this->perfect(value, 29);
      case 30:
        return this->perfect(value, 30);
      case 31:
        return this->perfect(value, 31);
      default:
        // This case is specifically designed to be constant with regards to
        // string length, and to exploit the fact that most JSON objects don't
        // have a lot of entries, so hash collision is not as common
        auto hash = this->perfect(value, 31);
        hash.a |=
            1 + (size + static_cast<typename hash_type::type>(value.front()) +
                 static_cast<typename hash_type::type>(value.back())) %
                    // Make sure the property hash can never exceed 8 bits
                    255;
        return hash;
    }
  }

  [[nodiscard]]
  inline auto is_perfect(const hash_type &hash) const noexcept -> bool {
    // If there is anything written past the first byte,
    // then it is a perfect hash
    return (hash.a & 255) == 0;
  }
};

/// @ingroup json
/// Until C++26, `std::reference_wrapper` does not overload `operator==`,
/// so we need custom comparisons for use in i.e. `unordered_set`
/// See
/// https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper/operator_cmp.html
template <typename T> struct EqualJSON {
  inline auto operator()(const T &left, const T &right) const -> bool {
    if constexpr (requires { left.get() == right.get(); }) {
      return left.get() == right.get();
    } else {
      return left == right;
    }
  }
};

} // namespace sourcemeta::core

#endif
