#ifndef SOURCEMETA_JSONTOOLKIT_JSON_HASH_H_
#define SOURCEMETA_JSONTOOLKIT_JSON_HASH_H_

#include <cstdint> // std::uint64_t
#include <cstring> // std::memcpy

namespace sourcemeta::jsontoolkit {

/// @ingroup json
template <typename T> struct FastHash {
  using hash_type = std::uint64_t;
  inline auto operator()(const T &value) const noexcept -> hash_type {
    return value.fast_hash();
  }

  struct property_hash_type {
    using type = std::uint64_t;
    type a{0};
    type b{0};
    type c{0};
    type d{0};

    inline auto operator==(const property_hash_type &other) const noexcept
        -> bool {
      return this->a == other.a && this->b == other.b && this->c == other.c &&
             this->d == other.d;
    }

    inline auto is_perfect() const noexcept -> bool {
      // If there is anything written past the first byte,
      // then it is a perfect hash
      return this->a > 255;
    }
  };

  inline auto operator()(const typename T::String &value) const noexcept
      -> property_hash_type {
    const auto size{value.size()};
    property_hash_type result;
    if (size == 0) {
      return result;
    } else if (size <= 31) {
      // Copy starting a byte 2
      std::memcpy(reinterpret_cast<char *>(&result) + 1, value.data(), size);
      return result;
    } else {
      // This case is specifically designed to be constant with regards to
      // string length, and to exploit the fact that most JSON objects don't
      // have a lot of entries, so hash collision is not as common
      return {(size +
               static_cast<typename property_hash_type::type>(value.front()) +
               static_cast<typename property_hash_type::type>(value.back())) %
              // Make sure the property hash can never exceed 8 bits
              256};
    }
  }
};

} // namespace sourcemeta::jsontoolkit

#endif