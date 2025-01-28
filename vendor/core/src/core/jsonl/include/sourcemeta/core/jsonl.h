#ifndef SOURCEMETA_CORE_JSONL_H_
#define SOURCEMETA_CORE_JSONL_H_

#ifndef SOURCEMETA_CORE_JSONL_EXPORT
#include <sourcemeta/core/jsonl_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl_iterator.h>

#include <istream> // std::basic_istream

/// @defgroup jsonl JSONL
/// @brief A JSON Lines implementation with iterators support.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonl.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonl
class SOURCEMETA_CORE_JSONL_EXPORT JSONL {
public:
  /// Parse a JSONL document from a C++ standard input stream using a standard
  /// read-only C++ forward iterator interface. For example, you can parse a
  /// JSONL document and prettify each of its rows as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonl.h>
  /// #include <cassert>
  /// #include <sstream>
  /// #include <iostream>
  ///
  /// std::istringstream stream{
  ///   "{ \"foo\": 1 }\n{ \"bar\": 2 }\n{ \"baz\": 3 }"};
  ///
  /// for (const auto &document : sourcemeta::core::JSONL{stream}) {
  ///   assert(document.is_object());
  ///   sourcemeta::core::prettify(document, std::cout);
  ///   std::cout << '\n';
  /// }
  /// ```
  ///
  /// If parsing fails, sourcemeta::core::ParseError will be thrown.
  JSONL(std::basic_istream<JSON::Char, JSON::CharTraits> &stream);

  using const_iterator = ConstJSONLIterator;
  auto begin() -> const_iterator;
  auto end() -> const_iterator;
  auto cbegin() -> const_iterator;
  auto cend() -> const_iterator;

private:
  std::basic_istream<JSON::Char, JSON::CharTraits> &data;
};

} // namespace sourcemeta::core

#endif
