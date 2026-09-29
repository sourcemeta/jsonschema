#include <sourcemeta/core/json_auto.h>
#include <sourcemeta/core/json_property_set.h>

#include <algorithm> // std::ranges::lower_bound
#include <cassert>   // assert
#include <optional>  // std::optional, std::nullopt
#include <utility>   // std::move

namespace sourcemeta::core {

auto JSONPropertySet::insert(const string_type &value, const hash_type hash)
    -> bool {
  assert(HASHER(value) == hash);
  const auto &entries{this->data_};
  const auto position{
      std::ranges::lower_bound(entries, value, {}, &value_type::first)};
  if (position != entries.cend() && position->first == value) {
    return false;
  }

  this->data_.emplace(position, value, hash);
  return true;
}

auto JSONPropertySet::insert(string_type &&value, const hash_type hash)
    -> bool {
  assert(HASHER(value) == hash);
  const auto &entries{this->data_};
  const auto position{
      std::ranges::lower_bound(entries, value, {}, &value_type::first)};
  if (position != entries.cend() && position->first == value) {
    return false;
  }

  this->data_.emplace(position, std::move(value), hash);
  return true;
}

auto JSONPropertySet::insert(const string_type &value) -> bool {
  return this->insert(value, HASHER(value));
}

auto JSONPropertySet::insert(string_type &&value) -> bool {
  const auto hash{HASHER(value)};
  return this->insert(std::move(value), hash);
}

auto JSONPropertySet::to_json() const -> JSON {
  return sourcemeta::core::to_json(this->data_, [](const auto &entry) -> JSON {
    return sourcemeta::core::to_json(entry.first);
  });
}

auto JSONPropertySet::from_json(const JSON &value)
    -> std::optional<JSONPropertySet> {
  if (!value.is_array()) {
    return std::nullopt;
  }

  JSONPropertySet result;
  result.data_.reserve(value.size());
  for (const auto &item : value.as_array()) {
    auto subvalue{sourcemeta::core::from_json<string_type>(item)};
    if (!subvalue.has_value()) {
      return std::nullopt;
    }

    result.insert(std::move(subvalue).value());
  }

  return result;
}

} // namespace sourcemeta::core
