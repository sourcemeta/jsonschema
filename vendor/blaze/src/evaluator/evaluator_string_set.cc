#include <sourcemeta/blaze/evaluator_string_set.h>

#include <algorithm> // std::sort

namespace sourcemeta::blaze {

auto StringSet::contains(const string_type &value, const hash_type hash) const
    -> bool {
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

auto StringSet::insert(const string_type &value) -> void {
  const auto hash{this->hasher(value)};
  if (!this->contains(value, hash)) {
    this->data.emplace_back(value, hash);
    std::ranges::sort(this->data, [](const auto &left, const auto &right) {
      return left.first < right.first;
    });
  }
}

auto StringSet::insert(string_type &&value) -> void {
  const auto hash{this->hasher(value)};
  if (!this->contains(value, hash)) {
    this->data.emplace_back(std::move(value), hash);
    std::ranges::sort(this->data, [](const auto &left, const auto &right) {
      return left.first < right.first;
    });
  }
}

} // namespace sourcemeta::blaze
