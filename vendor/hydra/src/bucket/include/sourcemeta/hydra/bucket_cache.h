#ifndef SOURCEMETA_HYDRA_BUCKET_CACHE_H
#define SOURCEMETA_HYDRA_BUCKET_CACHE_H

#include <sourcemeta/hydra/bucket_error.h>

#include <cassert>       // assert
#include <cstdint>       // std::uint64_t
#include <iterator>      // std::prev
#include <list>          // std::list
#include <optional>      // std::optional
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <utility>       // std::move

namespace sourcemeta::hydra {

// This entire class is considered to be private and should not be directly used
// by consumers of this project
#if !defined(DOXYGEN)
template <typename Value> class BucketCache {
public:
  // No logical size limit means no cache
  BucketCache() : logical_size_limit{0} {}
  BucketCache(const std::uint64_t limit) : logical_size_limit{limit} {}

  auto upsert(std::string key, const Value &document,
              const std::uint64_t logical_size = 1) -> void {
    if (this->logical_size_limit == 0) {
      return;
    } else if (logical_size > this->logical_size_limit) {
      throw BucketError(
          "The value does not fit within the bucket cache limits");
    }

    if (this->index.contains(key)) {
      this->remove_entry(key);
    }

    // Make space
    while (this->current_logical_size + logical_size >
           this->logical_size_limit) {
      assert(!this->queue.empty());
      // Get rid of the last value
      const auto &last_key{this->queue.back()};
      assert(this->index.contains(last_key) &&
             std::get<0>(this->index.at(last_key)) ==
                 std::prev(this->queue.end()));
      this->remove_entry(last_key);
    }

    this->current_logical_size += logical_size;
    assert(this->current_logical_size <= this->logical_size_limit);
    this->queue.push_front(key);
    this->index.insert(
        {std::move(key), {this->queue.begin(), logical_size, document}});
  }

  auto at(const std::string &key) -> std::optional<Value> {
    if (this->logical_size_limit == 0 || !this->index.contains(key)) {
      return std::nullopt;
    }

    // Move value to the front
    this->queue.splice(this->queue.begin(), this->queue,
                       std::get<0>(this->index.at(key)));
    // Otherwise there is something very off
    assert(this->queue.size() == this->index.size());
    // Get the value
    return std::get<2>(this->index.at(this->queue.front()));
  }

private:
  auto remove_entry(const std::string &key) -> void {
    assert(!this->queue.empty());
    assert(this->queue.size() == this->index.size());
    assert(this->index.contains(key));
    const auto &entry{this->index.at(key)};
    const auto iterator{std::get<0>(entry)};
    const std::uint64_t value_logical_size{std::get<1>(entry)};
    assert(this->index.find(key) != this->index.cend());
    this->index.erase(key);
    this->queue.erase(iterator);
    assert(this->queue.size() == this->index.size());
    this->current_logical_size -= value_logical_size;
  }

  const std::uint64_t logical_size_limit;
  std::uint64_t current_logical_size = 0;

  // Least-recently used values will be at the back of the list
  using List = std::list<std::string>;
  List queue;
  using Entry =
      std::tuple<typename List::iterator, const std::uint64_t, const Value>;
  std::unordered_map<std::string, Entry> index;
};
#endif

} // namespace sourcemeta::hydra

#endif
