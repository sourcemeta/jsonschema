#ifndef SOURCEMETA_JSONTOOLKIT_JSONL_ITERATOR_H_
#define SOURCEMETA_JSONTOOLKIT_JSONL_ITERATOR_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONL_EXPORT
#include <sourcemeta/jsontoolkit/jsonl_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>

#include <cstddef>  // std::ptrdiff_t
#include <cstdint>  // std::uint64_t
#include <istream>  // std::basic_istream
#include <iterator> // std::forward_iterator_tag
#include <memory>   // std::unique_ptr

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonl
/// A forward iterator to parse JSON documents out of a JSON Lines stream.
class SOURCEMETA_JSONTOOLKIT_JSONL_EXPORT ConstJSONLIterator {
public:
  ConstJSONLIterator(std::basic_istream<JSON::Char, JSON::CharTraits> *stream);
  ~ConstJSONLIterator();
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = JSON;
  using pointer = const value_type *;
  using reference = const value_type &;

  auto operator*() const -> reference;
  auto operator->() const -> pointer;
  auto operator++() -> ConstJSONLIterator &;

  SOURCEMETA_JSONTOOLKIT_JSONL_EXPORT friend auto
  operator==(const ConstJSONLIterator &left, const ConstJSONLIterator &right)
      -> bool;

private:
  std::uint64_t line{1};
  std::uint64_t column{0};
  auto parse_next() -> JSON;
  std::basic_istream<JSON::Char, JSON::CharTraits> *data;

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  // Use PIMPL idiom to hide internal details, mainly
  // templated members, which are tricky to DLL-export.
  struct Internal;
  std::unique_ptr<Internal> internal;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::jsontoolkit

#endif
