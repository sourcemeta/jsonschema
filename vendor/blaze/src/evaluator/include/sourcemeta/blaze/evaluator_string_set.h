#ifndef SOURCEMETA_BLAZE_EVALUATOR_STRING_SET_H
#define SOURCEMETA_BLAZE_EVALUATOR_STRING_SET_H

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>

#include <utility> // std::pair
#include <vector>  // std::vector

namespace sourcemeta::blaze {

/// @ingroup evaluator
class SOURCEMETA_BLAZE_EVALUATOR_EXPORT StringSet {
public:
  StringSet() = default;

  using value_type = sourcemeta::jsontoolkit::JSON::String;
  using hash_type = sourcemeta::jsontoolkit::JSON::Object::Container::hash_type;
  using underlying_type = std::vector<std::pair<value_type, hash_type>>;
  using size_type = typename underlying_type::size_type;
  using difference_type = typename underlying_type::difference_type;
  using const_iterator = typename underlying_type::const_iterator;

  auto contains(const value_type &value, const hash_type hash) const -> bool;
  inline auto contains(const value_type &value) const -> bool {
    return this->contains(value, this->hasher(value));
  }

  auto insert(const value_type &value) -> void;
  auto insert(value_type &&value) -> void;

  inline auto empty() const noexcept -> bool { return this->data.empty(); }
  inline auto size() const noexcept -> size_type { return this->data.size(); }

  inline auto begin() const -> const_iterator { return this->data.begin(); }
  inline auto end() const -> const_iterator { return this->data.end(); }

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251 4275)
#endif
  underlying_type data;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
  sourcemeta::jsontoolkit::Hash hasher;
};

} // namespace sourcemeta::blaze

#endif
