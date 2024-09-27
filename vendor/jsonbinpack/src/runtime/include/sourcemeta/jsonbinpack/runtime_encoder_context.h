#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_ENCODER_CONTEXT_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_ENCODER_CONTEXT_H_
#ifndef DOXYGEN

#include <cassert>   // assert
#include <iterator>  // std::cbegin, std::cend
#include <map>       // std::map
#include <stdexcept> // std::logic_error
#include <string>    // std::basic_string
#include <utility>   // std::pair, std::make_pair

// Encoding a shared string has some overhead, such as the
// shared string marker + the offset, so its not worth
// doing for strings that are too small.
static constexpr auto MINIMUM_STRING_LENGTH{3};

// We don't want to allow the context to grow
// forever, otherwise an attacker could force the
// program to exhaust memory given an input
// document that contains a high number of large strings.
static constexpr auto MAXIMUM_BYTE_SIZE{20971520};

namespace sourcemeta::jsonbinpack {

template <typename CharT> class Context {
public:
  enum class Type { Standalone, PrefixLengthVarintPlusOne };

  auto record(const std::basic_string<CharT> &value, const std::uint64_t offset,
              const Type type) -> void {
    const auto value_size{value.size()};
    if (value_size < MINIMUM_STRING_LENGTH) {
      return;
      // The value is too big for the context to start with
    } else if (value_size >= MAXIMUM_BYTE_SIZE) {
      return;
    }

    // Remove the oldest entries to make space if needed
    while (!this->strings.empty() &&
           this->byte_size + value_size >= MAXIMUM_BYTE_SIZE) {
      this->remove_oldest();
    }

    // If the string already exists, we want to
    // bump the offset for locality purposes.
    if (this->has(value, type)) {
      const auto key{std::make_pair(value, type)};
      const auto previous_offset{this->strings[key]};
      if (offset > previous_offset) {
        this->strings[key] = offset;
        this->offsets.erase(previous_offset);
        this->offsets.insert({offset, std::make_pair(value, type)});
      }
    } else {
      const auto result{
          this->offsets.insert({offset, std::make_pair(value, type)})};
      // Prevent recording two strings to the same offset
      assert(result.second);
      if (result.second) {
        this->strings.insert({std::make_pair(value, type), offset});
        this->byte_size += value_size;
      }
    }
  }

  auto remove_oldest() -> void {
    assert(!this->strings.empty());
    // std::map are by definition ordered by key,
    // so the begin iterator points to the entry
    // with the lowest offset, a.k.a. the oldest.
    const auto iterator{std::cbegin(this->offsets)};
    this->strings.erase(
        std::make_pair(iterator->second.first, iterator->second.second));
    this->byte_size -= iterator->second.first.size();
    this->offsets.erase(iterator);
  }

  auto has(const std::basic_string<CharT> &value, const Type type) const
      -> bool {
    return this->strings.contains(std::make_pair(value, type));
  }

  auto offset(const std::basic_string<CharT> &value, const Type type) const
      -> std::uint64_t {
    // This method assumes the value indeed exists for performance reasons
    assert(this->has(value, type));
    return this->strings.at(std::make_pair(value, type));
  }

private:
  std::map<std::pair<std::basic_string<CharT>, Type>, std::uint64_t> strings;
  // A mirror of the above map to be able to sort by offset.
  // While this means we need 2x the amount of memory to keep track
  // of strings, it allows us to efficiently put an upper bound
  // on the amount of memory being consumed by this class.
  std::map<std::uint64_t, std::pair<std::basic_string<CharT>, Type>> offsets;
  std::uint64_t byte_size = 0;
};

} // namespace sourcemeta::jsonbinpack

#endif
#endif
