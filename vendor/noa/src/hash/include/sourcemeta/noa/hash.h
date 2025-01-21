#ifndef SOURCEMETA_NOA_HASH_H_
#define SOURCEMETA_NOA_HASH_H_

/// @defgroup hash Hash

namespace sourcemeta::noa {

// @ingroup hash
template <typename T> struct StandardHash {
  using hash_type = std::size_t;
  inline auto operator()(const T &value) const -> hash_type {
    return this->hasher(value);
  }

  inline auto is_perfect(const hash_type &) const noexcept -> bool {
    return false;
  }

private:
  std::hash<T> hasher;
};

} // namespace sourcemeta::noa

#endif
