#ifndef SOURCEMETA_BLAZE_EVALUATOR_STRING_SET_H
#define SOURCEMETA_BLAZE_EVALUATOR_STRING_SET_H

#ifndef SOURCEMETA_BLAZE_EVALUATOR_EXPORT
#include <sourcemeta/blaze/evaluator_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <algorithm> // std::ranges::sort
#include <optional>  // std::optional
#include <utility>   // std::pair, std::move
#include <vector>    // std::vector

namespace sourcemeta::blaze {

/// @ingroup evaluator
class SOURCEMETA_BLAZE_EVALUATOR_EXPORT StringSet {
public:
  StringSet() = default;

  using string_type = sourcemeta::core::JSON::String;
  using hash_type = sourcemeta::core::JSON::Object::hash_type;
  using value_type = std::pair<string_type, hash_type>;
  using underlying_type = std::vector<value_type>;
  using size_type = typename underlying_type::size_type;
  using difference_type = typename underlying_type::difference_type;
  using const_iterator = typename underlying_type::const_iterator;

  [[nodiscard]] inline auto contains(const string_type &value,
                                     const hash_type hash) const -> bool {
    if (this->hasher.is_perfect(hash)) {
      for (const auto &entry : this->data) {
        if (entry.second == hash) {
          return true;
        }
      }
    } else {
      for (const auto &entry : this->data) {
        if (entry.second == hash && entry.first == value) {
          return true;
        }
      }
    }

    return false;
  }
  [[nodiscard]] inline auto contains(const string_type &value) const -> bool {
    return this->contains(value, this->hasher(value));
  }

  [[nodiscard]] inline auto at(const size_type index) const noexcept
      -> const value_type & {
    return this->data[index];
  }

  inline auto insert(const string_type &value) -> void {
    const auto hash{this->hasher(value)};
    if (!this->contains(value, hash)) {
      this->data.emplace_back(value, hash);
      std::ranges::sort(this->data, [](const auto &left, const auto &right) {
        return left.first < right.first;
      });
    }
  }
  inline auto insert(string_type &&value) -> void {
    const auto hash{this->hasher(value)};
    if (!this->contains(value, hash)) {
      this->data.emplace_back(std::move(value), hash);
      std::ranges::sort(this->data, [](const auto &left, const auto &right) {
        return left.first < right.first;
      });
    }
  }

  [[nodiscard]] inline auto empty() const noexcept -> bool {
    return this->data.empty();
  }
  [[nodiscard]] inline auto size() const noexcept -> size_type {
    return this->data.size();
  }

  [[nodiscard]] inline auto begin() const -> const_iterator {
    return this->data.begin();
  }
  [[nodiscard]] inline auto end() const -> const_iterator {
    return this->data.end();
  }
  [[nodiscard]] inline auto cbegin() const -> const_iterator {
    return this->data.cbegin();
  }
  [[nodiscard]] inline auto cend() const -> const_iterator {
    return this->data.cend();
  }

  [[nodiscard]] auto to_json() const -> sourcemeta::core::JSON {
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
