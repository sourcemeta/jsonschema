#ifndef SOURCEMETA_CORE_JSONL_ITERATOR_H_
#define SOURCEMETA_CORE_JSONL_ITERATOR_H_

#ifndef SOURCEMETA_CORE_JSONL_EXPORT
#include <sourcemeta/core/jsonl_export.h>
#endif

#include <sourcemeta/core/json_value.h>

#include <cstddef>  // std::ptrdiff_t
#include <cstdint>  // std::uint64_t
#include <istream>  // std::basic_istream
#include <iterator> // std::forward_iterator_tag
#include <memory>   // std::unique_ptr

namespace sourcemeta::core {

/// @ingroup jsonl
/// The framing that delimits the JSON values of a stream
enum class JSONLFraming : std::uint8_t {
  /// Every value is terminated by a line feed, as in JSON Lines and NDJSON
  LineFeed,
  /// Every value is introduced by a record separator (U+001E) and terminated
  /// by a line feed, as in RFC 7464 JSON text sequences
  RecordSeparator
};

/// @ingroup jsonl
/// A forward iterator to parse JSON documents out of a JSON Lines stream. Blank
/// and whitespace-only lines are skipped rather than treated as errors.
class SOURCEMETA_CORE_JSONL_EXPORT ConstJSONLIterator {
public:
  /// Construct an iterator over the JSON documents in a stream, optionally
  /// selecting the framing that delimits them.
  ConstJSONLIterator(std::basic_istream<JSON::Char, JSON::CharTraits> *stream,
                     JSONLFraming framing = JSONLFraming::LineFeed);
  ~ConstJSONLIterator();
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = JSON;
  using pointer = const value_type *;
  using reference = const value_type &;

  auto operator*() const -> reference;
  auto operator->() const -> pointer;
  auto operator++() -> ConstJSONLIterator &;

  SOURCEMETA_CORE_JSONL_EXPORT friend auto
  operator==(const ConstJSONLIterator &left, const ConstJSONLIterator &right)
      -> bool;

private:
  std::uint64_t line_{0};
  std::uint64_t column_{0};
  JSONLFraming framing_{JSONLFraming::LineFeed};
  bool at_sequence_start_{true};
  auto parse_next() -> JSON;
  auto parse_next_line() -> JSON;
  auto parse_next_record() -> JSON;
  std::basic_istream<JSON::Char, JSON::CharTraits> *data_{};

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  // Use PIMPL idiom to hide internal details, mainly
  // templated members, which are tricky to DLL-export.
  struct Internal;
  std::unique_ptr<Internal> internal_{};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};

} // namespace sourcemeta::core

#endif
