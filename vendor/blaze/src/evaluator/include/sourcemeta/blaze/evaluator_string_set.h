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
  using size_type = underlying_type::size_type;
  using difference_type = underlying_type::difference_type;
  using const_iterator = underlying_type::const_iterator;

  [[nodiscard]] auto contains(const string_type &value,
                              const hash_type hash) const -> bool {
    if (this->hasher_.is_perfect(hash)) {
      // A perfect hash captures the key bytes but not its length, so two keys
      // that only differ in trailing length hash the same and the size is
      // confirmed too
      for (const auto &entry : this->data_) {
        if (entry.second == hash && entry.first.size() == value.size()) {
          return true;
        }
      }
    } else {
      for (const auto &entry : this->data_) {
        if (entry.second == hash && entry.first == value) {
          return true;
        }
      }
    }

    return false;
  }
  [[nodiscard]] auto contains(const string_type &value) const -> bool {
    return this->contains(value, this->hasher_(value));
  }

  [[nodiscard]] auto at(const size_type index) const noexcept
      -> const value_type & {
    return this->data_[index];
  }

  auto insert(const string_type &value) -> void {
    const auto hash{this->hasher_(value)};
    if (!this->contains(value, hash)) {
      this->data_.emplace_back(value, hash);
      std::ranges::sort(this->data_,
                        [](const auto &left, const auto &right) -> bool {
                          return left.first < right.first;
                        });
    }
  }
  auto insert(string_type &&value) -> void {
    const auto hash{this->hasher_(value)};
    if (!this->contains(value, hash)) {
      this->data_.emplace_back(std::move(value), hash);
      std::ranges::sort(this->data_,
                        [](const auto &left, const auto &right) -> bool {
                          return left.first < right.first;
                        });
    }
  }

  [[nodiscard]] auto empty() const noexcept -> bool {
    return this->data_.empty();
  }
  [[nodiscard]] auto size() const noexcept -> size_type {
    return this->data_.size();
  }

  [[nodiscard]] auto begin() const -> const_iterator {
    return this->data_.begin();
  }
  [[nodiscard]] auto end() const -> const_iterator { return this->data_.end(); }
  [[nodiscard]] auto cbegin() const -> const_iterator {
    return this->data_.cbegin();
  }
  [[nodiscard]] auto cend() const -> const_iterator {
    return this->data_.cend();
  }

  [[nodiscard]] auto to_json() const -> sourcemeta::core::JSON {
    return sourcemeta::core::to_json(this->data_, [](const auto &item) -> auto {
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
  underlying_type data_;
#if defined(_MSC_VER)
#pragma warning(default : 4251 4275)
#endif
  sourcemeta::core::PropertyHashJSON<string_type> hasher_;
};

} // namespace sourcemeta::blaze

#endif
