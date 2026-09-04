#include <sourcemeta/jsonbinpack/runtime_encoder_cache.h>

namespace sourcemeta::jsonbinpack {

auto Cache::record(const sourcemeta::core::JSON::String &value,
                   const std::uint64_t offset, const Type type) -> void {
  // Encoding a shared string has some overhead, such as the
  // shared string marker + the offset, so its not worth
  // doing for strings that are too small.
  constexpr auto MINIMUM_STRING_LENGTH{3};

  // We don't want to allow the context to grow
  // forever, otherwise an attacker could force the
  // program to exhaust memory given an input
  // document that contains a high number of large strings.
  constexpr auto MAXIMUM_BYTE_SIZE{20971520};

  const auto value_size{value.size()};
  if (value_size < MINIMUM_STRING_LENGTH || value_size >= MAXIMUM_BYTE_SIZE) {
    return;
  }

  // Remove the oldest entries to make space if needed
  while (!this->data_.empty() &&
         this->byte_size_ + value_size >= MAXIMUM_BYTE_SIZE) {
    this->remove_oldest();
  }

  auto result{this->data_.insert({std::make_pair(value, type), offset})};
  if (result.second) {
    this->byte_size_ += value_size;
    this->order_.emplace(offset, result.first->first);
  } else if (offset > result.first->second) {
    this->order_.erase(result.first->second);
    // If the string already exists, we want to
    // bump the offset for locality purposes.
    result.first->second = offset;
    this->order_.emplace(offset, result.first->first);
  }

  // Otherwise we are doing something wrong
  assert(this->order_.size() == this->data_.size());
}

auto Cache::remove_oldest() -> void {
  assert(!this->data_.empty());
  // std::map are by definition ordered by key,
  // so the begin iterator points to the entry
  // with the lowest offset, a.k.a. the oldest.
  const auto iterator{this->order_.cbegin()};
  this->byte_size_ -= iterator->second.get().first.size();
  this->data_.erase(iterator->second.get());
  this->order_.erase(iterator);
}

auto Cache::find(const sourcemeta::core::JSON::String &value,
                 const Type type) const -> std::optional<std::uint64_t> {
  const auto result{this->data_.find(std::make_pair(value, type))};
  if (result == this->data_.cend()) {
    return std::nullopt;
  }

  return result->second;
}

} // namespace sourcemeta::jsonbinpack
