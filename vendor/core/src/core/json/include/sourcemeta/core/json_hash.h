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
  inline auto perfect(const char *data, const std::size_t size) const noexcept
      -> hash_type {
    hash_type result;
    assert(size > 0);
    std::memcpy(reinterpret_cast<char *>(&result) + 1, data, size);
    return result;
  }

  // GCC does not optimise well across implicit type conversions such as
  // std::string to std::string_view, so we provide separate overloads with
  // duplicated logic instead of unifying on a single parameter type

  inline auto operator()(const T &value) const noexcept -> hash_type {
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
        // have a lot of entries, so hash collision is not as common
        auto hash = this->perfect(value.data(), 31);
        hash.a |=
            1 + (size + static_cast<typename hash_type::type>(value.front()) +
                 static_cast<typename hash_type::type>(value.back())) %
                    // Make sure the property hash can never exceed 8 bits
                    255;
        return hash;
    }
  }

  inline auto operator()(const char *data,
                         const std::size_t size) const noexcept -> hash_type {
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
        // have a lot of entries, so hash collision is not as common
        auto hash = this->perfect(data, 31);
        hash.a |= 1 + (size + static_cast<typename hash_type::type>(data[0]) +
                       static_cast<typename hash_type::type>(data[size - 1])) %
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
