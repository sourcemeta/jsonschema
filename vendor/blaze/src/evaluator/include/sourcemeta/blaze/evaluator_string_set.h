#ifndef SOURCEMETA_BLAZE_EVALUATOR_STRING_SET_H
#define SOURCEMETA_BLAZE_EVALUATOR_STRING_SET_H

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <optional> // std::optional
#include <utility>  // std::pair
#include <vector>   // std::vector

namespace sourcemeta::blaze {

/// @ingroup evaluator
class SOURCEMETA_BLAZE_EVALUATOR_EXPORT StringSet {
public:
  StringSet() = default;

  using string_type = sourcemeta::core::JSON::String;
  using hash_type = sourcemeta::core::JSON::Object::Container::hash_type;
  using value_type = std::pair<string_type, hash_type>;
  using underlying_type = std::vector<value_type>;
  using size_type = typename underlying_type::size_type;
  using difference_type = typename underlying_type::difference_type;
  using const_iterator = typename underlying_type::const_iterator;

  auto contains(const string_type &value, const hash_type hash) const -> bool;
  inline auto contains(const string_type &value) const -> bool {
    return this->contains(value, this->hasher(value));
  }

  inline auto at(const size_type index) const noexcept -> const value_type & {
    return this->data[index];
  }

  auto insert(const string_type &value) -> void;
  auto insert(string_type &&value) -> void;

  inline auto empty() const noexcept -> bool { return this->data.empty(); }
  inline auto size() const noexcept -> size_type { return this->data.size(); }

  inline auto begin() const -> const_iterator { return this->data.begin(); }
  inline auto end() const -> const_iterator { return this->data.end(); }
  inline auto cbegin() const -> const_iterator { return this->data.cbegin(); }
  inline auto cend() const -> const_iterator { return this->data.cend(); }

  auto to_json() const -> sourcemeta::core::JSON {
    return sourcemeta::core::to_json(this->data, [](const auto &item) {
      return sourcemeta::core::to_json(item.first);
    });
  }

  static auto from_json(const sourcemeta::core::JSON &value)
      -> std::optional<StringSet> {
    if (!value.is_array()) {
      return std::nullopt;
    }

    StringSet result;
    for (const auto &item : value.as_array()) {
      auto subvalue{
          sourcemeta::core::from_json<sourcemeta::core::JSON::String>(item)};
      if (!subvalue.has_value()) {
        return std::nullopt;
      }

      result.insert(std::move(subvalue).value());
    }

    return result;
  }

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
  sourcemeta::core::PropertyHashJSON<string_type> hasher;
};

} // namespace sourcemeta::blaze

#endif
